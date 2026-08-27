// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Net/BattleTurnCoordinator.h"
#include "Meta/PetCollectionService.h"
#include "Meta/PetProgressionService.h"
#include "Kismet/GameplayStatics.h"
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

	// Até 2026-08-26 este teste afirmava "fila travada após Commit" — e isso
	// era verdade só porque NADA reabria o turno: a batalha era de uma rodada
	// só, e o defeito apareceu no primeiro turno jogado de verdade, com a
	// tela travada em "aguardando o oponente".
	// O commit resolve o turno e, se a batalha continua, a fila reabre para a
	// rodada seguinte. O travamento em si segue coberto, isolado, em
	// BattleSquare.UI.BattleActionSelector.LockedAfterCommit.
	// A rodada resolve na hora, mas a REPRODUÇÃO leva tempo: as três ações
	// aparecem uma fase por vez. Abrir o turno seguinte antes disso deixaria
	// o jogador escolhendo o próximo turno com o anterior ainda na tela.
	TestTrue(TEXT("Enquanto o trace toca, a fila segue travada"),
		Arena->PlayerActionQueue->IsCommitted());

	// Tempo injetado: dirige a reprodução até o fim sem esperar de verdade.
	for (int32 Frame = 0; Frame < 60 && Arena->PlayerActionQueue->IsCommitted(); ++Frame)
	{
		Arena->Tick(0.5f);
	}

	TestFalse(TEXT("Terminada a reprodução, a fila destrava para a rodada seguinte"),
		Arena->PlayerActionQueue->IsCommitted());
	TestEqual(TEXT("E começa vazia, sem vazar as ações da rodada anterior"),
		Arena->PlayerActionQueue->GetConfirmedActionCount(), 0);

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

// T7 (arenas-variadas, ARENA-02): montagem que posicionaria um pet numa
// casa bloqueada é rejeitada explicitamente, nunca reposicionada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaRejectsInitialPositionOnBlockedCellTest,
	"BattleSquare.BattleArena.RejectsInitialPositionOnBlockedCell",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaRejectsInitialPositionOnBlockedCellTest::RunTest(const FString& Parameters)
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

	FBattleState BlockedState;
	FPetState PlayerPet;
	PlayerPet.PetId = 1; PlayerPet.Side = 0; PlayerPet.Column = 1; PlayerPet.Row = 1;
	PlayerPet.Health = 50; PlayerPet.MaxHealth = 50;
	FPetState OpponentPet;
	OpponentPet.PetId = 2; OpponentPet.Side = 1; OpponentPet.Column = 2; OpponentPet.Row = 1;
	OpponentPet.Health = 50; OpponentPet.MaxHealth = 50;
	BlockedState.Pets.Add(PlayerPet);
	BlockedState.Pets.Add(OpponentPet);
	BlockedState.CellLayout[CellLayoutIndex(1, 1)] = static_cast<uint8>(ECellProperty::Blocked); // casa do próprio PlayerPet

	TArray<FPetPresentationInfo> Presentations;
	FPetPresentationInfo PlayerPresentation;
	PlayerPresentation.PetId = PlayerPet.PetId;
	FPetPresentationInfo OpponentPresentation;
	OpponentPresentation.PetId = OpponentPet.PetId;
	Presentations.Add(PlayerPresentation);
	Presentations.Add(OpponentPresentation);

	AddExpectedError(TEXT("posicionado numa casa bloqueada"), EAutomationExpectedErrorFlags::Contains, 1);
	const bool bAccepted = Arena->BeginBattle(BlockedState, Presentations);
	TestFalse(TEXT("Montagem com pet em casa bloqueada é rejeitada"), bAccepted);
	TestEqual(TEXT("Nenhuma view spawnada — montagem nunca prosseguiu"), Arena->GetPetViews().Num(), 0);

	// Confirma que uma montagem válida (mesmos pets, layout neutro) continua funcionando.
	FBattleState ValidState = BlockedState;
	ValidState.CellLayout[CellLayoutIndex(1, 1)] = static_cast<uint8>(ECellProperty::None);
	const bool bValidAccepted = Arena->BeginBattle(ValidState, Presentations);
	TestTrue(TEXT("Montagem sem casa bloqueada é aceita normalmente"), bValidAccepted);

	DestroyHeadlessTestWorld(World);
	return true;
}

