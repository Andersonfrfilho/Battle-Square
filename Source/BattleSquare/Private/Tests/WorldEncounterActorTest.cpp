// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldEncounterActor.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	UWorld* CreateEncounterActorTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyEncounterActorTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterActorCandidateReflectsActorTest,
	"BattleSquare.World.WorldEncounterActor.CandidateReflectsActorState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterActorCandidateReflectsActorTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateEncounterActorTestWorld();

	AWorldEncounterActor* Encounter = World->SpawnActor<AWorldEncounterActor>();
	Encounter->SetActorLocation(FVector(1200.0, -400.0, 90.0));
	Encounter->CatalogId = TEXT("Pet_DoMundo");
	Encounter->EncounterRadiusUnits = 450.0f;

	const FEncounterCandidate Candidate = Encounter->MakeEncounterCandidate();

	TestEqual(TEXT("posição do candidato vem do ator"), Candidate.WorldLocation, FVector(1200.0, -400.0, 90.0));
	TestEqual(TEXT("raio do candidato vem do ator"), Candidate.EncounterRadiusUnits, 450.0f);
	TestEqual(TEXT("CatalogId do candidato vem do ator"), Candidate.CatalogId, FName(TEXT("Pet_DoMundo")));
	TestFalse(TEXT("encontro nasce não resolvido"), Candidate.bIsResolved);

	DestroyEncounterActorTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterActorMarkResolvedTest,
	"BattleSquare.World.WorldEncounterActor.MarkResolvedPropagatesToCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterActorMarkResolvedTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateEncounterActorTestWorld();

	AWorldEncounterActor* Encounter = World->SpawnActor<AWorldEncounterActor>();
	Encounter->CatalogId = TEXT("Pet_DoMundo");
	Encounter->MarkResolved();

	TestTrue(TEXT("MarkResolved aparece no candidato colhido"), Encounter->MakeEncounterCandidate().bIsResolved);

	DestroyEncounterActorTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterActorDoesNotTickTest,
	"BattleSquare.World.WorldEncounterActor.DoesNotTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterActorDoesNotTickTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateEncounterActorTestWorld();

	AWorldEncounterActor* Encounter = World->SpawnActor<AWorldEncounterActor>();

	// DP-enc-01: o ator é dado posicionado. Um Tick aqui seria comportamento
	// entrando por uma porta que o design fechou de propósito.
	TestFalse(TEXT("o ator de encontro nunca tica"), Encounter->PrimaryActorTick.bCanEverTick);

	DestroyEncounterActorTestWorld(World);
	return true;
}
