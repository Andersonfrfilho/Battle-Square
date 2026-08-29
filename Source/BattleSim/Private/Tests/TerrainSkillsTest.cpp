// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeDuelForTerrain()
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

	FBattleAction TerrainAct(EActionType Type, EBattleDirection Direction = EBattleDirection::Nenhuma)
	{
		FBattleAction Action;
		Action.Type = Type;
		Action.Direction = Direction;
		return Action;
	}

	const FPetState& TerrainPetOnSide(const FBattleState& State, uint8 Side)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side) { return Pet; }
		}
		return State.Pets[0];
	}

	bool TerrainHasEvent(const TArray<FBattleEvent>& Trace, EBattleEventType Type)
	{
		for (const FBattleEvent& Event : Trace)
		{
			if (Event.Type == Type) { return true; }
		}
		return false;
	}

	void RunTerrainSlot(FBattleState& State, const FBattleAction& Left, const FBattleAction& Right,
		TArray<FBattleEvent>& Trace)
	{
		BattlePhases::ApplyPostures(State, Left, Right, 0, Trace);
		BattlePhases::ApplyMovement(State, Left, Right, 0, Trace);
		BattlePhases::ApplyCombat(State, Left, Right, 0, Trace);
		BattlePhases::ApplyResolution(State, 0, Trace);
	}
}

// Submergir exige ÁGUA. Sem terreno, a skill funcionava em qualquer lugar — e
// "mergulhar no chão seco" contradiz o que a skill É, além de tornar o mapa
// irrelevante para a decisão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSubmergeRequiresWaterTest,
	"BattleSim.Terrain.Submerge.RequiresWater",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSubmergeRequiresWaterTest::RunTest(const FString& Parameters)
{
	// Em terra seca: a skill FALHA, e o pet toma o golpe.
	{
		FBattleState State = MakeDuelForTerrain();
		TArray<FBattleEvent> Trace;
		RunTerrainSlot(State, TerrainAct(EActionType::Atacar, EBattleDirection::Direita),
			TerrainAct(EActionType::Submergir), Trace);

		TestTrue(TEXT("Fora d'água a postura falha"),
			TerrainHasEvent(Trace, EBattleEventType::PosturaFalhou));
		TestTrue(TEXT("E o pet fica exposto"), TerrainPetOnSide(State, 1).Health < 200);
	}

	// Na água: funciona, e a imunidade vale.
	{
		FBattleState State = MakeDuelForTerrain();
		State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Water);

		TArray<FBattleEvent> Trace;
		RunTerrainSlot(State, TerrainAct(EActionType::Atacar, EBattleDirection::Direita),
			TerrainAct(EActionType::Submergir), Trace);

		TestFalse(TEXT("Na água a postura NÃO falha"),
			TerrainHasEvent(Trace, EBattleEventType::PosturaFalhou));
		TestEqual(TEXT("E o pet fica intocável"), TerrainPetOnSide(State, 1).Health, 200);
	}

	return true;
}

// As outras posturas NÃO dependem de terreno: só submergir foi condicionada, e
// condicionar as demais mudaria o combate inteiro sem ninguém ter pedido.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOtherPosturesIgnoreTerrainTest,
	"BattleSim.Terrain.OtherPostures.IgnoreTerrain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FOtherPosturesIgnoreTerrainTest::RunTest(const FString& Parameters)
{
	const EActionType Posturas[] = {
		EActionType::Defender, EActionType::Esquivar,
		EActionType::Camuflar, EActionType::Voar
	};

	for (EActionType Postura : Posturas)
	{
		FBattleState State = MakeDuelForTerrain();
		TArray<FBattleEvent> Trace;
		RunTerrainSlot(State, TerrainAct(EActionType::Aguardar), TerrainAct(Postura), Trace);

		TestFalse(TEXT("Postura sem exigência de terreno não falha em terra seca"),
			TerrainHasEvent(Trace, EBattleEventType::PosturaFalhou));
	}

	return true;
}
