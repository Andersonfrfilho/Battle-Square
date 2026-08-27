// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Battle/BattleActionQueueComponent.h"
#include "Battle/BattleState.h"
#include "Data/BattleDataTranslator.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOpponentActuallyMovesOverTurnsTest,
	"BattleSquare.BattleArena.OpponentMovesAtLeastOnceOverTenTurns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOpponentActuallyMovesOverTurnsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState InitialState;
	FPetState Jogador;
	Jogador.PetId = 1; Jogador.Side = 0; Jogador.Column = 1; Jogador.Row = 1;
	Jogador.Health = 500; Jogador.MaxHealth = 500; Jogador.Attack = 1; Jogador.Defense = 50; Jogador.Speed = 5;
	FPetState Inimigo = Jogador;
	Inimigo.PetId = 2; Inimigo.Side = 1; Inimigo.Column = 2; Inimigo.Row = 1;
	InitialState.Pets.Add(Jogador);
	InitialState.Pets.Add(Inimigo);

	TArray<FPetPresentationInfo> Apresentacoes;
	FPetPresentationInfo A; A.PetId = 1; A.Name = TEXT("Eu");
	FPetPresentationInfo B; B.PetId = 2; B.Name = TEXT("Ele");
	Apresentacoes.Add(A); Apresentacoes.Add(B);

	Arena->BeginBattle(InitialState, Apresentacoes);

	// Vida alta e ataque 1 de propósito: a batalha não pode acabar antes dos
	// dez turnos, senão o teste mede outra coisa.
	uint8 CasaInicialColuna = 0;
	uint8 CasaInicialLinha = 0;
	for (const FPetState& Pet : Arena->GetCurrentState().Pets)
	{
		if (Pet.Side == 1) { CasaInicialColuna = Pet.Column; CasaInicialLinha = Pet.Row; }
	}

	bool bInimigoSaiuDoLugar = false;
	TSet<uint64> EstadosDoGerador;

	for (int32 Turno = 0; Turno < 10; ++Turno)
	{
		EstadosDoGerador.Add(Arena->GetCurrentState().Random.State);

		Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
		Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
		Arena->PlayerActionQueue->BeginSelectingType(EActionType::Aguardar);
		Arena->PlayerActionQueue->Commit();

		// A rodada seguinte só abre quando a reprodução termina.
		for (int32 Frame = 0; Frame < 100 && Arena->PlayerActionQueue->IsCommitted(); ++Frame)
		{
			Arena->Tick(0.5f);
		}

		for (const FPetState& Pet : Arena->GetCurrentState().Pets)
		{
			if (Pet.Side == 1 && (Pet.Column != CasaInicialColuna || Pet.Row != CasaInicialLinha))
			{
				bInimigoSaiuDoLugar = true;
			}
		}
	}

	AddInfo(FString::Printf(TEXT("estados distintos do gerador em 10 turnos: %d"), EstadosDoGerador.Num()));

	TestTrue(TEXT("o gerador avança entre turnos (não repete o mesmo estado)"), EstadosDoGerador.Num() > 5);
	TestTrue(TEXT("em dez turnos o inimigo sai do lugar ao menos uma vez"), bInimigoSaiuDoLugar);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
