// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleTracePlayer.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeDuelState()
	{
		FBattleState State;
		FPetState Left;
		Left.PetId = 1; Left.Side = 0; Left.Column = 1; Left.Row = 1;
		Left.Health = 50; Left.MaxHealth = 50; Left.Attack = 20; Left.Defense = 5;
		FPetState Right;
		Right.PetId = 2; Right.Side = 1; Right.Column = 2; Right.Row = 1;
		Right.Health = 50; Right.MaxHealth = 50; Right.Attack = 10; Right.Defense = 5;
		State.Pets.Add(Left);
		State.Pets.Add(Right);
		return State;
	}
}

// T6: eventos do mesmo (SlotIndex, Phase) caem no mesmo grupo; fases
// diferentes do mesmo slot ficam em grupos separados. Usa um trace REAL
// produzido por FBattleResolver::ResolveTurn — não um trace inventado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTracePlayerGroupsRealTraceTest,
	"BattleSquare.TracePlayer.GroupEventsByPhaseOnRealTrace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTracePlayerGroupsRealTraceTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakeDuelState();

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
	LeftCommit.Actions[1] = { EActionType::Defender, EBattleDirection::Nenhuma };
	LeftCommit.Actions[2] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

	FTurnCommit RightCommit;
	RightCommit.Actions[0] = { EActionType::Esquivar, EBattleDirection::Nenhuma };
	RightCommit.Actions[1] = { EActionType::Atacar, EBattleDirection::Esquerda };
	RightCommit.Actions[2] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit);
	TestTrue(TEXT("Trace real tem eventos"), Result.Trace.Num() > 0);

	const TArray<TArray<FBattleEvent>> Groups = GroupBattleEventsByPhase(Result.Trace);

	// Soma dos eventos de todos os grupos == eventos do trace original —
	// nenhum evento perdido ou duplicado no agrupamento.
	int32 TotalGroupedEvents = 0;
	for (const TArray<FBattleEvent>& Group : Groups)
	{
		TotalGroupedEvents += Group.Num();
	}
	TestEqual(TEXT("Nenhum evento perdido ou duplicado"), TotalGroupedEvents, Result.Trace.Num());

	// Cada grupo é internamente consistente: todos os eventos do grupo
	// têm o MESMO SlotIndex e Phase.
	bool bAllGroupsConsistent = true;
	for (const TArray<FBattleEvent>& Group : Groups)
	{
		if (Group.IsEmpty()) { continue; }
		const uint8 GroupSlot = Group[0].SlotIndex;
		const uint8 GroupPhase = Group[0].Phase;
		for (const FBattleEvent& Event : Group)
		{
			if (Event.SlotIndex != GroupSlot || Event.Phase != GroupPhase)
			{
				bAllGroupsConsistent = false;
			}
		}
	}
	TestTrue(TEXT("Todo grupo é internamente consistente (mesmo Slot+Phase)"), bAllGroupsConsistent);

	// Ordem preservada: concatenar os grupos de volta reproduz o trace original.
	TArray<FBattleEvent> Flattened;
	for (const TArray<FBattleEvent>& Group : Groups)
	{
		Flattened.Append(Group);
	}
	bool bOrderPreserved = (Flattened.Num() == Result.Trace.Num());
	for (int32 Index = 0; bOrderPreserved && Index < Flattened.Num(); ++Index)
	{
		if (Flattened[Index].Type != Result.Trace[Index].Type || Flattened[Index].ActorId != Result.Trace[Index].ActorId)
		{
			bOrderPreserved = false;
		}
	}
	TestTrue(TEXT("Ordem do trace original é preservada ao concatenar os grupos"), bOrderPreserved);

	return true;
}

