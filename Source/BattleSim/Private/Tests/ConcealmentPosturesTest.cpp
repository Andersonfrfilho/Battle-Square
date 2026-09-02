// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Battle/BattleArenaConstants.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeAdjacentDuel()
	{
		FBattleState State;

		FPetState Atacante;
		Atacante.PetId = 1; Atacante.Side = 0;
		Atacante.Column = 1; Atacante.Row = 1;
		Atacante.Health = 200; Atacante.MaxHealth = 200;
		Atacante.Attack = 50; Atacante.Defense = 10; Atacante.Speed = 5;

		FPetState Alvo = Atacante;
		Alvo.PetId = 2; Alvo.Side = 1;
		Alvo.Column = 2; Alvo.Row = 1;

		State.Pets.Add(Atacante);
		State.Pets.Add(Alvo);
		return State;
	}

	FBattleAction Act(EActionType Type, EBattleDirection Direction = EBattleDirection::Nenhuma)
	{
		FBattleAction Action;
		Action.Type = Type;
		Action.Direction = Direction;
		return Action;
	}

	const FPetState& PetOnSide(const FBattleState& State, uint8 Side)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side) { return Pet; }
		}
		return State.Pets[0];
	}

	/** Um slot completo: postura, movimento, combate, resolução. */
	void RunSlot(FBattleState& State, const FBattleAction& Left, const FBattleAction& Right,
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
}

// Camuflagem barra o que a esquiva não barra: MAGIA. É esse o motivo de ela
// existir ao lado de Esquivar — se barrasse só o físico, seria um sinônimo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCamouflageBlocksMagicTooTest,
	"BattleSim.Concealment.Camouflage.BlocksMagicToo",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCamouflageBlocksMagicTooTest::RunTest(const FString& Parameters)
{
	{
		FBattleState State = MakeAdjacentDuel();
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Magia, EBattleDirection::Direita), Act(EActionType::Camuflar), Trace);
		TestEqual(TEXT("Camuflado não toma magia"), PetOnSide(State, 1).Health, 200);
	}
	{
		// Contraste que dá sentido à regra: esquivar NÃO barra magia (BTL-10).
		FBattleState State = MakeAdjacentDuel();
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Magia, EBattleDirection::Direita), Act(EActionType::Esquivar), Trace);
		TestTrue(TEXT("Esquiva continua sendo furada por magia"), PetOnSide(State, 1).Health < 200);
	}
	{
		FBattleState State = MakeAdjacentDuel();
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Atacar, EBattleDirection::Direita), Act(EActionType::Camuflar), Trace);
		TestEqual(TEXT("Camuflado também não toma físico"), PetOnSide(State, 1).Health, 200);
	}
	return true;
}

// Proteção total sem custo seria dominante: quem camufla todo slot nunca perde.
// O custo é TEMPO — no slot seguinte o pet está se revelando e não ataca.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCamouflageCostsNextActionTest,
	"BattleSim.Concealment.Camouflage.CostsNextAction",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCamouflageCostsNextActionTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeAdjacentDuel();
	TArray<FBattleEvent> Trace;

	RunSlot(State, Act(EActionType::Aguardar), Act(EActionType::Camuflar), Trace, 0);

	const int32 VidaAntes = PetOnSide(State, 0).Health;
	RunSlot(State, Act(EActionType::Aguardar), Act(EActionType::Atacar, EBattleDirection::Esquerda), Trace, 1);

	TestEqual(TEXT("Quem acabou de se revelar não fere"), PetOnSide(State, 0).Health, VidaAntes);
	return true;
}

