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

// Com o modo ligado, o turno pede DUAS escolhas antes de resolver. Sem isso,
// verificar a trombada ou a esquiva na trombada depende do sorteio do oponente
// cair no caso que se quer ver — e não cai quando se precisa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FControlBothSidesAsksForTwoCommitsTest,
	"BattleSquare.BattleArena.ControlBothSides.AsksForTwoCommits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FControlBothSidesAsksForTwoCommitsTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena;
	Cena.Arena->SetControllingBothSides(true);

	TestEqual(TEXT("Começa escolhendo pelo jogador"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(0));

	Cena.Commit(EActionType::Aguardar);

	// Nada foi resolvido ainda: a fila reabriu para o outro lado.
	TestEqual(TEXT("Agora escolhe pelo oponente"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(1));
	TestFalse(TEXT("A fila reabriu para a segunda escolha"),
		Cena.Arena->PlayerActionQueue->IsCommitted());

	return true;
}

// A prova de que o modo serve para o que foi pedido: mandar os dois para a
// mesma casa e ver a trombada acontecer, por escolha, não por sorte.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FControlBothSidesCanForceAnEncounterTest,
	"BattleSquare.BattleArena.ControlBothSides.CanForceAnEncounter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FControlBothSidesCanForceAnEncounterTest::RunTest(const FString& Parameters)
{
	// Casas 0 e 2: ADJACENTES trocariam de lugar em vez de disputar a mesma
	// casa, e o teste mediria outra coisa. Foi o erro que escrevi primeiro.
	FScopedArena Cena(/*PlayerColumn=*/0, /*OpponentColumn=*/2);
	Cena.Arena->SetControllingBothSides(true);

	// Os dois mirando (1,1) — sem controlar os dois lados, isto seria esperar
	// o acaso do oponente escolher exatamente esse movimento.
	Cena.Commit(EActionType::Mover, EBattleDirection::Direita);
	Cena.Commit(EActionType::Mover, EBattleDirection::Esquerda);
	Cena.PlayOutAnimation();

	TestEqual(TEXT("Jogador ficou onde estava"), Cena.ColumnOfSide(0), static_cast<uint8>(0));
	TestEqual(TEXT("Oponente ficou onde estava"), Cena.ColumnOfSide(1), static_cast<uint8>(2));

	return true;
}

// Desligado, nada muda: o turno resolve com uma escolha só, como sempre.
// Um modo de depuração que altera o jogo com ele desligado é pior que não ter.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FControlBothSidesOffKeepsSingleCommitTest,
	"BattleSquare.BattleArena.ControlBothSides.OffKeepsSingleCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FControlBothSidesOffKeepsSingleCommitTest::RunTest(const FString& Parameters)
{
	FScopedArena Cena;

	TestEqual(TEXT("Desligado, sempre se escolhe pelo jogador"),
		Cena.Arena->GetSideBeingChosen(), static_cast<uint8>(0));

	Cena.Commit(EActionType::Aguardar);
	TestTrue(TEXT("Uma escolha só já fecha o turno"),
		Cena.Arena->PlayerActionQueue->IsCommitted());

	return true;
}
