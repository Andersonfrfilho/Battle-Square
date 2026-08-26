// Copyright 2026 Anderson. All Rights Reserved.

#include "World/DebugRouteMoverComponent.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// AActor puro spawnado via SpawnActor<AActor>() não tem RootComponent —
	// SetActorLocation/GetActorLocation viram no-op silencioso sem uma raiz
	// de cena. O pawn de debug real (T4) já vem com um root por construção;
	// o teste precisa dar um explicitamente.
	AActor* SpawnMovableTestActor(UWorld* World)
	{
		AActor* Owner = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Owner);
		Owner->SetRootComponent(Root);
		Root->RegisterComponent();
		return Owner;
	}

	UWorld* CreateDebugRouteTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyDebugRouteTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	// TickComponent nunca lê tempo real — chamado manualmente com DeltaTime
	// fixo, mesma disciplina de "tempo sempre injetado" de UBattleTurnCoordinator.
	void TickManually(UDebugRouteMoverComponent* Mover, float DeltaTime)
	{
		Mover->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All, nullptr);
	}
}

// T1: rota de 3 waypoints — avança até o primeiro, depois o segundo, e
// para exatamente no último sem ultrapassar (sem loop).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDebugRouteMoverComponentFollowsRouteDeterministicallyTest,
	"BattleSquare.World.DebugRouteMoverComponent.FollowsRouteDeterministically",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDebugRouteMoverComponentFollowsRouteDeterministicallyTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateDebugRouteTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	AActor* Owner = SpawnMovableTestActor(World);
	UDebugRouteMoverComponent* Mover = NewObject<UDebugRouteMoverComponent>(Owner);
	Mover->RegisterComponent();
	Mover->Waypoints = { FVector(100.0, 0.0, 0.0), FVector(100.0, 100.0, 0.0), FVector(0.0, 100.0, 0.0) };
	Mover->SpeedUnitsPerSecond = 100.0f;
	Mover->WaypointReachedToleranceUnits = 1.0f;

	TestEqual(TEXT("Começa mirando o waypoint 0"), Mover->GetCurrentWaypointIndex(), 0);

	// 1s a 100 u/s chega exatamente ao primeiro waypoint (100,0,0).
	TickManually(Mover, 1.0f);
	TestEqual(TEXT("Alcançou o waypoint 0, mira o waypoint 1"), Mover->GetCurrentWaypointIndex(), 1);

	// Mais 1s chega ao segundo waypoint (100,100,0).
	TickManually(Mover, 1.0f);
	TestEqual(TEXT("Alcançou o waypoint 1, mira o waypoint 2"), Mover->GetCurrentWaypointIndex(), 2);

	// Mais 1s chega ao último waypoint — rota termina, sem loop.
	TickManually(Mover, 1.0f);
	TestTrue(TEXT("Rota terminada após o último waypoint"), Mover->HasFinishedRoute());

	// Tick adicional não deve mover o Owner nem sair do estado terminado.
	const FVector LocationAfterFinished = Owner->GetActorLocation();
	TickManually(Mover, 5.0f);
	TestEqual(TEXT("Posição não muda depois de terminar a rota"), Owner->GetActorLocation(), LocationAfterFinished);
	TestTrue(TEXT("Continua terminada, sem voltar a mover"), Mover->HasFinishedRoute());

	DestroyDebugRouteTestWorld(World);
	return true;
}

// T1: rota vazia não crasha e já começa terminada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDebugRouteMoverComponentEmptyRouteFinishesImmediatelyTest,
	"BattleSquare.World.DebugRouteMoverComponent.EmptyRouteFinishesImmediately",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDebugRouteMoverComponentEmptyRouteFinishesImmediatelyTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateDebugRouteTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	AActor* Owner = SpawnMovableTestActor(World);
	UDebugRouteMoverComponent* Mover = NewObject<UDebugRouteMoverComponent>(Owner);
	Mover->RegisterComponent();

	TestTrue(TEXT("Rota vazia já começa terminada"), Mover->HasFinishedRoute());
	TickManually(Mover, 1.0f);
	TestTrue(TEXT("Continua terminada após tick, sem crash"), Mover->HasFinishedRoute());

	DestroyDebugRouteTestWorld(World);
	return true;
}
