// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeDuel(uint8 LeftColumn, uint8 LeftRow, uint8 RightColumn, uint8 RightRow)
	{
		FBattleState State;

		FPetState Esquerdo;
		Esquerdo.PetId = 1; Esquerdo.Side = 0;
		Esquerdo.Column = LeftColumn; Esquerdo.Row = LeftRow;
		Esquerdo.Health = 300; Esquerdo.MaxHealth = 300;
		Esquerdo.Attack = 60; Esquerdo.Defense = 10; Esquerdo.Speed = 5;

		FPetState Direito = Esquerdo;
		Direito.PetId = 2; Direito.Side = 1;
		Direito.Column = RightColumn; Direito.Row = RightRow;

		State.Pets.Add(Esquerdo);
		State.Pets.Add(Direito);
		return State;
	}

	FBattleAction EncounterAct(EActionType Type, EBattleDirection Direction = EBattleDirection::Nenhuma)
	{
		FBattleAction Action;
		Action.Type = Type;
		Action.Direction = Direction;
		return Action;
	}

	const FPetState& EncounterPetOnSide(const FBattleState& State, uint8 Side)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side) { return Pet; }
		}
		return State.Pets[0];
	}

	void EncounterRunSlot(FBattleState& State, const FBattleAction& Left, const FBattleAction& Right,
		TArray<FBattleEvent>& Trace, uint8 SlotIndex = 0)
	{
		BattlePhases::ApplyPostures(State,
		BattlePhases::DuelSlotActions(State, Left, Right), SlotIndex, Trace);
		BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, Left, Right), SlotIndex, Trace);
		BattlePhases::ApplyCombat(State,
		BattlePhases::DuelSlotActions(State, Left, Right), SlotIndex, Trace);
		BattlePhases::ApplyResolution(State, SlotIndex, Trace);
	}

	bool EncounterHasEvent(const TArray<FBattleEvent>& Trace, EBattleEventType Type)
	{
		for (const FBattleEvent& Event : Trace)
		{
			if (Event.Type == Type) { return true; }
		}
		return false;
	}
}

// DP-02 invertido: dois pets NÃO ocupam a mesma casa. Os dois indo para o
// mesmo ponto ficam onde estavam — empilhar era o comportamento antigo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSameCellEncounterBlocksBothTest,
	"BattleSim.Encounter.SameCell.BlocksBoth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSameCellEncounterBlocksBothTest::RunTest(const FString& Parameters)
{
	// (0,1) e (2,1) mirando a casa do meio.
	FBattleState State = MakeDuel(0, 1, 2, 1);
	TArray<FBattleEvent> Trace;

	EncounterRunSlot(State, EncounterAct(EActionType::Mover, EBattleDirection::Direita),
		EncounterAct(EActionType::Mover, EBattleDirection::Esquerda), Trace);

	TestEqual(TEXT("Esquerdo ficou onde estava"), EncounterPetOnSide(State, 0).Column, static_cast<uint8>(0));
	TestEqual(TEXT("Direito ficou onde estava"), EncounterPetOnSide(State, 1).Column, static_cast<uint8>(2));
	TestTrue(TEXT("O encontro foi registrado"),
		EncounterHasEvent(Trace, EBattleEventType::EncontroNoMesmoPonto));
	return true;
}

// "Se acabarem de se reencontrar no ponto do campo, eles se atacam."
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSameCellEncounterHurtsBothTest,
	"BattleSim.Encounter.SameCell.HurtsBoth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSameCellEncounterHurtsBothTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeDuel(0, 1, 2, 1);
	TArray<FBattleEvent> Trace;

	EncounterRunSlot(State, EncounterAct(EActionType::Mover, EBattleDirection::Direita),
		EncounterAct(EActionType::Mover, EBattleDirection::Esquerda), Trace);

	TestTrue(TEXT("Esquerdo se feriu no encontro"), EncounterPetOnSide(State, 0).Health < 300);
	TestTrue(TEXT("Direito se feriu no encontro"), EncounterPetOnSide(State, 1).Health < 300);
	return true;
}

