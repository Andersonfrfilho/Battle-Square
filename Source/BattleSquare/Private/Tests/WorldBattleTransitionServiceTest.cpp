// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldBattleTransitionService.h"
#include "World/EncounterDetectionComponent.h"
#include "World/WorldEncounterActor.h"
#include "Battle/BattleArena.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	UWorld* CreateTransitionTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyTransitionTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	struct FTransitionFixture
	{
		UWorld* World = nullptr;
		AActor* Pawn = nullptr;
		UEncounterDetectionComponent* Detection = nullptr;
		AWorldEncounterActor* Encounter = nullptr;
		UWorldBattleTransitionService* Service = nullptr;
	};

	FTransitionFixture MakeTransitionFixture(const FTransform& PawnTransform)
	{
		FTransitionFixture Fixture;
		Fixture.World = CreateTransitionTestWorld();

		Fixture.Pawn = Fixture.World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Fixture.Pawn);
		Fixture.Pawn->SetRootComponent(Root);
		Root->RegisterComponent();
		Fixture.Pawn->SetActorTransform(PawnTransform);

		Fixture.Detection = NewObject<UEncounterDetectionComponent>(Fixture.Pawn);
		Fixture.Detection->RegisterComponent();

		Fixture.Encounter = Fixture.World->SpawnActor<AWorldEncounterActor>();
		Fixture.Encounter->CatalogId = TEXT("Pet_DoMundo");

		Fixture.Service = NewObject<UWorldBattleTransitionService>();
		return Fixture;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldBattleTransitionBeginsAwayFromWorldTest,
	"BattleSquare.World.WorldBattleTransitionService.ArenaSpawnsAwayAndDetectionStops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldBattleTransitionBeginsAwayFromWorldTest::RunTest(const FString& Parameters)
{
	FTransitionFixture Fixture = MakeTransitionFixture(FTransform(FRotator(0.0, 45.0, 0.0), FVector(1200.0, -800.0, 300.0)));

	ABattleArena* Arena = Fixture.Service->BeginTransition(Fixture.Pawn, Fixture.Detection,
		Fixture.Encounter, ABattleArena::StaticClass());

	TestNotNull(TEXT("a transição spawna uma arena"), Arena);
	TestTrue(TEXT("a transição fica ativa"), Fixture.Service->IsTransitionActive());
	TestFalse(TEXT("a detecção é desligada durante a batalha"), Fixture.Detection->IsDetectionEnabled());
	TestEqual(TEXT("a arena nasce no deslocamento nomeado, fora do mundo jogável"),
		Arena->GetActorLocation().X, WorldBattleTransition::ArenaWorldOffsetUnits);
	TestFalse(TEXT("o encontro ainda não está resolvido enquanto a batalha corre"),
		Fixture.Encounter->bIsResolved);

	DestroyTransitionTestWorld(Fixture.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldBattleTransitionRestoresPawnTest,
	"BattleSquare.World.WorldBattleTransitionService.RestoresPawnTransformExactly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldBattleTransitionRestoresPawnTest::RunTest(const FString& Parameters)
{
	const FTransform OriginalTransform(FRotator(0.0, 137.0, 0.0), FVector(-3600.0, 900.0, 300.0));
	FTransitionFixture Fixture = MakeTransitionFixture(OriginalTransform);

	ABattleArena* Arena = Fixture.Service->BeginTransition(Fixture.Pawn, Fixture.Detection,
		Fixture.Encounter, ABattleArena::StaticClass());

	// O pawn é deslocado durante a batalha; o ponto do teste é que a volta o
	// devolve exatamente onde estava, não "perto".
	Fixture.Pawn->SetActorTransform(FTransform(FRotator::ZeroRotator, FVector(99999.0, 99999.0, 99999.0)));

	Arena->OnBattleFinished.Broadcast();

	TestEqual(TEXT("posição restaurada é idêntica à capturada"),
		Fixture.Pawn->GetActorLocation(), OriginalTransform.GetLocation());
	TestEqual(TEXT("rotação restaurada é idêntica à capturada"),
		Fixture.Pawn->GetActorRotation().Yaw, OriginalTransform.Rotator().Yaw);
	TestFalse(TEXT("a transição termina"), Fixture.Service->IsTransitionActive());

	DestroyTransitionTestWorld(Fixture.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldBattleTransitionResolvesBeforeReenablingTest,
	"BattleSquare.World.WorldBattleTransitionService.MarksResolvedBeforeReenablingDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldBattleTransitionResolvesBeforeReenablingTest::RunTest(const FString& Parameters)
{
	// Pawn em cima do pet: se a ordem de DP-enc-03 estiver invertida, a
	// próxima avaliação dispara um segundo encontro pelo mesmo pet.
	FTransitionFixture Fixture = MakeTransitionFixture(FTransform(FVector::ZeroVector));
	Fixture.Encounter->SetActorLocation(FVector::ZeroVector);
	Fixture.Encounter->EncounterRadiusUnits = 500.0f;

	ABattleArena* Arena = Fixture.Service->BeginTransition(Fixture.Pawn, Fixture.Detection,
		Fixture.Encounter, ABattleArena::StaticClass());
	Arena->OnBattleFinished.Broadcast();

	TestTrue(TEXT("o encontro fica marcado resolvido"), Fixture.Encounter->bIsResolved);
	TestTrue(TEXT("a detecção volta ligada para os próximos encontros"), Fixture.Detection->IsDetectionEnabled());

	int32 BroadcastCount = 0;
	Fixture.Detection->OnEncounterTriggered.AddLambda([&BroadcastCount](AWorldEncounterActor*) { ++BroadcastCount; });

	TestNull(TEXT("voltar parado em cima do pet derrotado não dispara segundo encontro"),
		Fixture.Detection->EvaluateAndTrigger({ Fixture.Encounter }));
	TestEqual(TEXT("nenhum broadcast na volta"), BroadcastCount, 0);

	DestroyTransitionTestWorld(Fixture.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldBattleTransitionDestroysArenaOnlyAtEndTest,
	"BattleSquare.World.WorldBattleTransitionService.DestroysArenaOnlyAfterBattleFinished",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldBattleTransitionDestroysArenaOnlyAtEndTest::RunTest(const FString& Parameters)
{
	FTransitionFixture Fixture = MakeTransitionFixture(FTransform(FVector(500.0, 0.0, 0.0)));

	ABattleArena* Arena = Fixture.Service->BeginTransition(Fixture.Pawn, Fixture.Detection,
		Fixture.Encounter, ABattleArena::StaticClass());

	TestFalse(TEXT("a arena sobrevive enquanto a batalha corre"), Arena->IsActorBeingDestroyed());
	TestEqual(TEXT("o serviço aponta para a arena ativa"), Fixture.Service->GetActiveArena(), Arena);

	Arena->OnBattleFinished.Broadcast();

	TestTrue(TEXT("a arena é destruída no fim"), Arena->IsActorBeingDestroyed());
	TestNull(TEXT("o serviço solta a arena"), Fixture.Service->GetActiveArena());

	DestroyTransitionTestWorld(Fixture.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldBattleTransitionRejectsReentryTest,
	"BattleSquare.World.WorldBattleTransitionService.RejectsSecondTransitionWhileActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldBattleTransitionRejectsReentryTest::RunTest(const FString& Parameters)
{
	FTransitionFixture Fixture = MakeTransitionFixture(FTransform(FVector(500.0, 0.0, 0.0)));

	Fixture.Service->BeginTransition(Fixture.Pawn, Fixture.Detection, Fixture.Encounter, ABattleArena::StaticClass());

	TestNull(TEXT("uma segunda transição durante a batalha é recusada"),
		Fixture.Service->BeginTransition(Fixture.Pawn, Fixture.Detection, Fixture.Encounter, ABattleArena::StaticClass()));

	DestroyTransitionTestWorld(Fixture.World);
	return true;
}