// T6: trace vazio produz zero grupos, sem crash.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTracePlayerGroupsEmptyTraceTest,
	"BattleSquare.TracePlayer.GroupEventsByPhaseHandlesEmptyTrace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTracePlayerGroupsEmptyTraceTest::RunTest(const FString& Parameters)
{
	const TArray<FBattleEvent> EmptyTrace;
	const TArray<TArray<FBattleEvent>> Groups = GroupBattleEventsByPhase(EmptyTrace);
	TestTrue(TEXT("Trace vazio produz zero grupos"), Groups.IsEmpty());
	return true;
}

// T6: dois eventos genuinamente simultâneos (mesmo slot+fase, dois
// atores diferentes) caem no MESMO grupo — é a prova concreta de
// PRES-09/critério 2 (não só "não quebrou nada").
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTracePlayerGroupsSimultaneousEventsTogetherTest,
	"BattleSquare.TracePlayer.SimultaneousEventsShareOneGroup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTracePlayerGroupsSimultaneousEventsTogetherTest::RunTest(const FString& Parameters)
{
	// F2 (Postura): os dois lados podem assumir postura no mesmo slot —
	// cenário real que gera 2 eventos PosturaAssumida, mesmo Slot+Phase.
	const FBattleState State = MakeDuelState();

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Defender, EBattleDirection::Nenhuma };
	FTurnCommit RightCommit;
	RightCommit.Actions[0] = { EActionType::Esquivar, EBattleDirection::Nenhuma };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit);
	const TArray<TArray<FBattleEvent>> Groups = GroupBattleEventsByPhase(Result.Trace);

	int32 PostureEventsInSameGroup = 0;
	for (const TArray<FBattleEvent>& Group : Groups)
	{
		int32 PostureCountInThisGroup = 0;
		for (const FBattleEvent& Event : Group)
		{
			if (Event.Type == EBattleEventType::PosturaAssumida) { ++PostureCountInThisGroup; }
		}
		if (PostureCountInThisGroup > PostureEventsInSameGroup)
		{
			PostureEventsInSameGroup = PostureCountInThisGroup;
		}
	}

	TestEqual(TEXT("Os 2 eventos PosturaAssumida (Left e Right) caem no MESMO grupo"), PostureEventsInSameGroup, 2);

	return true;
}

// T7: PlayTrace despacha todos os eventos, na ordem do trace original,
// via delegate; SkipToEnd faz o mesmo sem depender de grupos anteriores.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTracePlayerPlayAndSkipTest,
	"BattleSquare.TracePlayer.PlayAndSkip",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTracePlayerPlayAndSkipTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakeDuelState();

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
	FTurnCommit RightCommit;
	RightCommit.Actions[0] = { EActionType::Esquivar, EBattleDirection::Nenhuma };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit);
	TestTrue(TEXT("Trace real tem eventos"), Result.Trace.Num() > 0);

	UBattleTracePlayer* Player = NewObject<UBattleTracePlayer>();

	TArray<FBattleEvent> PlayedEvents;
	Player->OnEventApplied.AddLambda([&PlayedEvents](const FBattleEvent& Event) { PlayedEvents.Add(Event); });
	Player->PlayTrace(Result.Trace);
	TestEqual(TEXT("PlayTrace despacha todos os eventos"), PlayedEvents.Num(), Result.Trace.Num());
	bool bPlayOrderMatches = true;
	for (int32 Index = 0; Index < PlayedEvents.Num(); ++Index)
	{
		if (PlayedEvents[Index].Type != Result.Trace[Index].Type)
		{
			bPlayOrderMatches = false;
		}
	}
	TestTrue(TEXT("PlayTrace preserva a ordem original"), bPlayOrderMatches);

	TArray<FBattleEvent> SkippedEvents;
	UBattleTracePlayer* SkipPlayer = NewObject<UBattleTracePlayer>();
	SkipPlayer->OnEventApplied.AddLambda([&SkippedEvents](const FBattleEvent& Event) { SkippedEvents.Add(Event); });
	SkipPlayer->SkipToEnd(Result.Trace);
	TestEqual(TEXT("SkipToEnd despacha todos os eventos sem exigir reprodução anterior"), SkippedEvents.Num(), Result.Trace.Num());

	return true;
}