// Andar para cima de quem está parado é o mesmo encontro: não dá para entrar
// na casa de alguém, e a trombada machuca os dois.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkingIntoAStandingOpponentCollidesTest,
	"BattleSim.Encounter.SameCell.WalkingIntoStandingOpponent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWalkingIntoAStandingOpponentCollidesTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeDuel(1, 1, 2, 1);
	TArray<FBattleEvent> Trace;

	EncounterRunSlot(State, EncounterAct(EActionType::Mover, EBattleDirection::Direita), EncounterAct(EActionType::Aguardar), Trace);

	TestEqual(TEXT("Não entrou na casa ocupada"), EncounterPetOnSide(State, 0).Column, static_cast<uint8>(1));
	TestTrue(TEXT("Quem andou se feriu"), EncounterPetOnSide(State, 0).Health < 300);
	TestTrue(TEXT("Quem estava parado também"), EncounterPetOnSide(State, 1).Health < 300);
	return true;
}

// Pedido explícito do usuário: as ações já registradas VALEM no encontro.
// Quem defendeu sofre menos; quem esquivou não sofre. Isto sai de graça se o
// encontro passar pelo MESMO caminho de dano do ataque — e é exatamente por
// isso que ele passa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEncounterRespectsPosturesTest,
	"BattleSim.Encounter.SameCell.RespectsPostures",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEncounterRespectsPosturesTest::RunTest(const FString& Parameters)
{
	// Esquivou: o encontro é trombada FÍSICA, e esquiva anula físico.
	{
		FBattleState State = MakeDuel(1, 1, 2, 1);
		TArray<FBattleEvent> Trace;
		EncounterRunSlot(State, EncounterAct(EActionType::Mover, EBattleDirection::Direita), EncounterAct(EActionType::Esquivar), Trace);
		TestEqual(TEXT("Quem esquivou não se feriu"), EncounterPetOnSide(State, 1).Health, 300);
	}

	// Defendeu: sofre, mas menos que quem não fez nada.
	int32 DanoDefendendo = 0;
	{
		FBattleState State = MakeDuel(1, 1, 2, 1);
		TArray<FBattleEvent> Trace;
		EncounterRunSlot(State, EncounterAct(EActionType::Mover, EBattleDirection::Direita), EncounterAct(EActionType::Defender), Trace);
		DanoDefendendo = 300 - EncounterPetOnSide(State, 1).Health;
		TestTrue(TEXT("Quem defendeu ainda sofre algo"), DanoDefendendo > 0);
	}
	{
		FBattleState State = MakeDuel(1, 1, 2, 1);
		TArray<FBattleEvent> Trace;
		EncounterRunSlot(State, EncounterAct(EActionType::Mover, EBattleDirection::Direita), EncounterAct(EActionType::Aguardar), Trace);
		const int32 DanoParado = 300 - EncounterPetOnSide(State, 1).Health;
		TestTrue(TEXT("Defender reduz o dano da trombada"), DanoDefendendo < DanoParado);
	}
	return true;
}

// Trocar de casas continua permitido: ninguém TERMINA no mesmo ponto, e é a
// posição final que a regra governa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSwappingCellsIsStillAllowedTest,
	"BattleSim.Encounter.SameCell.SwappingStillAllowed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSwappingCellsIsStillAllowedTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeDuel(1, 1, 2, 1);
	TArray<FBattleEvent> Trace;

	EncounterRunSlot(State, EncounterAct(EActionType::Mover, EBattleDirection::Direita),
		EncounterAct(EActionType::Mover, EBattleDirection::Esquerda), Trace);

	TestEqual(TEXT("Esquerdo assumiu a casa do outro"), EncounterPetOnSide(State, 0).Column, static_cast<uint8>(2));
	TestEqual(TEXT("Direito assumiu a casa do outro"), EncounterPetOnSide(State, 1).Column, static_cast<uint8>(1));
	return true;
}
