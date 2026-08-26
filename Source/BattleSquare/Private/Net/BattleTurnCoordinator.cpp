// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleTurnCoordinator.h"
#include "Net/BattleNetConstants.h"
#include "Battle/BattleResolver.h"

void UBattleTurnCoordinator::BeginTurn(const FBattleState& State, double StartTimeSeconds)
{
	CurrentState = State;
	TurnStartTimeSeconds = StartTimeSeconds;
	bResolved = false;
	bHasCommitSide0 = false;
	bHasCommitSide1 = false;
}

bool UBattleTurnCoordinator::SubmitCommit(uint8 Side, const FTurnCommit& Commit)
{
	if (bResolved)
	{
		return false;
	}

	if (Side == 0)
	{
		if (bHasCommitSide0)
		{
			return false;
		}
		PendingCommitSide0 = Commit;
		bHasCommitSide0 = true;
	}
	else if (Side == 1)
	{
		if (bHasCommitSide1)
		{
			return false;
		}
		PendingCommitSide1 = Commit;
		bHasCommitSide1 = true;
	}
	else
	{
		return false;
	}

	// Um commit que chega, mesmo que muito perto do timeout, resolve na
	// hora se o outro lado já estiver presente — nunca fica esperando o
	// próximo CheckTimeout (T6: garante que commit tardio-mas-real nunca
	// perde para o preenchimento automático).
	TryResolveIfBothPresent();
	return true;
}

void UBattleTurnCoordinator::CheckTimeout(double CurrentTimeSeconds)
{
	if (bResolved)
	{
		return;
	}

	const double ElapsedSeconds = CurrentTimeSeconds - TurnStartTimeSeconds;
	if (ElapsedSeconds < static_cast<double>(BattleNetConstants::CommitTimeoutSeconds))
	{
		return;
	}

	// Timeout estourou: preenche o(s) lado(s) ausente(s) com 3x Aguardar
	// (mesma regra de UBattleActionQueueComponent::Commit) e resolve.
	if (!bHasCommitSide0)
	{
		PendingCommitSide0 = MakeWaitOnlyCommit();
		bHasCommitSide0 = true;
	}
	if (!bHasCommitSide1)
	{
		PendingCommitSide1 = MakeWaitOnlyCommit();
		bHasCommitSide1 = true;
	}

	TryResolveIfBothPresent();
}

void UBattleTurnCoordinator::TryResolveIfBothPresent()
{
	if (bResolved || !bHasCommitSide0 || !bHasCommitSide1)
	{
		return;
	}

	ResolveWithCommits(PendingCommitSide0, PendingCommitSide1);
}

void UBattleTurnCoordinator::ResolveWithCommits(const FTurnCommit& CommitSide0, const FTurnCommit& CommitSide1)
{
	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(CurrentState, CommitSide0, CommitSide1);
	bResolved = true;
	OnTurnResolved.Broadcast(Result.NextState, Result.Trace);
}

FTurnCommit UBattleTurnCoordinator::MakeWaitOnlyCommit()
{
	FTurnCommit Commit;
	Commit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Commit.Actions[1] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Commit.Actions[2] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	return Commit;
}

void UBattleTurnCoordinator::DeclareAbandonment(uint8 PresentSide)
{
	OnAbandonment.Broadcast(MakeAbandonmentEvent(PresentSide));
}

FBattleEvent UBattleTurnCoordinator::MakeAbandonmentEvent(uint8 PresentSide)
{
	FBattleEvent Event;
	Event.Type = EBattleEventType::BatalhaEncerrada;
	Event.SlotIndex = 0;
	Event.Phase = 0;
	Event.ActorId = BattleEventNoActor;
	Event.TargetId = BattleEventNoActor;
	Event.Value = static_cast<int32>(PresentSide);
	return Event;
}
