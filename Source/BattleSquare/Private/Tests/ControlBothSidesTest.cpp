// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Meta/PetCollectionSaveGame.h"
#include "Meta/PetCollectionService.h"
#include "Battle/BattleActionQueueComponent.h"
#include "Battle/PetView.h"
#include "Data/BattleDataTranslator.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
	struct FScopedArena
	{
		UWorld* World = nullptr;
		ABattleArena* Arena = nullptr;

		explicit FScopedArena(uint8 PlayerColumn = 1, uint8 OpponentColumn = 2)
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			Arena = World->SpawnActor<ABattleArena>();

			FBattleState Initial;
			FPetState Jogador;
			Jogador.PetId = 1; Jogador.Side = 0; Jogador.Column = PlayerColumn; Jogador.Row = 1;
			Jogador.Health = 500; Jogador.MaxHealth = 500;
			Jogador.Attack = 10; Jogador.Defense = 50; Jogador.Speed = 5;
			FPetState Inimigo = Jogador;
			Inimigo.PetId = 2; Inimigo.Side = 1; Inimigo.Column = OpponentColumn; Inimigo.Row = 1;
			Initial.Pets.Add(Jogador);
			Initial.Pets.Add(Inimigo);

			TArray<FPetPresentationInfo> Apresentacoes;
			FPetPresentationInfo A; A.PetId = 1; A.Name = TEXT("Eu");
			FPetPresentationInfo B; B.PetId = 2; B.Name = TEXT("Ele");
			Apresentacoes.Add(A); Apresentacoes.Add(B);

			Arena->BeginBattle(Initial, Apresentacoes);
		}

		~FScopedArena()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		void Commit(EActionType Type, EBattleDirection Direction = EBattleDirection::Nenhuma)
		{
			for (int32 Slot = 0; Slot < 3; ++Slot)
			{
				Arena->PlayerActionQueue->BeginSelectingType(Type);

				// Desde DP-golpe-05 o ataque confirma por GOLPE, não por
				// direção: mover ainda pergunta para onde, atacar pergunta
				// qual.
				if (BattleActionRequiresDirection(Type))
				{
					Arena->PlayerActionQueue->ConfirmDirection(Direction);
				}
				else if (BattleActionRequiresMove(Type))
				{
					Arena->PlayerActionQueue->ConfirmMove(0);
				}
			}
			Arena->PlayerActionQueue->Commit();
		}

		void PlayOutAnimation()
		{
			for (int32 Frame = 0; Frame < 200 && Arena->PlayerActionQueue->IsCommitted(); ++Frame)
			{
				Arena->Tick(0.5f);
			}
		}

		uint8 ColumnOfSide(uint8 Side) const
		{
			for (const FPetState& Pet : Arena->GetCurrentState().Pets)
			{
				if (Pet.Side == Side) { return Pet.Column; }
			}
			return 0xFF;
		}
	};
}

// TROCAR de jogador controlado: aperta uma vez e você comanda o jogador 2, o
// bot assume o 1; aperta de novo e volta. É o que permite experimentar ações
// diferentes de cada lado sem depender do sorteio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FControlledPlayerSwapsSidesTest,
	"BattleSquare.BattleArena.ControlledPlayer.SwapsSides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FControlledPlayerSwapsSidesTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena;

	TestEqual(TEXT("Começa controlando o jogador 1"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(0));

	Cena.Arena->SwapControlledPlayer();
	TestEqual(TEXT("Uma troca leva ao jogador 2"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(1));

	Cena.Arena->SwapControlledPlayer();
	TestEqual(TEXT("Outra troca volta ao jogador 1"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(0));

	return true;
}

// A prova de que a troca serve: controlando o jogador 2, a ação escolhida tem
// que sair NO PET DELE. Se ela continuasse indo para o jogador 1, a troca seria
// só um rótulo diferente na tela.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FControlledPlayerActionLandsTest,
	"BattleSquare.BattleArena.ControlledPlayer.ActionLandsOnControlledPet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FControlledPlayerActionLandsTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena(/*PlayerColumn=*/0, /*OpponentColumn=*/2);
	Cena.Arena->SwapControlledPlayer();

	// Jogador 2 está em (2,1); mandar para a ESQUERDA o leva a (1,1).
	Cena.Commit(EActionType::Mover, EBattleDirection::Esquerda);
	Cena.PlayOutAnimation();

	TestEqual(TEXT("O pet do jogador 2 obedeceu"), Cena.ColumnOfSide(1), static_cast<uint8>(1));

	return true;
}

// Cada jogador tem o PRÓPRIO rascunho, e trocar de um para o outro não apaga
// nada: quem escolheu dois movimentos pelo jogador 1, foi ver o 2 e voltou,
// encontra os dois movimentos onde os deixou.
//
// A primeira versão ZERAVA as escolhas ao trocar. O motivo era legítimo — não
// aplicar ao pet errado o que foi pensado para o outro — mas a solução era
// grosseira: separar os rascunhos resolve o mesmo problema sem perder trabalho
// de quem está jogando.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FControlledPlayerSwapKeepsDraftsTest,
	"BattleSquare.BattleArena.ControlledPlayer.SwapKeepsEachDraft",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FControlledPlayerSwapKeepsDraftsTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena;
	UBattleActionQueueComponent* Fila = Cena.Arena->PlayerActionQueue;

	// Jogador 1 escolhe duas ações.
	Fila->BeginSelectingType(EActionType::Defender);
	Fila->BeginSelectingType(EActionType::Esquivar);
	TestEqual(TEXT("Jogador 1 tem duas ações"), Fila->GetConfirmedActionCount(), 2);

	Cena.Arena->SwapControlledPlayer();
	TestEqual(TEXT("Jogador 2 começa com a folha em branco"),
		Fila->GetConfirmedActionCount(), 0);

	// Jogador 2 escolhe uma.
	Fila->BeginSelectingType(EActionType::Aguardar);
	TestEqual(TEXT("Jogador 2 tem uma ação"), Fila->GetConfirmedActionCount(), 1);

	Cena.Arena->SwapControlledPlayer();
	TestEqual(TEXT("As duas do jogador 1 continuam lá"),
		Fila->GetConfirmedActionCount(), 2);

	Cena.Arena->SwapControlledPlayer();
	TestEqual(TEXT("E a do jogador 2 também"),
		Fila->GetConfirmedActionCount(), 1);

	return true;
}

