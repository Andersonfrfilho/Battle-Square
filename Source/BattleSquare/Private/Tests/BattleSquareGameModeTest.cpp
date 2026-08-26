// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleSquareGameMode.h"
#include "Net/BattleSquarePlayerController.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	UWorld* CreateGameModeTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyGameModeTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	TArray<FPetPresentationInfo> MakeTwoPetPresentations()
	{
		FPetPresentationInfo Side0;
		Side0.PetId = 1;
		Side0.Name = TEXT("Side0Pet");
		FPetPresentationInfo Side1;
		Side1.PetId = 2;
		Side1.Name = TEXT("Side1Pet");
		return { Side0, Side1 };
	}

	FBattleState MakeTwoPetState()
	{
		FBattleState State;
		FPetState Side0Pet;
		Side0Pet.PetId = 1; Side0Pet.Side = 0; Side0Pet.Column = 1; Side0Pet.Row = 1;
		Side0Pet.Health = 50; Side0Pet.MaxHealth = 50; Side0Pet.Attack = 10; Side0Pet.Defense = 5;
		FPetState Side1Pet;
		Side1Pet.PetId = 2; Side1Pet.Side = 1; Side1Pet.Column = 2; Side1Pet.Row = 1;
		Side1Pet.Health = 50; Side1Pet.MaxHealth = 50; Side1Pet.Attack = 10; Side1Pet.Defense = 5;
		State.Pets.Add(Side0Pet);
		State.Pets.Add(Side1Pet);
		return State;
	}
}

// T7: ABattleSquareGameMode spawna sem crash, e Logout marca o lado do
// controller que saiu como desconectado no registro de sala.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleSquareGameModeLogoutMarksDisconnectedTest,
	"BattleSquare.Net.GameMode.LogoutMarksDisconnected",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleSquareGameModeLogoutMarksDisconnectedTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateGameModeTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	ABattleSquareGameMode* GameMode = World->SpawnActor<ABattleSquareGameMode>();
	if (!TestNotNull(TEXT("ABattleSquareGameMode spawna sem crash"), GameMode))
	{
		DestroyGameModeTestWorld(World);
		return false;
	}
	// Não depende de BeginPlay disparar sozinho no mundo de teste — ver
	// design.md/EnsureRoomRegistry.
	GameMode->EnsureRoomRegistry();
	TestNotNull(TEXT("RoomRegistry criado em BeginPlay"), GameMode->RoomRegistry.Get());

	FGuid CreatorSecret;
	const FString Code = GameMode->RoomRegistry->CreateRoom(CreatorSecret);

	ABattleSquarePlayerController* Controller = World->SpawnActor<ABattleSquarePlayerController>();
	GameMode->RegisterControllerForRoom(Code, /*Side=*/0, Controller);

	GameMode->Logout(Controller);

	const FBattleRoomState* Room = GameMode->RoomRegistry->GetRoomState(Code);
	if (TestNotNull(TEXT("Sala ainda existe após Logout"), Room))
	{
		TestFalse(TEXT("Side0 marcado como desconectado pelo Logout"), Room->Side0.bConnected);
	}

	DestroyGameModeTestWorld(World);
	return true;
}

// T8: AssembleMatchForRoom monta ABattleArena + UBattleTurnCoordinator
// reais para uma sala, a partir de um FBattleState já pronto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleSquareGameModeAssembleMatchTest,
	"BattleSquare.Net.GameMode.AssembleMatchForRoomBuildsRealArena",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleSquareGameModeAssembleMatchTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateGameModeTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	ABattleSquareGameMode* GameMode = World->SpawnActor<ABattleSquareGameMode>();
	if (!TestNotNull(TEXT("ABattleSquareGameMode spawna sem crash"), GameMode))
	{
		DestroyGameModeTestWorld(World);
		return false;
	}
	// Não depende de BeginPlay disparar sozinho no mundo de teste — ver
	// design.md/EnsureRoomRegistry.
	GameMode->EnsureRoomRegistry();

	const FString Code = TEXT("TESTE1");
	GameMode->AssembleMatchForRoom(Code, MakeTwoPetState(), MakeTwoPetPresentations());

	const ABattleSquareGameMode::FActiveMatch* Match = GameMode->GetActiveMatch(Code);
	if (TestNotNull(TEXT("Partida montada para a sala"), Match))
	{
		TestNotNull(TEXT("Arena real instanciada"), Match->Arena.Get());
		TestNotNull(TEXT("Coordenador real instanciado"), Match->Coordinator.Get());
		if (Match->Arena)
		{
			TestEqual(TEXT("Arena tem os 2 pets do estado inicial"), Match->Arena->GetCurrentState().Pets.Num(), 2);
		}
	}

	DestroyGameModeTestWorld(World);
	return true;
}

// T9: abandono real, encadeado do registro ao coordenador da SALA
// CORRETA — não afeta o coordenador de outra sala ativa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleSquareGameModeAbandonmentReachesCorrectCoordinatorTest,
	"BattleSquare.Net.GameMode.AbandonmentReachesRealCoordinator",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleSquareGameModeAbandonmentReachesCorrectCoordinatorTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateGameModeTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	ABattleSquareGameMode* GameMode = World->SpawnActor<ABattleSquareGameMode>();
	if (!TestNotNull(TEXT("ABattleSquareGameMode spawna sem crash"), GameMode))
	{
		DestroyGameModeTestWorld(World);
		return false;
	}
	// Não depende de BeginPlay disparar sozinho no mundo de teste — ver
	// design.md/EnsureRoomRegistry.
	GameMode->EnsureRoomRegistry();

	// Duas salas ativas simultaneamente.
	GameMode->AssembleMatchForRoom(TEXT("SALAA"), MakeTwoPetState(), MakeTwoPetPresentations());
	GameMode->AssembleMatchForRoom(TEXT("SALAB"), MakeTwoPetState(), MakeTwoPetPresentations());

	bool bRoomAAbandoned = false;
	bool bRoomBAbandoned = false;
	GameMode->GetActiveMatch(TEXT("SALAA"))->Coordinator->OnAbandonment.AddLambda(
		[&bRoomAAbandoned](const FBattleEvent&) { bRoomAAbandoned = true; });
	GameMode->GetActiveMatch(TEXT("SALAB"))->Coordinator->OnAbandonment.AddLambda(
		[&bRoomBAbandoned](const FBattleEvent&) { bRoomBAbandoned = true; });

	// Simula o registro declarando abandono na sala A — dispara via o
	// caminho real (RoomRegistry->OnRoomAbandoned -> HandleRoomAbandoned
	// -> UBattleTurnCoordinator::DeclareAbandonment DA SALA CERTA).
	GameMode->RoomRegistry->OnRoomAbandoned.Broadcast(TEXT("SALAA"), /*PresentSide=*/0);

	TestTrue(TEXT("Coordenador da sala A recebeu o abandono"), bRoomAAbandoned);
	TestFalse(TEXT("Coordenador da sala B NÃO foi tocado"), bRoomBAbandoned);

	DestroyGameModeTestWorld(World);
	return true;
}
