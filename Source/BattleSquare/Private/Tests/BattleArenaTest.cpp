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
	Arena->PlayerActionQueue->ConfirmMove(0);
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
	Arena->PlayerActionQueue->ConfirmMove(0);
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
	BlockedState.CellLayout[BlockedState.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Blocked); // casa do próprio PlayerPet

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
	ValidState.CellLayout[ValidState.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::None);
	const bool bValidAccepted = Arena->BeginBattle(ValidState, Presentations);
	TestTrue(TEXT("Montagem sem casa bloqueada é aceita normalmente"), bValidAccepted);

	DestroyHeadlessTestWorld(World);
	return true;
}

// T5 🧠 (colecao-e-captura) + PS11: vitória contra um SELVAGEM captura o pet
// do oponente — nunca o próprio pet do jogador. O oponente aqui NÃO tem dono
// configurado (SideOwners[1] vazio), então é selvagem, e selvagem se captura.
// CatalogIds distintos tornam a inversão fácil de detectar se acontecer.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaVictoryCapturesWildPetTest,
	"BattleSquare.BattleArena.VictoryCapturesWildPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaVictoryCapturesWildPetTest::RunTest(const FString& Parameters)
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
	Arena->PlayerActionQueue->ConfirmMove(0);
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

	// "Exatamente 1 na coleção" era um PROXY para "a vitória não captura o seu
	// próprio pet". Ele envelheceu em 2026-08-27, quando o pet do jogador
	// passou a entrar na coleção ao COMEÇAR a batalha — sem isso a experiência
	// dele não tinha onde cair e ninguém progredia (L-036, mesmo padrão).
	//
	// A intenção original continua guardada, e agora com mais precisão: o
	// oponente entra por VITÓRIA, e o seu pet já estava lá por ser seu.
	const TArray<FOwnedPetInstance> Collection = FPetCollectionService::LoadCollection(TestSlotName);

	TestTrue(TEXT("O oponente derrotado foi capturado"),
		Collection.ContainsByPredicate([](const FOwnedPetInstance& Instance)
		{
			return Instance.CatalogId == TEXT("catalog-oponente-capturado");
		}));

	TestTrue(TEXT("O pet do jogador está na coleção — por ser dele, não por captura"),
		Collection.ContainsByPredicate([](const FOwnedPetInstance& Instance)
		{
			return Instance.CatalogId == TEXT("catalog-player-nunca-capturado");
		}));

	TestEqual(TEXT("E nada além dos dois entrou"), Collection.Num(), 2);

	if (UGameplayStatics::DoesSaveGameExist(TestSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(TestSlotName, 0);
	}

	DestroyHeadlessTestWorld(World);
	return true;
}