// Voar é risco/recompensa, não escudo: escapa do físico e do chão, e é
// justamente por estar exposto no céu que a magia dói MAIS.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingTradesPhysicalForMagicTest,
	"BattleSim.Concealment.Flying.TradesPhysicalForMagic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFlyingTradesPhysicalForMagicTest::RunTest(const FString& Parameters)
{
	{
		FBattleState State = MakeAdjacentDuel();
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Atacar, EBattleDirection::Direita), Act(EActionType::Voar), Trace);
		TestEqual(TEXT("Físico não alcança quem voa"), PetOnSide(State, 1).Health, 200);
	}

	int32 DanoVoando = 0;
	{
		FBattleState State = MakeAdjacentDuel();
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Magia, EBattleDirection::Direita), Act(EActionType::Voar), Trace);
		DanoVoando = 200 - PetOnSide(State, 1).Health;
		TestTrue(TEXT("Magia alcança quem voa"), DanoVoando > 0);
	}
	{
		FBattleState State = MakeAdjacentDuel();
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Magia, EBattleDirection::Direita), Act(EActionType::Aguardar), Trace);
		const int32 DanoNoChao = 200 - PetOnSide(State, 1).Health;
		TestTrue(TEXT("No céu a magia dói mais que no chão"), DanoVoando > DanoNoChao);
	}
	return true;
}

// Voar e submergir tiram o pet do CHÃO, então a casa de dano deixa de tocá-lo.
// Camuflar não: quem se esconde continua pisando ali.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLeavingTheGroundAvoidsCellDamageTest,
	"BattleSim.Concealment.LeavingTheGround.AvoidsCellDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLeavingTheGroundAvoidsCellDamageTest::RunTest(const FString& Parameters)
{
	auto RodarSobreCasaDeDano = [](EActionType Postura) -> int32
	{
		FBattleState State = MakeAdjacentDuel();
		State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Damage);
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Aguardar), Act(Postura), Trace);
		return 200 - PetOnSide(State, 1).Health;
	};

	TestTrue(TEXT("No chão a casa fere"), RodarSobreCasaDeDano(EActionType::Aguardar) > 0);
	TestEqual(TEXT("Voando a casa não alcança"), RodarSobreCasaDeDano(EActionType::Voar), 0);
	TestTrue(TEXT("Camuflado continua pisando na casa"), RodarSobreCasaDeDano(EActionType::Camuflar) > 0);

	// SUBMERGIR SAIU DESTE TESTE, e não por ter parado de funcionar.
	//
	// Desde 2026-08-27 submergir exige uma casa de ÁGUA — e uma casa é água OU
	// é de dano, nunca as duas. A combinação que este teste media virou
	// impossível de montar, e testá-la seria testar um estado que o jogo não
	// produz. A imunidade ao chão continua valendo para quem submerge; ela é
	// que não tem mais como ser exercida numa casa de dano.
	{
		FBattleState State = MakeAdjacentDuel();
		State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Water);
		TArray<FBattleEvent> Trace;
		RunSlot(State, Act(EActionType::Aguardar), Act(EActionType::Submergir), Trace);
		TestEqual(TEXT("E submergir na água não fere ninguém"), PetOnSide(State, 1).Health, 200);
	}

	return true;
}

// Submergir é a proteção mais completa, e por isso a mais cara: no slot
// seguinte o pet não ataca NEM anda enquanto emerge.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUndergroundCostsMovementAndAttackTest,
	"BattleSim.Concealment.Underground.CostsMovementAndAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUndergroundCostsMovementAndAttackTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeAdjacentDuel();

	// Água na casa dele: desde 2026-08-27 submergir exige o terreno, e sem
	// isso este teste mediria a FALHA da postura, não o custo dela.
	State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Water);

	TArray<FBattleEvent> Trace;

	RunSlot(State, Act(EActionType::Magia, EBattleDirection::Direita), Act(EActionType::Submergir), Trace, 0);
	TestEqual(TEXT("Submerso não toma nem magia"), PetOnSide(State, 1).Health, 200);

	const uint8 ColunaAntes = PetOnSide(State, 1).Column;
	const int32 VidaDoOutro = PetOnSide(State, 0).Health;

	RunSlot(State, Act(EActionType::Aguardar), Act(EActionType::Mover, EBattleDirection::Cima), Trace, 1);
	TestEqual(TEXT("Emergindo não anda"), PetOnSide(State, 1).Column, ColunaAntes);
	TestEqual(TEXT("Emergindo não fere"), PetOnSide(State, 0).Health, VidaDoOutro);
	return true;
}
