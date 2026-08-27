// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleActionSelectorWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Battle/BattleGridNavigation.h"

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

	RefreshLayoutFromState();
}

void UBattleActionSelectorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	WireButtons();
	RefreshLayoutFromState();
}

void UBattleActionSelectorWidget::WireButtons()
{
	// Cada botão encaminha, nada decide (DP-ui-01). AddDynamic exige o
	// nome da função literal, então a lista é explícita de propósito.
	if (Button_Aguardar) { Button_Aguardar->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickAguardar); }
	if (Button_Mover) { Button_Mover->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickMover); }
	if (Button_Atacar) { Button_Atacar->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickAtacar); }
	if (Button_Magia) { Button_Magia->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickMagia); }
	if (Button_Defender) { Button_Defender->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickDefender); }
	if (Button_Esquivar) { Button_Esquivar->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickEsquivar); }
	if (Button_CimaEsquerda) { Button_CimaEsquerda->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickCimaEsquerda); }
	if (Button_Cima) { Button_Cima->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickCima); }
	if (Button_CimaDireita) { Button_CimaDireita->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickCimaDireita); }
	if (Button_Esquerda) { Button_Esquerda->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickEsquerda); }
	if (Button_Direita) { Button_Direita->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickDireita); }
	if (Button_BaixoEsquerda) { Button_BaixoEsquerda->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickBaixoEsquerda); }
	if (Button_Baixo) { Button_Baixo->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickBaixo); }
	if (Button_BaixoDireita) { Button_BaixoDireita->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickBaixoDireita); }
	if (Button_Cancelar) { Button_Cancelar->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickCancelar); }
	if (Button_Desfazer) { Button_Desfazer->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickDesfazer); }
	if (Button_Commit) { Button_Commit->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickCommit); }
}

