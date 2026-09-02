// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/DumbOpponentAI.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FBattleState MakeTestState(uint8 Column, uint8 Row)
	{
		FBattleState State;
		FPetState Pet;
		Pet.PetId = 1;
		Pet.Side = 1;
		Pet.Column = Column;
		Pet.Row = Row;
		Pet.Health = 50;
		Pet.MaxHealth = 50;
		State.Pets.Add(Pet);
		return State;
	}

	bool IsInsideGridBounds(const FBattleState& State, uint8 Column, uint8 Row)
	{
		return State.IsInside(static_cast<int32>(Column), static_cast<int32>(Row));
	}
}

// T5: nunca gera Mover para fora da grade, mesmo repetido muitas vezes
// a partir de um canto (onde só metade das direções são válidas).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDumbOpponentAINeverMovesOutOfBoundsTest,
	"BattleSquare.DumbOpponentAI.NeverGeneratesOutOfBoundsMove",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDumbOpponentAINeverMovesOutOfBoundsTest::RunTest(const FString& Parameters)
{
	// Canto (0,0): só 3 das 8 direções mantêm o pet dentro da grade.
	const FBattleState State = MakeTestState(0, 0);
	FBattleRandom Random;
	Random.State = 12345ULL;

	bool bAllValid = true;
	for (int32 Iteration = 0; Iteration < 500; ++Iteration)
	{
		const FTurnCommit Commit = FDumbOpponentAI::GenerateRandomValidCommit(State, /*Side=*/1, Random);
		for (const FBattleAction& Action : Commit.Actions)
		{
			if (Action.Type != EActionType::Mover)
			{
				continue;
			}
			int8 DeltaColumn = 0;
			int8 DeltaRow = 0;
			GetDirectionDelta(Action.Direction, DeltaColumn, DeltaRow);
			const int32 DestColumn = static_cast<int32>(State.Pets[0].Column) + DeltaColumn;
			const int32 DestRow = static_cast<int32>(State.Pets[0].Row) + DeltaRow;
			if (!State.IsInside(DestColumn, DestRow))
			{
				AddError(FString::Printf(TEXT("Iteração %d: Mover gerado para fora da grade (%d,%d)"), Iteration, DestColumn, DestRow));
				bAllValid = false;
			}
		}
	}

	return bAllValid;
}

// T5: determinismo — mesma seed, mesmo estado, mesmo commit gerado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDumbOpponentAIIsDeterministicTest,
	"BattleSquare.DumbOpponentAI.SameSeedProducesSameCommit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDumbOpponentAIIsDeterministicTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakeTestState(1, 1);

	FBattleRandom RandomA;
	RandomA.State = 999ULL;
	FBattleRandom RandomB;
	RandomB.State = 999ULL;

	const FTurnCommit CommitA = FDumbOpponentAI::GenerateRandomValidCommit(State, 1, RandomA);
	const FTurnCommit CommitB = FDumbOpponentAI::GenerateRandomValidCommit(State, 1, RandomB);

	bool bIdentical = true;
	for (int32 Index = 0; Index < FTurnCommit::ActionsPerTurn; ++Index)
	{
		if (CommitA.Actions[Index].Type != CommitB.Actions[Index].Type
			|| CommitA.Actions[Index].Direction != CommitB.Actions[Index].Direction)
		{
			AddError(FString::Printf(TEXT("Slot %d diverge entre as duas execuções"), Index));
			bIdentical = false;
		}
	}

	return bIdentical;
}

// T5: usa exclusivamente FBattleRandom — nenhuma chamada a FMath::Rand
// ou fonte de aleatoriedade fora do estado da batalha (auditado por
// varredura de texto, mesmo espírito do audit_determinism.sh).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDumbOpponentAIUsesOnlyBattleRandomTest,
	"BattleSquare.DumbOpponentAI.UsesOnlyBattleRandom",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDumbOpponentAIUsesOnlyBattleRandomTest::RunTest(const FString& Parameters)
{
	const FString SourcePath = FPaths::ProjectDir() / TEXT("Source/BattleSquare/Private/Battle/DumbOpponentAI.cpp");
	FString SourceContents;
	if (!TestTrue(TEXT("Consegue ler o próprio arquivo fonte para auditoria"), FFileHelper::LoadFileToString(SourceContents, *SourcePath)))
	{
		return false;
	}

	TestFalse(TEXT("Nenhuma chamada a FMath::Rand"), SourceContents.Contains(TEXT("FMath::Rand")));
	TestFalse(TEXT("Nenhum uso de FRandomStream"), SourceContents.Contains(TEXT("FRandomStream")));

	return true;
}

// T5: FTurnCommit gerado é consumível pelo resolvedor real, junto com
// um commit de jogador real (fila) — ponta a ponta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDumbOpponentAIFeedsRealResolverTest,
	"BattleSquare.DumbOpponentAI.GeneratedCommitFeedsRealResolver",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDumbOpponentAIFeedsRealResolverTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	FPetState LeftPet;
	LeftPet.PetId = 1; LeftPet.Side = 0; LeftPet.Column = 1; LeftPet.Row = 1;
	LeftPet.Health = 50; LeftPet.MaxHealth = 50; LeftPet.Attack = 10; LeftPet.Defense = 5;
	FPetState RightPet;
	RightPet.PetId = 2; RightPet.Side = 1; RightPet.Column = 2; RightPet.Row = 1;
	RightPet.Health = 50; RightPet.MaxHealth = 50; RightPet.Attack = 10; RightPet.Defense = 5;
	State.Pets.Add(LeftPet);
	State.Pets.Add(RightPet);
	State.Random.State = 42ULL;

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

	const FTurnCommit RightCommit = FDumbOpponentAI::GenerateRandomValidCommit(State, /*Side=*/1, State.Random);

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, FBattleResolver::DuelCommits(State, LeftCommit, RightCommit));
	TestTrue(TEXT("Resolvedor real aceitou o commit gerado pela IA"), Result.Trace.Num() > 0);

	return true;
}
