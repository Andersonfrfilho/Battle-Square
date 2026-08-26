// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleNetCommitComponent.h"

UBattleNetCommitComponent::UBattleNetCommitComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBattleNetCommitComponent::SetServerCoordinator(UBattleTurnCoordinator* InCoordinator, uint8 InSide)
{
	Coordinator = InCoordinator;
	Side = InSide;

	if (Coordinator)
	{
		Coordinator->OnTurnResolved.AddUObject(this, &UBattleNetCommitComponent::HandleCoordinatorResolved);
	}
}

void UBattleNetCommitComponent::SubmitLocalCommit(const FTurnCommit& Commit)
{
	Server_SubmitCommit(ToNetTurnCommit(Commit));
}

bool UBattleNetCommitComponent::ValidateIncomingCommit(const FNetTurnCommit& Commit) const
{
	return ValidateNetTurnCommit(Commit);
}

void UBattleNetCommitComponent::ProcessValidatedCommit(const FNetTurnCommit& Commit)
{
	if (!Coordinator)
	{
		return;
	}

	Coordinator->SubmitCommit(Side, ToTurnCommit(Commit));
}

bool UBattleNetCommitComponent::Server_SubmitCommit_Validate(FNetTurnCommit Commit)
{
	return ValidateIncomingCommit(Commit);
}

void UBattleNetCommitComponent::Server_SubmitCommit_Implementation(FNetTurnCommit Commit)
{
	ProcessValidatedCommit(Commit);
}

void UBattleNetCommitComponent::Client_ReceiveTurnResult_Implementation(FBattleState NextState, const TArray<FBattleEvent>& Trace)
{
	OnResultReceived.Broadcast(NextState, Trace);
}

void UBattleNetCommitComponent::HandleCoordinatorResolved(const FBattleState& NextState, const TArray<FBattleEvent>& Trace)
{
	Client_ReceiveTurnResult(NextState, Trace);
}
