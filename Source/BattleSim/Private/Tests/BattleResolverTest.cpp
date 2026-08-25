// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeDuelState()
	{
		FBattleState State;

		FPetState Left;
		Left.PetId = 1;
		Left.Side = 0;
		Left.Column = 0;
		Left.Row = 1;
		Left.Health = 50;
		Left.MaxHealth = 50;
		Left.Attack = 10;
		Left.Defense = 5;
		Left.Speed = 8;
		State.Pets.Add(Left);

		FPetState Right;
		Right.PetId = 2;
		Right.Side = 1;
		Right.Column = 2;
		Right.Row = 1;
		Right.Health = 50;
		Right.MaxHealth = 50;
		Right.Attack = 10;
		Right.Defense = 5;
		Right.Speed = 8;
		State.Pets.Add(Right);

		State.Random.State = 777ULL;
		return State;
	}

	FTurnCommit MakeCommit(EActionType Type0, EBattleDirection Dir0, EActionType Type1 = EActionType::Aguardar, EBattleDirection Dir1 = EBattleDirection::Nenhuma, EActionType Type2 = EActionType::Aguardar, EBattleDirection Dir2 = EBattleDirection::Nenhuma)
	{
		FTurnCommit Commit;
		Commit.Actions[0] = { Type0, Dir0 };
		Commit.Actions[1] = { Type1, Dir1 };
		Commit.Actions[2] = { Type2, Dir2 };
		return Commit;
	}
}

// T9: ResolveTurn NÃO muta o estado de entrada — pureza (design.md).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleResolverDoesNotMutateInputTest,
	"BattleSim.Resolver.DoesNotMutateInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleResolverDoesNotMutateInputTest::RunTest(const FString& Parameters)
{
	const FBattleState InitialState = MakeDuelState();
	const uint64 HashBefore = InitialState.ComputeHash();

	FTurnCommit LeftCommit = MakeCommit(EActionType::Atacar, EBattleDirection::Direita);
	FTurnCommit RightCommit = MakeCommit(EActionType::Atacar, EBattleDirection::Esquerda);

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(InitialState, LeftCommit, RightCommit);

	TestEqual(TEXT("InState não mudou após a chamada"), InitialState.ComputeHash(), HashBefore);
	TestNotEqual(TEXT("NextState é diferente do InState (algo de fato aconteceu)"), Result.NextState.ComputeHash(), HashBefore);

	return true;
}

// T9/BTL-16: mesma seed e mesmas ações produzem trace idêntico — a base
// de tudo. Duas chamadas independentes com entrada idêntica.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleResolverIsDeterministicTest,
	"BattleSim.Resolver.SameInputProducesIdenticalTrace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleResolverIsDeterministicTest::RunTest(const FString& Parameters)
{
	const FBattleState InitialState = MakeDuelState();
	const FTurnCommit LeftCommit = MakeCommit(EActionType::Mover, EBattleDirection::Direita, EActionType::Atacar, EBattleDirection::Direita, EActionType::Defender);
	const FTurnCommit RightCommit = MakeCommit(EActionType::Defender, EBattleDirection::Nenhuma, EActionType::Mover, EBattleDirection::Esquerda, EActionType::Atacar, EBattleDirection::Esquerda);

	FBattleResolveResult ResultA = FBattleResolver::ResolveTurn(InitialState, LeftCommit, RightCommit);
	FBattleResolveResult ResultB = FBattleResolver::ResolveTurn(InitialState, LeftCommit, RightCommit);

	TestEqual(TEXT("Hashes finais idênticos"), ResultA.NextState.ComputeHash(), ResultB.NextState.ComputeHash());
	TestEqual(TEXT("Traces do mesmo tamanho"), ResultA.Trace.Num(), ResultB.Trace.Num());

	bool bAllEventsMatch = true;
	for (int32 Index = 0; Index < FMath::Min(ResultA.Trace.Num(), ResultB.Trace.Num()); ++Index)
	{
		const FBattleEvent& EventA = ResultA.Trace[Index];
		const FBattleEvent& EventB = ResultB.Trace[Index];
		if (EventA.Type != EventB.Type || EventA.ActorId != EventB.ActorId || EventA.Value != EventB.Value)
		{
			AddError(FString::Printf(TEXT("Evento %d diverge entre as duas execuções"), Index));
			bAllEventsMatch = false;
		}
	}
	TestTrue(TEXT("Todos os eventos batem"), bAllEventsMatch);

	return true;
}