void UBattleActionSelectorWidget::RefreshLayoutFromState()
{
	const bool bChoosingDirection = (CurrentStep == EBattleActionSelectionStep::ChoosingDirection);

	// Mostrar os dois passos ao mesmo tempo convidaria a clicar no que não
	// vale agora — e a recusa viria do componente, sem explicação na tela.
	UButton* const TypeButtons[] = { Button_Aguardar, Button_Mover, Button_Atacar,
		Button_Magia, Button_Defender, Button_Esquivar };
	for (UButton* Button : TypeButtons)
	{
		if (Button)
		{
			Button->SetVisibility(bChoosingDirection ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
	}

	UButton* const DirectionButtons[] = { Button_CimaEsquerda, Button_Cima, Button_CimaDireita,
		Button_Esquerda, Button_Direita, Button_BaixoEsquerda, Button_Baixo, Button_BaixoDireita,
		Button_Cancelar };
	for (UButton* Button : DirectionButtons)
	{
		if (Button)
		{
			Button->SetVisibility(bChoosingDirection ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	if (Button_Commit)
	{
		Button_Commit->SetIsEnabled(!bIsCommitted && !bChoosingDirection);
	}
	if (Button_Desfazer)
	{
		Button_Desfazer->SetIsEnabled(!bIsCommitted && ConfirmedActionCount > 0 && !bChoosingDirection);
	}

	RefreshDirectionAvailability();

	if (Text_Status)
	{
		const FText Status = bIsCommitted
			? NSLOCTEXT("BattleUI", "Committed", "Turno fechado — aguardando o oponente")
			: (bChoosingDirection
				? NSLOCTEXT("BattleUI", "ChooseDirection", "Escolha a direção")
				: FText::Format(NSLOCTEXT("BattleUI", "ChooseType", "Ações: {0} de 3 — escolha o tipo"),
					FText::AsNumber(ConfirmedActionCount)));
		Text_Status->SetText(Status);
	}
}

void UBattleActionSelectorWidget::OnClickAguardar() { BeginSelectingType(EActionType::Aguardar); }
void UBattleActionSelectorWidget::OnClickMover()    { BeginSelectingType(EActionType::Mover); }
void UBattleActionSelectorWidget::OnClickAtacar()   { BeginSelectingType(EActionType::Atacar); }
void UBattleActionSelectorWidget::OnClickMagia()    { BeginSelectingType(EActionType::Magia); }
void UBattleActionSelectorWidget::OnClickDefender() { BeginSelectingType(EActionType::Defender); }
void UBattleActionSelectorWidget::OnClickEsquivar() { BeginSelectingType(EActionType::Esquivar); }

void UBattleActionSelectorWidget::OnClickCimaEsquerda()  { ConfirmDirection(EBattleDirection::CimaEsquerda); }
void UBattleActionSelectorWidget::OnClickCima()          { ConfirmDirection(EBattleDirection::Cima); }
void UBattleActionSelectorWidget::OnClickCimaDireita()   { ConfirmDirection(EBattleDirection::CimaDireita); }
void UBattleActionSelectorWidget::OnClickEsquerda()      { ConfirmDirection(EBattleDirection::Esquerda); }
void UBattleActionSelectorWidget::OnClickDireita()       { ConfirmDirection(EBattleDirection::Direita); }
void UBattleActionSelectorWidget::OnClickBaixoEsquerda() { ConfirmDirection(EBattleDirection::BaixoEsquerda); }
void UBattleActionSelectorWidget::OnClickBaixo()         { ConfirmDirection(EBattleDirection::Baixo); }
void UBattleActionSelectorWidget::OnClickBaixoDireita()  { ConfirmDirection(EBattleDirection::BaixoDireita); }

void UBattleActionSelectorWidget::OnClickCancelar() { CancelPendingSelection(); }
void UBattleActionSelectorWidget::OnClickDesfazer() { RemoveLastAction(); }
void UBattleActionSelectorWidget::OnClickCommit()   { Commit(); }

void UBattleActionSelectorWidget::SetOwningPetCell(uint8 Column, uint8 Row)
{
	OwningPetColumn = Column;
	OwningPetRow = Row;
	RefreshDirectionAvailability();
}

void UBattleActionSelectorWidget::RefreshDirectionAvailability()
{
	// Só MOVER sai do tabuleiro. Atacar ou lançar magia para fora é decisão
	// do núcleo (erra, não sai) — desabilitar aqui inventaria uma regra.
	const bool bIsMoving = BoundQueue
		&& CurrentStep == EBattleActionSelectionStep::ChoosingDirection
		&& BoundQueue->GetPendingType() == EActionType::Mover;

	uint8 ProjectedColumn = OwningPetColumn;
	uint8 ProjectedRow = OwningPetRow;
	if (BoundQueue)
	{
		FBattleGridNavigation::ProjectCell(OwningPetColumn, OwningPetRow,
			BoundQueue->GetConfirmedActions(), ProjectedColumn, ProjectedRow);
	}

	const TPair<UButton*, EBattleDirection> Direcoes[] = {
		{ Button_CimaEsquerda,  EBattleDirection::CimaEsquerda },
		{ Button_Cima,          EBattleDirection::Cima },
		{ Button_CimaDireita,   EBattleDirection::CimaDireita },
		{ Button_Esquerda,      EBattleDirection::Esquerda },
		{ Button_Direita,       EBattleDirection::Direita },
		{ Button_BaixoEsquerda, EBattleDirection::BaixoEsquerda },
		{ Button_Baixo,         EBattleDirection::Baixo },
		{ Button_BaixoDireita,  EBattleDirection::BaixoDireita },
	};

	for (const TPair<UButton*, EBattleDirection>& Entrada : Direcoes)
	{
		if (!Entrada.Key)
		{
			continue;
		}
		const bool bBloqueado = bIsMoving
			&& FBattleGridNavigation::WouldLeaveGrid(ProjectedColumn, ProjectedRow, Entrada.Value);
		Entrada.Key->SetIsEnabled(!bBloqueado);
	}
}
