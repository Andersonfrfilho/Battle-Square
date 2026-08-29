// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeTerrainDuel(uint8 EfeitoDoGolpe0)
	{
		FBattleState State;

		FPetState Atacante;
		Atacante.PetId = 1; Atacante.Side = 0;
		Atacante.Column = 1; Atacante.Row = 1;
		Atacante.Health = 500; Atacante.MaxHealth = 500;
		Atacante.Attack = 100; Atacante.Defense = 10; Atacante.Speed = 5;
		Atacante.MovePowers[0] = 100;
		Atacante.MoveTerrainEffects[0] = EfeitoDoGolpe0;

		FPetState Alvo = Atacante;
		Alvo.PetId = 2; Alvo.Side = 1; Alvo.Column = 2;
		Alvo.MovePowers[0] = 0;
		Alvo.MoveTerrainEffects[0] = 0;

		State.Pets.Add(Atacante);
		State.Pets.Add(Alvo);
		return State;
	}

	uint8 CasaDoAlvo(const FBattleState& State)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == 1)
			{
				return State.CellLayout[State.CellIndex(Pet.Column, Pet.Row)];
			}
		}
		return 0;
	}

	bool TemEvento(const TArray<FBattleEvent>& Trace, EBattleEventType Tipo)
	{
		for (const FBattleEvent& Evento : Trace)
		{
			if (Evento.Type == Tipo) { return true; }
		}
		return false;
	}
}

// O golpe DEIXA algo na casa que acertou. É o que fecha a cadeia que o usuário
// desenhou: um golpe de água alaga, e alagar é o que torna submergir possível
// onde antes não dava — o pet fabrica o terreno da própria skill.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoveChangesTerrainOnHitTest,
	"BattleSim.Moves.Terrain.ChangesOnHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoveChangesTerrainOnHitTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTerrainDuel(static_cast<uint8>(ECellProperty::Water));
	TArray<FBattleEvent> Trace;

	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("A casa do alvo virou água"),
		static_cast<int32>(CasaDoAlvo(State)), static_cast<int32>(ECellProperty::Water));
	TestTrue(TEXT("E a mudança foi anunciada no traço"),
		TemEvento(Trace, EBattleEventType::TerrenoMudou));

	return true;
}

// Golpe SEM efeito não mexe na casa — e "none" é o mesmo valor de casa neutra
// de propósito: golpe sem efeito não pode ser confundido com golpe que
// NEUTRALIZA a casa, que seria uma habilidade que ninguém cadastrou.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoveWithoutEffectLeavesTerrainAloneTest,
	"BattleSim.Moves.Terrain.WithoutEffectLeavesItAlone",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoveWithoutEffectLeavesTerrainAloneTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTerrainDuel(static_cast<uint8>(ECellProperty::None));

	// Casa do alvo começa como BÔNUS: se o golpe sem efeito a "neutralizasse",
	// o bônus sumiria sem ninguém ter pedido.
	State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Buff);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("A casa continua como bônus"),
		static_cast<int32>(CasaDoAlvo(State)), static_cast<int32>(ECellProperty::Buff));
	TestFalse(TEXT("E nada foi anunciado"),
		TemEvento(Trace, EBattleEventType::TerrenoMudou));

	return true;
}

// Casa BLOQUEADA nunca muda: ela é estrutura da arena, não superfície. Um golpe
// que abrisse passagem mudaria o tabuleiro DEPOIS de o movimento já ter sido
// resolvido naquele turno.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBlockedCellNeverChangesTest,
	"BattleSim.Moves.Terrain.BlockedCellNeverChanges",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBlockedCellNeverChangesTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTerrainDuel(static_cast<uint8>(ECellProperty::Water));
	State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("Casa bloqueada continua bloqueada"),
		static_cast<int32>(CasaDoAlvo(State)), static_cast<int32>(ECellProperty::Blocked));

	return true;
}

// A CADEIA COMPLETA, que é o motivo de tudo isto existir: alagar a casa faz
// submergir passar a funcionar onde antes falhava.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFloodingEnablesSubmergingTest,
	"BattleSim.Moves.Terrain.FloodingEnablesSubmerging",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFloodingEnablesSubmergingTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTerrainDuel(static_cast<uint8>(ECellProperty::Water));
	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;

	// Antes: terra seca, e submergir FALHA.
	BattlePhases::ApplyPostures(State, Aguardar,
		FBattleAction{ EActionType::Submergir, EBattleDirection::Nenhuma }, 0, Trace);
	TestTrue(TEXT("Em terra seca, submergir falha"),
		TemEvento(Trace, EBattleEventType::PosturaFalhou));

	// O golpe alaga a casa do alvo.
	Trace.Reset();
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	// Depois: mesma casa, e agora submergir vale.
	Trace.Reset();
	BattlePhases::ApplyPostures(State, Aguardar,
		FBattleAction{ EActionType::Submergir, EBattleDirection::Nenhuma }, 1, Trace);
	TestFalse(TEXT("Depois de alagada, submergir funciona"),
		TemEvento(Trace, EBattleEventType::PosturaFalhou));

	return true;
}