// Os DOIS no mesmo turno: é o que a trombada exige, e é o que faz o painel de
// ações do jogador 2 aparecer na barra. Sem os dois commits, forçar um caso
// específico continuaria dependendo do sorteio da IA.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBothSidesCollectsTwoCommitsTest,
	"BattleSquare.BattleArena.BothSides.CollectsTwoCommits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBothSidesCollectsTwoCommitsTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena(/*PlayerColumn=*/0, /*OpponentColumn=*/2);
	Cena.Arena->SetControllingBothSides(true);

	TestEqual(TEXT("Primeiro escolhe pelo jogador 1"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(0));

	Cena.Commit(EActionType::Mover, EBattleDirection::Direita);

	// É AQUI que o painel do jogador 2 aparece na barra: a vez virou dele.
	TestEqual(TEXT("Depois escolhe pelo jogador 2"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(1));

	Cena.Commit(EActionType::Mover, EBattleDirection::Esquerda);
	Cena.PlayOutAnimation();

	// Os dois mirando (1,1): trombam e nenhum entra (DP-02).
	TestEqual(TEXT("Jogador 1 ficou onde estava"), Cena.ColumnOfSide(0), static_cast<uint8>(0));
	TestEqual(TEXT("Jogador 2 ficou onde estava"), Cena.ColumnOfSide(1), static_cast<uint8>(2));

	return true;
}

// O caminho simples: escolhe-se as ações do jogador 2 na barra, e o turno fecha
// pelo botão NORMAL de confirmar do jogador 1. Sem modo para ligar, sem
// segunda fase — cada etapa a mais era uma chance de o caminho parecer
// destrutivo, e foi o que travou o usuário três vezes.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerTwoManualActionsReplaceBotTest,
	"BattleSquare.BattleArena.PlayerTwo.ManualActionsReplaceBot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerTwoManualActionsReplaceBotTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena(/*PlayerColumn=*/0, /*OpponentColumn=*/2);

	// Jogador 2 vai para a esquerda, escolhido à mão.
	FBattleAction Passo;
	Passo.Type = EActionType::Mover;
	Passo.Direction = EBattleDirection::Esquerda;
	Cena.Arena->AddPlayerTwoAction(Passo);

	TestEqual(TEXT("A ação ficou registrada"),
		Cena.Arena->GetPlayerTwoActions().Num(), 1);

	// Jogador 1 confirma pelo caminho de sempre — e isso já resolve o turno.
	Cena.Commit(EActionType::Aguardar);
	Cena.PlayOutAnimation();

	TestEqual(TEXT("O jogador 2 obedeceu à ação escolhida, não ao bot"),
		Cena.ColumnOfSide(1), static_cast<uint8>(1));

	TestEqual(TEXT("As ações não sobrevivem ao turno"),
		Cena.Arena->GetPlayerTwoActions().Num(), 0);

	return true;
}