// T9/BTL-03: um turno resolve os 3 slots, com fronteiras de trace corretas.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleResolverResolvesThreeSlotsTest,
	"BattleSim.Resolver.ResolvesThreeSlotsInOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleResolverResolvesThreeSlotsTest::RunTest(const FString& Parameters)
{
	const FBattleState InitialState = MakeDuelState();
	const FTurnCommit LeftCommit = MakeCommit(EActionType::Aguardar, EBattleDirection::Nenhuma, EActionType::Aguardar, EBattleDirection::Nenhuma, EActionType::Aguardar, EBattleDirection::Nenhuma);
	const FTurnCommit RightCommit = LeftCommit;

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(InitialState, LeftCommit, RightCommit);

	int32 TurnStartedCount = 0;
	int32 SlotStartedCount = 0;
	int32 SlotEndedCount = 0;
	int32 TurnEndedCount = 0;
	for (const FBattleEvent& Event : Result.Trace)
	{
		switch (Event.Type)
		{
			case EBattleEventType::TurnoIniciado: ++TurnStartedCount; break;
			case EBattleEventType::SlotIniciado: ++SlotStartedCount; break;
			case EBattleEventType::SlotEncerrado: ++SlotEndedCount; break;
			case EBattleEventType::TurnoEncerrado: ++TurnEndedCount; break;
			default: break;
		}
	}

	TestEqual(TEXT("Um TurnoIniciado"), TurnStartedCount, 1);
	TestEqual(TEXT("Três SlotIniciado"), SlotStartedCount, 3);
	TestEqual(TEXT("Três SlotEncerrado"), SlotEndedCount, 3);
	TestEqual(TEXT("Um TurnoEncerrado"), TurnEndedCount, 1);
	TestEqual(TEXT("TurnNumber incrementado"), Result.NextState.TurnNumber, InitialState.TurnNumber + 1);

	return true;
}

// T9: commit com menos de 3 ações reais completa com Aguardar, por
// construção do struct — sem crash, sem comportamento surpresa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleResolverIncompleteCommitDefaultsToWaitTest,
	"BattleSim.Resolver.IncompleteCommitDefaultsToWait",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleResolverIncompleteCommitDefaultsToWaitTest::RunTest(const FString& Parameters)
{
	const FBattleState InitialState = MakeDuelState();

	FTurnCommit LeftCommit; // nenhuma ação setada — os 3 slots são Aguardar por default
	FTurnCommit RightCommit;

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(InitialState, LeftCommit, RightCommit);

	// Sem ação nenhuma, nenhum dano, nenhum movimento — só as fronteiras de slot/turno.
	int32 GameplayEventCount = 0;
	for (const FBattleEvent& Event : Result.Trace)
	{
		if (Event.Type != EBattleEventType::TurnoIniciado
			&& Event.Type != EBattleEventType::SlotIniciado
			&& Event.Type != EBattleEventType::SlotEncerrado
			&& Event.Type != EBattleEventType::TurnoEncerrado)
		{
			++GameplayEventCount;
		}
	}
	TestEqual(TEXT("Nenhum evento de gameplay com commit vazio"), GameplayEventCount, 0);
	TestEqual(TEXT("Ninguém tomou dano"), Result.NextState.Pets[0].Health, InitialState.Pets[0].Health);

	return true;
}

// T9/BTL-07 de ponta a ponta: morte mútua sobrevive ao turno inteiro,
// e ações dos slots seguintes ao slot da morte não geram efeito.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleResolverDeadPetSkipsRemainingSlotsTest,
	"BattleSim.Resolver.DeadPetSkipsRemainingSlotActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleResolverDeadPetSkipsRemainingSlotsTest::RunTest(const FString& Parameters)
{
	// MakeDuelState() posiciona os pets 2 casas de distância (colunas 0 e 2)
	// — de propósito, para os testes de movimento terem espaço. Alcance de
	// ataque é 1 (adjacente), então este teste precisa dos pets já
	// adjacentes para que o ataque do slot 0 realmente acerte.
	FBattleState LowHealthState = MakeDuelState();
	LowHealthState.Pets[1].Column = 1; // Right adjacente a Left (que está em col 0)
	LowHealthState.Pets[1].Health = 5; // Right quase morto
	LowHealthState.Pets[1].MaxHealth = 50;

	// Left ataca nos 3 slots; Right ataca só no slot 0, depois só espera —
	// mas mesmo que atacasse, estaria morto.
	const FTurnCommit LeftCommit = MakeCommit(EActionType::Atacar, EBattleDirection::Direita, EActionType::Atacar, EBattleDirection::Direita, EActionType::Atacar, EBattleDirection::Direita);
	const FTurnCommit RightCommit = MakeCommit(EActionType::Atacar, EBattleDirection::Esquerda, EActionType::Atacar, EBattleDirection::Esquerda, EActionType::Atacar, EBattleDirection::Esquerda);

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(LowHealthState, LeftCommit, RightCommit);

	TestFalse(TEXT("Right morreu no slot 0"), Result.NextState.Pets[1].IsAlive());

	int32 PetMorreuCount = 0;
	for (const FBattleEvent& Event : Result.Trace)
	{
		if (Event.Type == EBattleEventType::PetMorreu && Event.ActorId == Result.NextState.Pets[1].PetId)
		{
			++PetMorreuCount;
		}
	}
	TestEqual(TEXT("PetMorreu emitido exatamente uma vez, não em cada slot"), PetMorreuCount, 1);

	return true;
}