// T5 🧠 (colecao-e-captura): vitória do jogador local captura o pet do
// OPONENTE — nunca o próprio pet do jogador. CatalogIds distintos
// tornam a inversão fácil de detectar se acontecer.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaVictoryCapturesOpponentPetTest,
	"BattleSquare.BattleArena.VictoryCapturesOpponentPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaVictoryCapturesOpponentPetTest::RunTest(const FString& Parameters)
{
	const FString TestSlotName = TEXT("PetCollectionTestSlot_ArenaVictory");
	if (UGameplayStatics::DoesSaveGameExist(TestSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(TestSlotName, 0);
	}

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
	Arena->PetCollectionSlotName = TestSlotName;

	FBattleState InitialState;
	FPetState PlayerPet;
	PlayerPet.PetId = 1; PlayerPet.Side = 0; PlayerPet.Column = 1; PlayerPet.Row = 1;
	PlayerPet.Health = 50; PlayerPet.MaxHealth = 50; PlayerPet.Attack = 20; PlayerPet.Defense = 5;
	FPetState OpponentPet;
	OpponentPet.PetId = 2; OpponentPet.Side = 1; OpponentPet.Column = 2; OpponentPet.Row = 1;
	OpponentPet.Health = 1; OpponentPet.MaxHealth = 1; OpponentPet.Attack = 1; OpponentPet.Defense = 1000; // nunca causa dano de volta
	InitialState.Pets.Add(PlayerPet);
	InitialState.Pets.Add(OpponentPet);

	TArray<FPetPresentationInfo> Presentations;
	FPetPresentationInfo PlayerPresentation;
	PlayerPresentation.PetId = PlayerPet.PetId;
	PlayerPresentation.CatalogId = TEXT("catalog-player-nunca-capturado");
	PlayerPresentation.Name = TEXT("MeuPet");
	PlayerPresentation.Type = TEXT("Normal");
	FPetPresentationInfo OpponentPresentation;
	OpponentPresentation.PetId = OpponentPet.PetId;
	OpponentPresentation.CatalogId = TEXT("catalog-oponente-capturado");
	OpponentPresentation.Name = TEXT("PetInimigo");
	OpponentPresentation.Type = TEXT("Fogo");
	Presentations.Add(PlayerPresentation);
	Presentations.Add(OpponentPresentation);

	TestTrue(TEXT("Montagem aceita"), Arena->BeginBattle(InitialState, Presentations));

	// Caminho de rede (não FDumbOpponentAI): os dois commits são
	// deterministas, controlados pelo teste — elimina qualquer chance de
	// o oponente esquivar (Esquiva anula ataque físico) ou se mover para
	// fora do alcance, o que FDumbOpponentAI poderia escolher por sorte.
	// Magia, além disso, ignora esquiva mesmo se houvesse alguma.
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(InitialState, 0.0);
	Arena->ConfigureNetworkedOpponent(Coordinator);

	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Magia);
	Arena->PlayerActionQueue->ConfirmDirection(EBattleDirection::Direita);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->Commit(); // dispara HandlePlayerCommitted -> ServerCoordinator->SubmitCommit(0, ...)

	FTurnCommit OpponentCommit; // 3x Aguardar — nunca esquiva, nunca sai do lugar
	OpponentCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	OpponentCommit.Actions[1] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	OpponentCommit.Actions[2] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Coordinator->SubmitCommit(/*Side=*/1, OpponentCommit); // dispara a resolução real

	TestTrue(TEXT("Batalha terminou"), Arena->GetCurrentState().bBattleEnded);
	TestEqual(TEXT("Jogador local (Side 0) venceu"), Arena->GetCurrentState().WinningSide, static_cast<uint8>(0));

	const TArray<FOwnedPetInstance> Collection = FPetCollectionService::LoadCollection(TestSlotName);
	TestEqual(TEXT("Exatamente 1 pet capturado"), Collection.Num(), 1);
	if (Collection.Num() == 1)
	{
		TestEqual(TEXT("O pet capturado é o OPONENTE, não o próprio jogador"), Collection[0].CatalogId, FString(TEXT("catalog-oponente-capturado")));
	}

	if (UGameplayStatics::DoesSaveGameExist(TestSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(TestSlotName, 0);
	}

	DestroyHeadlessTestWorld(World);
	return true;
}

// T4 (niveis-experiencia-evolucao): vitória credita XP ao pet do
// JOGADOR LOCAL, se já capturado; derrota credita menos; pet não
// capturado não gera XP fantasma.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaVictoryGrantsExperienceToOwnPetTest,
	"BattleSquare.BattleArena.VictoryGrantsExperienceToOwnPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaVictoryGrantsExperienceToOwnPetTest::RunTest(const FString& Parameters)
{
	const FString TestSlotName = TEXT("PetCollectionTestSlot_XpGrant");
	if (UGameplayStatics::DoesSaveGameExist(TestSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(TestSlotName, 0);
	}

	// Pré-captura o pet do JOGADOR na coleção — é a instância que deve
	// receber XP.
	FOwnedPetInstance PlayerInstance;
	PlayerInstance.CatalogId = TEXT("catalog-jogador-ja-capturado");
	PlayerInstance.Name = TEXT("MeuPet");
	PlayerInstance.Type = TEXT("Normal");
	FPetCollectionService::CaptureIfNew(TestSlotName, PlayerInstance);

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
	Arena->PetCollectionSlotName = TestSlotName;

	FBattleState InitialState;
	FPetState PlayerPet;
	PlayerPet.PetId = 1; PlayerPet.Side = 0; PlayerPet.Column = 1; PlayerPet.Row = 1;
	PlayerPet.Health = 50; PlayerPet.MaxHealth = 50; PlayerPet.Attack = 20; PlayerPet.Defense = 5;
	FPetState OpponentPet;
	OpponentPet.PetId = 2; OpponentPet.Side = 1; OpponentPet.Column = 2; OpponentPet.Row = 1;
	OpponentPet.Health = 1; OpponentPet.MaxHealth = 1; OpponentPet.Attack = 1; OpponentPet.Defense = 1000;
	InitialState.Pets.Add(PlayerPet);
	InitialState.Pets.Add(OpponentPet);

	TArray<FPetPresentationInfo> Presentations;
	FPetPresentationInfo PlayerPresentation;
	PlayerPresentation.PetId = PlayerPet.PetId;
	PlayerPresentation.CatalogId = TEXT("catalog-jogador-ja-capturado");
	FPetPresentationInfo OpponentPresentation;
	OpponentPresentation.PetId = OpponentPet.PetId;
	OpponentPresentation.CatalogId = TEXT("catalog-oponente-nao-capturado");
	Presentations.Add(PlayerPresentation);
	Presentations.Add(OpponentPresentation);

	TestTrue(TEXT("Montagem aceita"), Arena->BeginBattle(InitialState, Presentations));

	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(InitialState, 0.0);
	Arena->ConfigureNetworkedOpponent(Coordinator);

	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Magia);
	Arena->PlayerActionQueue->ConfirmDirection(EBattleDirection::Direita);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->Commit();

	FTurnCommit OpponentCommit;
	OpponentCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	OpponentCommit.Actions[1] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	OpponentCommit.Actions[2] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Coordinator->SubmitCommit(/*Side=*/1, OpponentCommit);

	TestTrue(TEXT("Batalha terminou em vitória do jogador"), Arena->GetCurrentState().bBattleEnded && Arena->GetCurrentState().WinningSide == 0);

	const TArray<FOwnedPetInstance> Collection = FPetCollectionService::LoadCollection(TestSlotName);
	// Só o pet do jogador (já capturado antes da partida) — o oponente
	// TAMBÉM foi capturado por CheckForCapture (T5, colecao-e-captura),
	// então esperamos 2 instâncias: a original com XP, e a nova sem.
	TestEqual(TEXT("Coleção tem as 2 instâncias (jogador + oponente recém-capturado)"), Collection.Num(), 2);

	const FOwnedPetInstance* PlayerAfter = Collection.FindByPredicate(
		[](const FOwnedPetInstance& Instance) { return Instance.CatalogId == TEXT("catalog-jogador-ja-capturado"); });
	if (TestNotNull(TEXT("Instância do jogador ainda existe"), PlayerAfter))
	{
		TestEqual(TEXT("Instância do jogador recebeu XP de vitória"), PlayerAfter->Experience, BattlePetProgressionConstants::ExperienceForWin);
	}

	const FOwnedPetInstance* OpponentAfter = Collection.FindByPredicate(
		[](const FOwnedPetInstance& Instance) { return Instance.CatalogId == TEXT("catalog-oponente-nao-capturado"); });
	if (TestNotNull(TEXT("Instância do oponente (recém-capturada) existe"), OpponentAfter))
	{
		TestEqual(TEXT("Instância do oponente recém-capturada não recebeu XP — só existe pela captura"), OpponentAfter->Experience, 0);
	}

	if (UGameplayStatics::DoesSaveGameExist(TestSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(TestSlotName, 0);
	}

	DestroyHeadlessTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaCellAxesMatchScreenTest,
	"BattleSquare.BattleArena.CellAxesMatchWhatThePlayerSees",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaCellAxesMatchScreenTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateHeadlessTestWorld();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	const FVector Centro = Arena->GetCellWorldLocation(1, 1);

	// A câmera olha ao longo de +X: na tela, +Y é a direita e +X é o fundo.
	// Mapear Coluna->X e Linha->Y fazia "Baixo" andar para a DIREITA na tela,
	// que foi o defeito relatado jogando.
	const FVector Direita = Arena->GetCellWorldLocation(2, 1);
	TestTrue(TEXT("Direita (coluna+1) anda para +Y, que é a direita da tela"),
		Direita.Y > Centro.Y && FMath::IsNearlyEqual(Direita.X, Centro.X));

	const FVector Baixo = Arena->GetCellWorldLocation(1, 2);
	TestTrue(TEXT("Baixo (linha+1) anda para -X, que é a frente da tela"),
		Baixo.X < Centro.X && FMath::IsNearlyEqual(Baixo.Y, Centro.Y));

	const FVector Cima = Arena->GetCellWorldLocation(1, 0);
	TestTrue(TEXT("Cima (linha-1) anda para +X, que é o fundo da tela"),
		Cima.X > Centro.X);

	DestroyHeadlessTestWorld(World);
	return true;
}

// O pet DESLIZA até a casa nova em vez de aparecer nela. Teleporte não conta a
// história: quem só vê o antes e o depois não sabe se ele andou, se foi
// empurrado, nem em que ordem as coisas aconteceram.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetViewGlidesInsteadOfTeleportingTest,
	"BattleSquare.Battle.PetView.GlidesInsteadOfTeleporting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetViewGlidesInsteadOfTeleportingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	APetView* View = World->SpawnActor<APetView>();
	View->SetActorLocation(FVector::ZeroVector);

	const FVector Destino(300.0f, 0.0f, 0.0f);
	View->GlideTo(Destino);

	// Um passo curto: já saiu da origem, e ainda NÃO chegou.
	View->AdvanceGlide(0.1f);
	const FVector NoMeio = View->GetActorLocation();
	TestTrue(TEXT("Saiu do lugar"), NoMeio.X > 1.0f);
	TestTrue(TEXT("Ainda não chegou — não teleportou"), NoMeio.X < Destino.X - 1.0f);

	// Tempo suficiente: chega exatamente no destino, sem passar dele.
	View->AdvanceGlide(5.0f);
	TestTrue(TEXT("Chega no destino"),
		FVector::DistSquared(View->GetActorLocation(), Destino) < 1.0f);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