// A batalha LOCAL precisa anunciar que acabou.
//
// Defeito que o usuário viveu: caminhou pelo mundo, achou o inimigo, foi para a
// arena, derrotou o inimigo — e NADA aconteceu. Ele ficou preso numa arena de
// uma batalha já terminada.
//
// A causa: captura, XP e anúncio do fim só eram chamados no caminho de REDE.
// A batalha local resolvia o turno, avaliava o desfecho, e não contava a
// ninguém. Como M1–M4 nunca jogaram uma partida até o fim por uma tela, o
// caminho local nunca tinha chegado ali.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLocalBattleAnnouncesItsEndTest,
	"BattleSquare.BattleArena.LocalBattle.AnnouncesItsEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocalBattleAnnouncesItsEndTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena;

	int32 Anuncios = 0;
	Cena.Arena->OnBattleFinished.AddLambda([&Anuncios]() { ++Anuncios; });

	// Deixa o jogador 2 quase morto e manda o jogador 1 atacá-lo até cair.
	for (FPetState& Pet : Cena.Arena->GetMutableCurrentState().Pets)
	{
		if (Pet.Side == 1)
		{
			Pet.Health = 1;
		}
		else
		{
			Pet.Attack = 500;
		}
	}

	// Vários turnos: a morte pode não vir no primeiro por esquiva ou postura.
	for (int32 Turno = 0; Turno < 6 && Anuncios == 0; ++Turno)
	{
		Cena.Commit(EActionType::Atacar, EBattleDirection::Direita);
		Cena.PlayOutAnimation();
	}

	TestEqual(TEXT("O fim da batalha foi anunciado UMA vez"), Anuncios, 1);
	return true;
}

// O fim só é anunciado DEPOIS que a reprodução termina.
//
// Anunciando na hora em que o turno resolve, a transição arranca o jogador da
// arena antes de o golpe final aparecer — e junto vai a mensagem de quem
// venceu. O usuário descreveu isso como "apenas saiu da tela".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleEndWaitsForPlaybackTest,
	"BattleSquare.BattleArena.LocalBattle.EndWaitsForPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBattleEndWaitsForPlaybackTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena;

	int32 Anuncios = 0;
	Cena.Arena->OnBattleFinished.AddLambda([&Anuncios]() { ++Anuncios; });

	for (FPetState& Pet : Cena.Arena->GetMutableCurrentState().Pets)
	{
		if (Pet.Side == 1) { Pet.Health = 1; } else { Pet.Attack = 500; }
	}

	Cena.Commit(EActionType::Atacar, EBattleDirection::Direita);

	// Turno resolvido, reprodução ainda rodando: o jogador ainda está vendo o
	// golpe acontecer, e arrancá-lo daqui é o defeito.
	TestEqual(TEXT("Ainda não anunciou — a reprodução não acabou"), Anuncios, 0);

	Cena.PlayOutAnimation();

	TestEqual(TEXT("Anunciou ao fim da reprodução"), Anuncios, 1);
	return true;
}

// O pet com que você LUTA é seu, e precisa estar na coleção.
//
// A única coisa que povoava a coleção era capturar o oponente derrotado. O pet
// do jogador nunca entrava — então a XP dele não tinha onde cair, e o jogador
// nunca progredia. A recusa de XP estava certa; o que faltava era o pet estar
// lá desde o começo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOwnPetJoinsCollectionOnBattleStartTest,
	"BattleSquare.BattleArena.Collection.OwnPetJoinsOnBattleStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOwnPetJoinsCollectionOnBattleStartTest::RunTest(const FString& Parameters)
{
	const FString Slot = TEXT("TesteColecaoInicial");
	FPetCollectionService::SaveCollection(Slot, {});

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	Arena->PetCollectionSlotName = Slot;

	FBattleState Inicial;
	FPetState Meu;
	Meu.PetId = 1; Meu.Side = 0; Meu.Column = 1; Meu.Row = 1;
	Meu.Health = 100; Meu.MaxHealth = 100;
	FPetState Dele = Meu;
	Dele.PetId = 2; Dele.Side = 1; Dele.Column = 2;
	Inicial.Pets.Add(Meu);
	Inicial.Pets.Add(Dele);

	TArray<FPetPresentationInfo> Apresentacoes;
	FPetPresentationInfo A; A.PetId = 1; A.Name = TEXT("Meu"); A.CatalogId = TEXT("meu-pet");
	FPetPresentationInfo B; B.PetId = 2; B.Name = TEXT("Dele"); B.CatalogId = TEXT("pet-dele");
	Apresentacoes.Add(A); Apresentacoes.Add(B);

	Arena->BeginBattle(Inicial, Apresentacoes);

	const TArray<FOwnedPetInstance> Colecao = FPetCollectionService::LoadCollection(Slot);

	const bool bTemOMeu = Colecao.ContainsByPredicate(
		[](const FOwnedPetInstance& Instance) { return Instance.CatalogId == TEXT("meu-pet"); });
	const bool bTemODele = Colecao.ContainsByPredicate(
		[](const FOwnedPetInstance& Instance) { return Instance.CatalogId == TEXT("pet-dele"); });

	TestTrue(TEXT("O pet com que você luta entrou na coleção"), bTemOMeu);

	// O oponente NÃO entra por começar a batalha: ele se ganha vencendo, e
	// entrar aqui daria de graça o que a captura deveria custar.
	TestFalse(TEXT("O pet do oponente NÃO entra ao começar"), bTemODele);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
