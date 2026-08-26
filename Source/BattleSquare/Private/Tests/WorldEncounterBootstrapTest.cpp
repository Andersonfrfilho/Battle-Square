// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleSquareGameMode.h"
#include "World/WorldEncounterFlow.h"
#include "World/WorldExplorerCharacter.h"
#include "World/EncounterDetectionComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	const TCHAR* const FixtureMirrorKeyHex = TEXT("ad2f50e7781fcca5148299d249825f117ff78cc0b1758de30ce9832f01918e39");
	const TCHAR* const FixtureMirrorPem =
		TEXT("-----BEGIN PUBLIC KEY-----\n")
		TEXT("MCowBQYDK2VwAyEASmCzzcPySYHgKJgbH2uuAjtP4gXGRl2jP4ynBxOF2K0=\n")
		TEXT("-----END PUBLIC KEY-----\n");

	UWorld* CreateBootstrapTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyBootstrapTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	ABattleSquareGameMode* SpawnConfiguredGameMode(UWorld* World)
	{
		ABattleSquareGameMode* GameMode = World->SpawnActor<ABattleSquareGameMode>();
		GameMode->WorldEncounterMirrorPath =
			FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PetMirrorFixture"), TEXT("pets-mirror.sqlite"));
		GameMode->WorldEncounterMirrorKeyHex = FixtureMirrorKeyHex;
		GameMode->WorldEncounterMirrorPublicKeyPem = FixtureMirrorPem;
		// Slot dedicado: nunca poluir o slot de produção em teste.
		GameMode->PetCollectionSlotName = TEXT("WorldEncounterBootstrapTestCollection");
		return GameMode;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterBootstrapWiresFlowTest,
	"BattleSquare.World.WorldEncounterBootstrap.WiresFlowWhenPawnAndMirrorExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterBootstrapWiresFlowTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateBootstrapTestWorld();
	ABattleSquareGameMode* GameMode = SpawnConfiguredGameMode(World);
	World->SpawnActor<AWorldExplorerCharacter>();

	const FString Problem = GameMode->SetUpWorldEncounterFlow();

	TestEqual(TEXT("com pawn e espelho, a fiação sucede sem motivo de falha"), Problem, FString());
	TestNotNull(TEXT("o flow de encontros passa a existir"), GameMode->WorldEncounterFlow.Get());
	TestNotNull(TEXT("o flow tem serviço de transição"),
		GameMode->WorldEncounterFlow ? GameMode->WorldEncounterFlow->GetTransitionService() : nullptr);

	DestroyBootstrapTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterBootstrapWithoutPawnTest,
	"BattleSquare.World.WorldEncounterBootstrap.NoPawnIsAReasonNotACrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterBootstrapWithoutPawnTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateBootstrapTestWorld();
	ABattleSquareGameMode* GameMode = SpawnConfiguredGameMode(World);

	// Nível sem pawn de exploração é legítimo (o mundo de batalha pura de
	// M1–M4 é assim) — precisa devolver MOTIVO, não travar nem crashar.
	const FString Problem = GameMode->SetUpWorldEncounterFlow();

	TestFalse(TEXT("sem pawn, devolve um motivo legível"), Problem.IsEmpty());
	TestNull(TEXT("e não cria flow nenhum"), GameMode->WorldEncounterFlow.Get());

	DestroyBootstrapTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterBootstrapWithoutMirrorTest,
	"BattleSquare.World.WorldEncounterBootstrap.MissingMirrorIsAReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterBootstrapWithoutMirrorTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateBootstrapTestWorld();
	ABattleSquareGameMode* GameMode = SpawnConfiguredGameMode(World);
	GameMode->WorldEncounterMirrorPath.Reset();
	World->SpawnActor<AWorldExplorerCharacter>();

	const FString Problem = GameMode->SetUpWorldEncounterFlow();

	TestFalse(TEXT("sem espelho configurado, devolve motivo em vez de seguir com dado vazio"), Problem.IsEmpty());
	TestNull(TEXT("e não cria flow nenhum"), GameMode->WorldEncounterFlow.Get());

	DestroyBootstrapTestWorld(World);
	return true;
}
