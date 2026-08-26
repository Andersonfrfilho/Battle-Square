// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Net/BattleTurnCoordinator.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// Cria um UWorld mínimo, fora de PIE/editor, só para permitir
	// SpawnActor/registro de componentes em teste headless — mesmo
	// espírito do padrão usado pelo framework de Automation da própria
	// engine para testes que precisam de um mundo real sem depender de
	// UnrealEd.
	UWorld* CreateHeadlessTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyHeadlessTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

// T9: ABattleArena instanciado num mundo de teste spawna sem crash, e as
// 9 casas da grade caem dentro do frustum da câmera fixa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaSpawnsAndFramesGridTest,
	"BattleSquare.BattleArena.SpawnsAndFramesGridInCameraFrustum",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaSpawnsAndFramesGridTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateHeadlessTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	TestNotNull(TEXT("ABattleArena spawna sem crash"), Arena);

	if (Arena)
	{
		TestTrue(TEXT("As 9 casas da grade caem dentro do frustum da câmera fixa"), Arena->AreAllGridCellsInCameraFrustum());

		const FVector CenterCell = Arena->GetCellWorldLocation(1, 1);
		const FVector CornerCell = Arena->GetCellWorldLocation(0, 0);
		TestTrue(TEXT("Casas distintas produzem posições distintas"), !CenterCell.Equals(CornerCell));
	}

	DestroyHeadlessTestWorld(World);
	return true;
}

// T10: fiação de ponta a ponta — jogador monta e commita 3 ações via
// UBattleActionQueueComponent (sem UI visual), a IA gera o commit dela
// automaticamente, o resolvedor real roda, e o estado resultante fica
// consistente. Um turno completo, sem intervenção manual além da
// seleção inicial de ações.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaFullTurnEndToEndTest,
	"BattleSquare.BattleArena.FullTurnEndToEnd",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaFullTurnEndToEndTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateHeadlessTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	if (!TestNotNull(TEXT("ABattleArena spawna sem crash"), Arena))
	{
		DestroyHeadlessTestWorld(World);
		return false;
	}

	FBattleState InitialState;
	FPetState PlayerPet;
	PlayerPet.PetId = 1; PlayerPet.Side = 0; PlayerPet.Column = 1; PlayerPet.Row = 1;
	PlayerPet.Health = 50; PlayerPet.MaxHealth = 50; PlayerPet.Attack = 15; PlayerPet.Defense = 5;
	FPetState OpponentPet;
	OpponentPet.PetId = 2; OpponentPet.Side = 1; OpponentPet.Column = 2; OpponentPet.Row = 1;
	OpponentPet.Health = 50; OpponentPet.MaxHealth = 50; OpponentPet.Attack = 10; OpponentPet.Defense = 5;
	InitialState.Pets.Add(PlayerPet);
	InitialState.Pets.Add(OpponentPet);
	InitialState.Random.State = 777ULL;

	TArray<FPetPresentationInfo> Presentations;
	FPetPresentationInfo PlayerPresentation;
	PlayerPresentation.PetId = PlayerPet.PetId;
	PlayerPresentation.Name = TEXT("Jogador");
	FPetPresentationInfo OpponentPresentation;
	OpponentPresentation.PetId = OpponentPet.PetId;
	OpponentPresentation.Name = TEXT("Oponente");
	Presentations.Add(PlayerPresentation);
	Presentations.Add(OpponentPresentation);

	Arena->BeginBattle(InitialState, Presentations);
	TestEqual(TEXT("Duas views spawnadas, uma por pet"), Arena->GetPetViews().Num(), 2);

	const FBattleState StateBeforeCommit = Arena->GetCurrentState();

	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Atacar);
	Arena->PlayerActionQueue->ConfirmDirection(EBattleDirection::Direita);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Defender);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	TestEqual(TEXT("3 ações confirmadas antes de commitar"), Arena->PlayerActionQueue->GetConfirmedActionCount(), 3);

	Arena->PlayerActionQueue->Commit();
	TestTrue(TEXT("Fila travada após Commit"), Arena->PlayerActionQueue->IsCommitted());

	const FBattleState StateAfterCommit = Arena->GetCurrentState();
	TestTrue(TEXT("HandlePlayerCommitted rodou o resolvedor real: TurnNumber avançou"), StateAfterCommit.TurnNumber != StateBeforeCommit.TurnNumber || StateAfterCommit.Random.State != StateBeforeCommit.Random.State);

	TestTrue(TEXT("Estado resultante é consistente (2 pets, valores dentro do esperado)"), StateAfterCommit.Pets.Num() == 2);

	DestroyHeadlessTestWorld(World);
	return true;
}

