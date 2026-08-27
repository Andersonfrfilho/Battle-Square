// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
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
				if (BattleActionRequiresDirection(Type))
				{
					Arena->PlayerActionQueue->ConfirmDirection(Direction);
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

// Trocar no meio de uma escolha não pode manter as ações já confirmadas: elas
// foram pensadas para o OUTRO pet, e aplicá-las ao novo faria a troca produzir
// uma jogada que ninguém pediu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FControlledPlayerSwapClearsChoicesTest,
	"BattleSquare.BattleArena.ControlledPlayer.SwapClearsPendingChoices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FControlledPlayerSwapClearsChoicesTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena;

	Cena.Arena->PlayerActionQueue->BeginSelectingType(EActionType::Defender);
	TestEqual(TEXT("Uma ação foi confirmada"),
		Cena.Arena->PlayerActionQueue->GetConfirmedActionCount(), 1);

	Cena.Arena->SwapControlledPlayer();

	TestEqual(TEXT("A troca zera o que estava escolhido"),
		Cena.Arena->PlayerActionQueue->GetConfirmedActionCount(), 0);

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
