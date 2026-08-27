// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleActionQueueComponent.h"

bool UBattleActionQueueComponent::BeginSelectingType(EActionType Type)
{
	if (bCommitted || ConfirmedActions.Num() >= FTurnCommit::ActionsPerTurn)
	{
		return false;
	}

	if (BattleActionRequiresDirection(Type))
	{
		// Passo 2 — ainda não entra na fila, só registra a intenção.
		Pending.Step = EBattleActionSelectionStep::ChoosingDirection;
		Pending.SelectedType = Type;
		OnQueueChanged.Broadcast();
		return true;
	}

	// Defender/Esquivar/Aguardar: confirma na hora, sem passar por direção.
	FBattleAction Action;
	Action.Type = Type;
	Action.Direction = EBattleDirection::Nenhuma;
	ConfirmedActions.Add(Action);
	Pending.Step = EBattleActionSelectionStep::ChoosingType;
	OnQueueChanged.Broadcast();
	return true;
}

bool UBattleActionQueueComponent::ConfirmDirection(EBattleDirection Direction)
{
	if (bCommitted || Pending.Step != EBattleActionSelectionStep::ChoosingDirection)
	{
		return false;
	}

	FBattleAction Action;
	Action.Type = Pending.SelectedType;
	Action.Direction = Direction;
	ConfirmedActions.Add(Action);

	Pending.Step = EBattleActionSelectionStep::ChoosingType;
	Pending.SelectedType = EActionType::Aguardar;
	OnQueueChanged.Broadcast();
	return true;
}

bool UBattleActionQueueComponent::CancelPendingSelection()
{
	if (bCommitted || Pending.Step != EBattleActionSelectionStep::ChoosingDirection)
	{
		return false;
	}

	// Só desfaz a INTENÇÃO pendente — nada foi adicionado a
	// ConfirmedActions ainda nesse ponto, então não há o que reverter lá.
	Pending.Step = EBattleActionSelectionStep::ChoosingType;
	Pending.SelectedType = EActionType::Aguardar;
	OnQueueChanged.Broadcast();
	return true;
}

bool UBattleActionQueueComponent::RemoveLastAction()
{
	if (bCommitted || ConfirmedActions.IsEmpty())
	{
		return false;
	}

	ConfirmedActions.Pop();
	OnQueueChanged.Broadcast();
	return true;
}

bool UBattleActionQueueComponent::Commit()
{
	if (bCommitted)
	{
		return false;
	}

	// Preenche o resto com Aguardar antes de travar (BTL-01/BTL-20).
	while (ConfirmedActions.Num() < FTurnCommit::ActionsPerTurn)
	{
		FBattleAction WaitAction;
		WaitAction.Type = EActionType::Aguardar;
		WaitAction.Direction = EBattleDirection::Nenhuma;
		ConfirmedActions.Add(WaitAction);
	}

	bCommitted = true;
	Pending.Step = EBattleActionSelectionStep::ChoosingType;
	OnCommitted.Broadcast();
	return true;
}

FTurnCommit UBattleActionQueueComponent::BuildCommit() const
{
	FTurnCommit Commit;
	for (int32 Index = 0; Index < FTurnCommit::ActionsPerTurn; ++Index)
	{
		Commit.Actions[Index] = ConfirmedActions.IsValidIndex(Index) ? ConfirmedActions[Index] : FBattleAction();
	}
	return Commit;
}

void UBattleActionQueueComponent::BeginNewTurn()
{
	ConfirmedActions.Reset();
	Pending = FBattlePendingActionSelection();
	bCommitted = false;

	// Um só broadcast, no fim: quem escuta vê o turno novo já inteiro, nunca
	// um estado pela metade.
	OnQueueChanged.Broadcast();
}

void UBattleActionQueueComponent::RestoreConfirmedActions(const TArray<FBattleAction>& Actions)
{
	ConfirmedActions = Actions;

	// Nunca acima do teto do turno, mesmo que o rascunho venha corrompido: o
	// núcleo resolve exatamente três ações.
	while (ConfirmedActions.Num() > FTurnCommit::ActionsPerTurn)
	{
		ConfirmedActions.Pop();
	}

	Pending = FBattlePendingActionSelection();
	bCommitted = false;

	OnQueueChanged.Broadcast();
}
