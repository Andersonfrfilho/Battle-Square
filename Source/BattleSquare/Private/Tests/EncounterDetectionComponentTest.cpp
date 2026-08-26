// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterDetectionComponent.h"
#include "World/WorldEncounterActor.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	UWorld* CreateDetectionTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyDetectionTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	AActor* SpawnPawnAt(UWorld* World, const FVector& Location)
	{
		AActor* Pawn = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Pawn);
		Pawn->SetRootComponent(Root);
		Root->RegisterComponent();
		Pawn->SetActorLocation(Location);
		return Pawn;
	}

	UEncounterDetectionComponent* AttachDetection(AActor* Pawn)
	{
		UEncounterDetectionComponent* Detection = NewObject<UEncounterDetectionComponent>(Pawn);
		Detection->RegisterComponent();
		return Detection;
	}

	AWorldEncounterActor* SpawnEncounterAt(UWorld* World, const FVector& Location, float RadiusUnits, FName CatalogId)
	{
		AWorldEncounterActor* Encounter = World->SpawnActor<AWorldEncounterActor>();
		Encounter->SetActorLocation(Location);
		Encounter->EncounterRadiusUnits = RadiusUnits;
		Encounter->CatalogId = CatalogId;
		return Encounter;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectionComponentFiresOnceTest,
	"BattleSquare.World.EncounterDetectionComponent.FiresExactlyOnceOnEnteringRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectionComponentFiresOnceTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateDetectionTestWorld();
	AActor* Pawn = SpawnPawnAt(World, FVector(100.0, 0.0, 0.0));
	UEncounterDetectionComponent* Detection = AttachDetection(Pawn);
	AWorldEncounterActor* Encounter = SpawnEncounterAt(World, FVector(150.0, 0.0, 0.0), 300.0f, TEXT("Pet_A"));

	int32 BroadcastCount = 0;
	Detection->OnEncounterTriggered.AddLambda([&BroadcastCount](AWorldEncounterActor*) { ++BroadcastCount; });

	const TArray<AWorldEncounterActor*> EncounterActors = { Encounter };
	TestEqual(TEXT("primeira avaliação dispara o encontro"), Detection->EvaluateAndTrigger(EncounterActors), Encounter);
	Detection->EvaluateAndTrigger(EncounterActors);
	Detection->EvaluateAndTrigger(EncounterActors);

	TestEqual(TEXT("o delegate dispara exatamente uma vez"), BroadcastCount, 1);
	TestFalse(TEXT("o disparo desliga a detecção"), Detection->IsDetectionEnabled());

	DestroyDetectionTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectionComponentDisabledDoesNotFireTest,
	"BattleSquare.World.EncounterDetectionComponent.DisabledDoesNotFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectionComponentDisabledDoesNotFireTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateDetectionTestWorld();
	AActor* Pawn = SpawnPawnAt(World, FVector::ZeroVector);
	UEncounterDetectionComponent* Detection = AttachDetection(Pawn);
	AWorldEncounterActor* Encounter = SpawnEncounterAt(World, FVector(10.0, 0.0, 0.0), 300.0f, TEXT("Pet_A"));

	int32 BroadcastCount = 0;
	Detection->OnEncounterTriggered.AddLambda([&BroadcastCount](AWorldEncounterActor*) { ++BroadcastCount; });
	Detection->SetDetectionEnabled(false);

	TestNull(TEXT("detecção desligada não dispara mesmo em cima do pet"),
		Detection->EvaluateAndTrigger({ Encounter }));
	TestEqual(TEXT("nenhum broadcast enquanto desligada"), BroadcastCount, 0);

	DestroyDetectionTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectionComponentReenableAfterResolvedTest,
	"BattleSquare.World.EncounterDetectionComponent.ReenablingAfterResolvedDoesNotFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectionComponentReenableAfterResolvedTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateDetectionTestWorld();
	AActor* Pawn = SpawnPawnAt(World, FVector::ZeroVector);
	UEncounterDetectionComponent* Detection = AttachDetection(Pawn);
	AWorldEncounterActor* Encounter = SpawnEncounterAt(World, FVector(10.0, 0.0, 0.0), 300.0f, TEXT("Pet_A"));

	const TArray<AWorldEncounterActor*> EncounterActors = { Encounter };
	Detection->EvaluateAndTrigger(EncounterActors);

	// É esta a ordem que P1/critério 4 exige: marcar resolvido ANTES de religar.
	Encounter->MarkResolved();
	Detection->SetDetectionEnabled(true);

	int32 BroadcastCount = 0;
	Detection->OnEncounterTriggered.AddLambda([&BroadcastCount](AWorldEncounterActor*) { ++BroadcastCount; });

	TestNull(TEXT("pawn parado em cima do pet resolvido não dispara"), Detection->EvaluateAndTrigger(EncounterActors));
	TestEqual(TEXT("nenhum broadcast após religar sobre um encontro resolvido"), BroadcastCount, 0);
	TestTrue(TEXT("a detecção continua ligada para os próximos encontros"), Detection->IsDetectionEnabled());

	DestroyDetectionTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectionComponentNullEntriesAreIgnoredTest,
	"BattleSquare.World.EncounterDetectionComponent.NullEntriesAreIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectionComponentNullEntriesAreIgnoredTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateDetectionTestWorld();
	// Pawn na origem: um FEncounterCandidate default (raio 0, origem) faria
	// distância 0 <= raio 0 e dispararia. O nulo precisa sair antes disso.
	AActor* Pawn = SpawnPawnAt(World, FVector::ZeroVector);
	UEncounterDetectionComponent* Detection = AttachDetection(Pawn);

	TestNull(TEXT("entrada nula não vira encontro fantasma na origem"),
		Detection->EvaluateAndTrigger({ nullptr }));
	TestTrue(TEXT("a detecção segue ligada"), Detection->IsDetectionEnabled());

	DestroyDetectionTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectionComponentGathersFromWorldTest,
	"BattleSquare.World.EncounterDetectionComponent.GathersEncountersFromWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectionComponentGathersFromWorldTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateDetectionTestWorld();
	AActor* Pawn = SpawnPawnAt(World, FVector::ZeroVector);
	UEncounterDetectionComponent* Detection = AttachDetection(Pawn);

	SpawnEncounterAt(World, FVector(1000.0, 0.0, 0.0), 300.0f, TEXT("Pet_A"));
	SpawnEncounterAt(World, FVector(2000.0, 0.0, 0.0), 300.0f, TEXT("Pet_B"));

	TestEqual(TEXT("a coleta encontra os dois atores de encontro do mundo"),
		Detection->GatherEncounterActorsFromWorld().Num(), 2);

	DestroyDetectionTestWorld(World);
	return true;
}