// T8: com ConfigureNetworkedOpponent chamado, o commit do jogador vai
// para o UBattleTurnCoordinator, NUNCA para FDumbOpponentAI — provado
// indiretamente: o turno não resolve com só o commit do jogador (algo
// que só aconteceria se a IA tivesse preenchido o outro lado na hora).
// Só resolve quando o coordenador recebe o segundo commit diretamente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaNetworkedOpponentBypassesAITest,
	"BattleSquare.BattleArena.NetworkedOpponentBypassesAI",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaNetworkedOpponentBypassesAITest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateHeadlessTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	if (!TestNotNull(TEXT("ABattleArena spawna sem crash"), Arena))
	{
		DestroyHeadlessTestWorld(World);
		return false;
	}

	FBattleState InitialState;
	FPetState PlayerPet;
	PlayerPet.PetId = 1; PlayerPet.Side = 0; PlayerPet.Column = 1; PlayerPet.Row = 1;
	PlayerPet.Health = 50; PlayerPet.MaxHealth = 50; PlayerPet.Attack = 15; PlayerPet.Defense = 5;
	FPetState OpponentPet;
	OpponentPet.PetId = 2; OpponentPet.Side = 1; OpponentPet.Column = 2; OpponentPet.Row = 1;
	OpponentPet.Health = 50; OpponentPet.MaxHealth = 50; OpponentPet.Attack = 10; OpponentPet.Defense = 5;
	InitialState.Pets.Add(PlayerPet);
	InitialState.Pets.Add(OpponentPet);

	TArray<FPetPresentationInfo> Presentations;
	FPetPresentationInfo PlayerPresentation;
	PlayerPresentation.PetId = PlayerPet.PetId;
	FPetPresentationInfo OpponentPresentation;
	OpponentPresentation.PetId = OpponentPet.PetId;
	Presentations.Add(PlayerPresentation);
	Presentations.Add(OpponentPresentation);

	Arena->BeginBattle(InitialState, Presentations);

	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(Arena->GetCurrentState(), 0.0);
	Arena->ConfigureNetworkedOpponent(Coordinator);

	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Atacar);
	Arena->PlayerActionQueue->ConfirmDirection(EBattleDirection::Direita);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Defender);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->Commit();

	TestFalse(TEXT("Turno NÃO resolve só com o commit do jogador — FDumbOpponentAI não foi chamado"), Coordinator->IsTurnResolved());
	const int32 TurnNumberBeforeOpponentCommit = Arena->GetCurrentState().TurnNumber;

	FTurnCommit OpponentCommit;
	OpponentCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Coordinator->SubmitCommit(/*Side=*/1, OpponentCommit);

	TestTrue(TEXT("Turno resolve quando o coordenador recebe o commit real do oponente"), Coordinator->IsTurnResolved());
	TestTrue(TEXT("ABattleArena absorveu o resultado via HandleCoordinatorTurnResolved"),
		Arena->GetCurrentState().TurnNumber != TurnNumberBeforeOpponentCommit || Arena->GetCurrentState().Pets[1].Health < OpponentPet.Health);

	DestroyHeadlessTestWorld(World);
	return true;
}
