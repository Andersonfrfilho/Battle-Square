// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleActionQueueComponent.h"

bool UBattleActionQueueComponent::BeginSelectingType(EActionType Type)
{
	// Skill que este pet não tem é RECUSADA aqui, não escondida na tela.
	if (!IsActionAvailable(Type))
	{
		return false;
	}

	if (bCommitted || ConfirmedActions.Num() >= FTurnCommit::ActionsPerTurn)
	{
		return false;
	}

	// Mover pede direção; Atacar e Magia pedem GOLPE. Os dois abrem o mesmo
	// passo 2 — o que muda é a pergunta que a tela faz, e qual função
	// confirma.
	if (BattleActionRequiresDirection(Type) || BattleActionRequiresMove(Type))
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

	// Ataque não se confirma por direção desde DP-golpe-05: aceitar aqui
	// gravaria uma direção no byte que o resolvedor lê como índice de golpe.
	if (!BattleActionRequiresDirection(Pending.SelectedType))
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

void UBattleActionQueueComponent::SetAvailableActions(const TArray<EActionType>& Actions)
{
	AvailableActions = Actions;
}

bool UBattleActionQueueComponent::IsActionAvailable(EActionType Type) const
{
	// Vazio = sem restrição. É o que mantém intacta toda batalha que não
	// configura skills, em vez de deixá-las sem ação nenhuma.
	return AvailableActions.IsEmpty() || AvailableActions.Contains(Type);
}

void UBattleActionQueueComponent::SetUnlockedMoves(const TArray<bool>& Unlocked)
{
	UnlockedMoves = Unlocked;
}

bool UBattleActionQueueComponent::IsMoveUnlocked(int32 MoveIndex) const
{
	// Fora da lista é DESTRANCADO, não trancado: pet com dois golpes
	// cadastrados não pode ter o segundo recusado por a lista ter um item a
	// menos, e lista vazia precisa continuar significando "sem restrição".
	return !UnlockedMoves.IsValidIndex(MoveIndex) || UnlockedMoves[MoveIndex];
}

bool UBattleActionQueueComponent::ConfirmMove(int32 MoveIndex)
{
	if (bCommitted || Pending.Step != EBattleActionSelectionStep::ChoosingDirection)
	{
		return false;
	}

	if (!BattleActionRequiresMove(Pending.SelectedType))
	{
		return false;
	}

	// Índice fora da faixa é RECUSADO, não corrigido para zero: aceitar
	// silenciosamente faria o jogador executar um golpe que não escolheu.
	if (MoveIndex < 0 || MoveIndex >= BattleMovesPerPet)
	{
		return false;
	}

	// Golpe trancado é RECUSADO aqui, não escondido lá. A tela some com o
	// botão para quem joga; esta linha é o que segura quem não joga pela tela.
	if (!IsMoveUnlocked(MoveIndex))
	{
		return false;
	}

	ConfirmedActions.Add(MakeMoveAction(Pending.SelectedType, static_cast<uint8>(MoveIndex)));
	Pending = FBattlePendingActionSelection();

	OnQueueChanged.Broadcast();
	return true;
}