// PS11: vencer o pet de OUTRO JOGADOR nao o captura — o oponente TEM dono, e
// dono nao se ganha em batalha (isso e roubo, outra feature). Este e o par
// negativo do teste acima: a MESMA vitoria, so que o oponente e de alguem.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaVictoryDoesNotStealOwnedPetTest,
	"BattleSquare.BattleArena.VictoryDoesNotCaptureOwnedPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaVictoryDoesNotStealOwnedPetTest::RunTest(const FString& Parameters)
{
	const FString VencedorSlot = TEXT("PetCollectionTestSlot_PS11_Vencedor");
	const FString DonoSlot = TEXT("PetCollectionTestSlot_PS11_Dono");
	for (const FString& Slot : { VencedorSlot, DonoSlot })
	{
		if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
		}
	}

	UWorld* World = CreateHeadlessTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World))
	{
		return false;
	}

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	Arena->PetCollectionSlotName = VencedorSlot;

	// OS DOIS LADOS TEM DONO: o lado zero e o vencedor, o lado um pertence a
	// OUTRA conta. E o oponente com dono que a regra nova protege.
	Arena->SideOwners[0].CollectionSlot = VencedorSlot;
	Arena->SideOwners[0].AccountId = TEXT("conta-vencedor");
	Arena->SideOwners[1].CollectionSlot = DonoSlot;
	Arena->SideOwners[1].AccountId = TEXT("conta-do-dono");

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
	PlayerPresentation.PetId = 1;
	PlayerPresentation.CatalogId = TEXT("catalog-vencedor");
	PlayerPresentation.Name = TEXT("MeuPet");
	PlayerPresentation.Type = TEXT("Normal");
	FPetPresentationInfo OpponentPresentation;
	OpponentPresentation.PetId = 2;
	OpponentPresentation.CatalogId = TEXT("catalog-do-outro-jogador");
	OpponentPresentation.Name = TEXT("PetDoOutro");
	OpponentPresentation.Type = TEXT("Fogo");
	Presentations.Add(PlayerPresentation);
	Presentations.Add(OpponentPresentation);

	TestTrue(TEXT("Montagem aceita"), Arena->BeginBattle(InitialState, Presentations));

	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(InitialState, 0.0);
	Arena->ConfigureNetworkedOpponent(Coordinator);

	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Magia);
	Arena->PlayerActionQueue->ConfirmMove(0);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->Commit();

	FTurnCommit OpponentCommit;
	OpponentCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	OpponentCommit.Actions[1] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	OpponentCommit.Actions[2] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Coordinator->SubmitCommit(1, OpponentCommit);

	TestEqual(TEXT("O vencedor foi o lado 0"),
		Arena->GetCurrentState().WinningSide, static_cast<uint8>(0));

	// O ACEITE DA PS11: o pet do outro jogador NAO entrou na colecao do
	// vencedor. A vitoria rendeu (premio/ranking sao de outra camada); o pet
	// com dono ficou com o dono.
	const TArray<FOwnedPetInstance> Colecao = FPetCollectionService::LoadCollection(VencedorSlot);
	TestFalse(TEXT("o pet do outro jogador NAO foi capturado"),
		Colecao.ContainsByPredicate([](const FOwnedPetInstance& Instance)
		{
			return Instance.CatalogId == TEXT("catalog-do-outro-jogador");
		}));

	for (const FString& Slot : { VencedorSlot, DonoSlot })
	{
		if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
		}
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
	Arena->PlayerActionQueue->ConfirmMove(0);
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
	// PS11: este teste NAO configura SideOwners, entao o oponente e SELVAGEM
	// (sem dono) e a vitoria o captura — o comportamento correto. Duas
	// instancias: o pet do jogador com XP, e o selvagem recem-capturado sem.
	TestEqual(TEXT("Coleção tem as 2 instâncias (jogador + selvagem recém-capturado)"), Collection.Num(), 2);

	const FOwnedPetInstance* PlayerAfter = Collection.FindByPredicate(
		[](const FOwnedPetInstance& Instance) { return Instance.CatalogId == TEXT("catalog-jogador-ja-capturado"); });
	if (TestNotNull(TEXT("Instância do jogador ainda existe"), PlayerAfter))
	{
		TestEqual(TEXT("Instância do jogador recebeu XP de vitória"), PlayerAfter->Experience, BattlePetProgressionConstants::ExperienceForWin);
	}

	const FOwnedPetInstance* OpponentAfter = Collection.FindByPredicate(
		[](const FOwnedPetInstance& Instance) { return Instance.CatalogId == TEXT("catalog-oponente-nao-capturado"); });
	if (TestNotNull(TEXT("Instância do selvagem (recém-capturada) existe"), OpponentAfter))
	{
		TestEqual(TEXT("Selvagem recém-capturado não recebeu XP — só existe pela captura"), OpponentAfter->Experience, 0);
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

// B-005: "de quem é a tela" e "de quem é a coleção" eram a MESMA variável.
//
// Num servidor com dois jogadores remotos, a arena roda no processo de
// nenhum dos dois: só o lado marcado como local capturava, e a escrita caía
// no save do SERVIDOR. Ninguém percebia, porque em Standalone o servidor é
// o jogador — e é assim que uma limitação vive dois marcos sem incomodar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleArenaCadaLadoTemSuaColecaoTest,
	"BattleSquare.BattleArena.CadaLadoTemSuaColecao",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleArenaCadaLadoTemSuaColecaoTest::RunTest(const FString& Parameters)
{
	const FString SlotDoLadoZero = TEXT("PetCollectionTestSlot_B005_LadoZero");
	const FString SlotDoLadoUm = TEXT("PetCollectionTestSlot_B005_LadoUm");
	for (const FString& Slot : { SlotDoLadoZero, SlotDoLadoUm })
	{
		if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
		}
	}

	UWorld* World = CreateHeadlessTestWorld();
	if (!TestNotNull(TEXT("Mundo de teste criado"), World)) { return false; }

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	if (!TestNotNull(TEXT("ABattleArena spawna sem crash"), Arena))
	{
		DestroyHeadlessTestWorld(World);
		return false;
	}

	// Dois donos DIFERENTES, e a tela é do lado zero.
	Arena->SideOwners[0].CollectionSlot = SlotDoLadoZero;
	Arena->SideOwners[1].CollectionSlot = SlotDoLadoUm;

	FBattleState InitialState;
	FPetState PetDoZero;
	PetDoZero.PetId = 1; PetDoZero.Side = 0; PetDoZero.Column = 1; PetDoZero.Row = 1;
	PetDoZero.Health = 1; PetDoZero.MaxHealth = 1; PetDoZero.Attack = 1; PetDoZero.Defense = 1;

	// O lado UM vence: é ele o jogador remoto que antes nunca recebia nada.
	FPetState PetDoUm;
	PetDoUm.PetId = 2; PetDoUm.Side = 1; PetDoUm.Column = 2; PetDoUm.Row = 1;
	PetDoUm.Health = 80; PetDoUm.MaxHealth = 80; PetDoUm.Attack = 60; PetDoUm.Defense = 1000;

	InitialState.Pets.Add(PetDoZero);
	InitialState.Pets.Add(PetDoUm);

	TArray<FPetPresentationInfo> Presentations;
	FPetPresentationInfo ApresentacaoDoZero;
	ApresentacaoDoZero.PetId = 1;
	ApresentacaoDoZero.CatalogId = TEXT("catalog-do-lado-zero");
	ApresentacaoDoZero.Name = TEXT("PetDoZero");
	ApresentacaoDoZero.Type = TEXT("Normal");
	FPetPresentationInfo ApresentacaoDoUm;
	ApresentacaoDoUm.PetId = 2;
	ApresentacaoDoUm.CatalogId = TEXT("catalog-do-lado-um");
	ApresentacaoDoUm.Name = TEXT("PetDoUm");
	ApresentacaoDoUm.Type = TEXT("Fogo");
	Presentations.Add(ApresentacaoDoZero);
	Presentations.Add(ApresentacaoDoUm);

	TestTrue(TEXT("Montagem aceita"), Arena->BeginBattle(InitialState, Presentations));

	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(InitialState, 0.0);
	Arena->ConfigureNetworkedOpponent(Coordinator);

	// O lado zero espera; o lado um ataca com magia, que ignora esquiva.
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
	Arena->PlayerActionQueue->Commit();

	FTurnCommit CommitDoUm;
	CommitDoUm.Actions[0] = { EActionType::Magia, EBattleDirection::Esquerda };
	CommitDoUm.Actions[1] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	CommitDoUm.Actions[2] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Coordinator->SubmitCommit(/*Side=*/1, CommitDoUm);

	TestTrue(TEXT("Batalha terminou"), Arena->GetCurrentState().bBattleEnded);
	TestEqual(TEXT("O lado UM venceu"), Arena->GetCurrentState().WinningSide,
		static_cast<uint8>(1));

	// PS11 REESCREVEU O ACEITE: os DOIS lados têm `CollectionSlot`, então o
	// perdedor TEM dono — e dono não se captura por vitória (é roubo, outra
	// feature). O que este teste guarda continua sendo o coração do B-005 (a
	// escrita vai para a coleção do lado CERTO, não a do servidor); o que
	// muda é que o pet com dono NÃO troca de mão numa batalha comum.
	const TArray<FOwnedPetInstance> ColecaoDoUm =
		FPetCollectionService::LoadCollection(SlotDoLadoUm);
	TestFalse(TEXT("Vencer o pet de OUTRO DONO nao o captura (PS11)"),
		ColecaoDoUm.ContainsByPredicate([](const FOwnedPetInstance& Instancia)
		{ return Instancia.CatalogId == TEXT("catalog-do-lado-zero"); }));

	// E a coleção de quem PERDEU não recebeu o próprio pet de volta como
	// troféu: cada save é de um dono, e misturá-los era o defeito.
	const TArray<FOwnedPetInstance> ColecaoDoZero =
		FPetCollectionService::LoadCollection(SlotDoLadoZero);
	TestFalse(TEXT("Quem perdeu não capturou ninguém"),
		ColecaoDoZero.ContainsByPredicate([](const FOwnedPetInstance& Instancia)
		{ return Instancia.CatalogId == TEXT("catalog-do-lado-um"); }));

	for (const FString& Slot : { SlotDoLadoZero, SlotDoLadoUm })
	{
		if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
		}
	}
	DestroyHeadlessTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaSideOwnerCarriesTheAccountTest,
	"BattleSquare.Battle.Arena.OLadoCarregaAConta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaSideOwnerCarriesTheAccountTest::RunTest(const FString& Parameters)
{
	// PS5: a costura de B-005 trocou o que GUARDA — o dono do lado é slot +
	// conta, UMA coisa (invariante 17). E o contrapeso: sem nada configurado,
	// o comportamento é o de Standalone, byte a byte.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
	Contexto.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	// SEM configuração: o slot local cai no nome único (Standalone de
	// sempre), e a CONTA é vazia — fallback de conta inventaria identidade,
	// e identidade inventada num servidor é posse de ninguém com cara de
	// posse de alguém.
	TestFalse(TEXT("o lado local tem slot (o de sempre)"),
		Arena->ResolveCollectionSlotForSide(0).IsEmpty());
	TestTrue(TEXT("mas conta NAO se inventa"),
		Arena->ResolveAccountIdForSide(0).IsEmpty());
	TestTrue(TEXT("e o lado sem dono segue sem slot"),
		Arena->ResolveCollectionSlotForSide(1).IsEmpty());

	// COM dono configurado, o lado carrega os dois — e cada lado o SEU.
	Arena->SideOwners[0].CollectionSlot = TEXT("slot-do-zero");
	Arena->SideOwners[0].AccountId = TEXT("conta-do-zero");
	Arena->SideOwners[1].CollectionSlot = TEXT("slot-do-um");
	Arena->SideOwners[1].AccountId = TEXT("conta-do-um");

	TestEqual(TEXT("o lado zero responde a conta do zero"),
		Arena->ResolveAccountIdForSide(0), FString(TEXT("conta-do-zero")));
	TestEqual(TEXT("e o lado um, a do um — nunca a de quem olha a tela"),
		Arena->ResolveAccountIdForSide(1), FString(TEXT("conta-do-um")));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
