// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/DumbOpponentAI.h"
#include "Battle/BattleState.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDumbOpponentEventuallyMovesTest,
	"BattleSquare.DumbOpponentAI.EventuallyChoosesEveryActionType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDumbOpponentEventuallyMovesTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	FPetState Pet;
	Pet.PetId = 2;
	Pet.Side = 1;
	Pet.Column = 2;
	Pet.Row = 1;
	Pet.Health = 50;
	Pet.MaxHealth = 50;
	State.Pets.Add(Pet);
	State.Random.State = 12345;

	TMap<EActionType, int32> Contagem;
	for (int32 Turno = 0; Turno < 200; ++Turno)
	{
		const FTurnCommit Commit = FDumbOpponentAI::GenerateRandomValidCommit(State, 1, State.Random);
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			Contagem.FindOrAdd(Commit.Actions[Slot].Type)++;
		}
	}

	// Em 600 ações, um sorteio uniforme entre 6 tipos deixa cada um com ~100.
	// O relato de "o inimigo nunca se move" só se explica se Mover for raro
	// ou impossível — este teste é o que separa azar de defeito.
	const int32 Moveres = Contagem.FindRef(EActionType::Mover);
	const int32 Defenderes = Contagem.FindRef(EActionType::Defender);

	AddInfo(FString::Printf(TEXT("Mover=%d Defender=%d Aguardar=%d Atacar=%d Magia=%d Esquivar=%d"),
		Moveres, Defenderes,
		Contagem.FindRef(EActionType::Aguardar), Contagem.FindRef(EActionType::Atacar),
		Contagem.FindRef(EActionType::Magia), Contagem.FindRef(EActionType::Esquivar)));

	TestTrue(TEXT("a IA escolhe Mover com frequência razoável (>5% de 600)"), Moveres > 30);
	TestTrue(TEXT("a IA escolhe Defender com frequência razoável (>5% de 600)"), Defenderes > 30);
	return true;
}
