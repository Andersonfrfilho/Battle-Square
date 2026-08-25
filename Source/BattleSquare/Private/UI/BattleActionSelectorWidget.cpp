// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleActionSelectorWidget.h"

void UBattleActionSelectorWidget::BindToQueue(UBattleActionQueueComponent* Queue)
{
	if (BoundQueue == Queue)
	{
		return;
	}

	if (BoundQueue)
	{
		BoundQueue->OnQueueChanged.RemoveDynamic(this, &UBattleActionSelectorWidget::RefreshStateFromQueue);
		BoundQueue->OnCommitted.RemoveDynamic(this, &UBattleActionSelectorWidget::RefreshStateFromQueue);
	}

	BoundQueue = Queue;

	if (BoundQueue)
	{
		BoundQueue->OnQueueChanged.AddDynamic(this, &UBattleActionSelectorWidget::RefreshStateFromQueue);
		BoundQueue->OnCommitted.AddDynamic(this, &UBattleActionSelectorWidget::RefreshStateFromQueue);
	}

	RefreshStateFromQueue();
}

bool UBattleActionSelectorWidget::BeginSelectingType(EActionType Type)
{
	return BoundQueue ? BoundQueue->BeginSelectingType(Type) : false;
}

bool UBattleActionSelectorWidget::ConfirmDirection(EBattleDirection Direction)
{
	return BoundQueue ? BoundQueue->ConfirmDirection(Direction) : false;
}

bool UBattleActionSelectorWidget::CancelPendingSelection()
{
	return BoundQueue ? BoundQueue->CancelPendingSelection() : false;
}

bool UBattleActionSelectorWidget::RemoveLastAction()
{
	return BoundQueue ? BoundQueue->RemoveLastAction() : false;
}

bool UBattleActionSelectorWidget::Commit()
{
	return BoundQueue ? BoundQueue->Commit() : false;
}

void UBattleActionSelectorWidget::RefreshStateFromQueue()
{
	if (!BoundQueue)
	{
		CurrentStep = EBattleActionSelectionStep::ChoosingType;
		ConfirmedActionCount = 0;
		bIsCommitted = false;
		return;
	}

	CurrentStep = BoundQueue->GetCurrentStep();
	ConfirmedActionCount = BoundQueue->GetConfirmedActionCount();
	bIsCommitted = BoundQueue->IsCommitted();
}
