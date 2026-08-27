// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleActionSelectorWidget.h"
#include "Debug/BattleDebugKeys.h"
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

	// Focável, sim; FOCADO, não aqui. NativeConstruct roda na CRIAÇÃO do
	// widget, antes de AddToViewport — e widget fora da viewport não tem o
	// que focar, então o pedido virava no-op e NativeOnKeyDown nunca
	// disparava. Quem pede o foco é o GameMode, depois de adicionar.
	SetIsFocusable(true);
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
	if (Button_Camuflar) { Button_Camuflar->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickCamuflar); }
	if (Button_Voar) { Button_Voar->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickVoar); }
	if (Button_Submergir) { Button_Submergir->OnClicked.AddDynamic(this, &UBattleActionSelectorWidget::OnClickSubmergir); }
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
		Button_Magia, Button_Defender, Button_Esquivar,
		Button_Camuflar, Button_Voar, Button_Submergir };
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
		const FText Base = bIsCommitted
			? NSLOCTEXT("BattleUI", "Committed", "Turno fechado — aguardando o oponente")
			: (bChoosingDirection
				? NSLOCTEXT("BattleUI", "ChooseDirection", "Escolha a direção")
				: FText::Format(NSLOCTEXT("BattleUI", "ChooseType", "Ações: {0} de 3 — escolha o tipo"),
					FText::AsNumber(ConfirmedActionCount)));

		// O nome de quem está sendo controlado vem PRIMEIRO: no modo de
		// escolher pelos dois lados, saber por quem se está jogando decide a
		// jogada inteira, e descobrir isso depois de confirmar é tarde.
		const FText Status = ChoosingForLabel.IsEmpty()
			? Base
			: FText::Format(NSLOCTEXT("BattleUI", "StatusComLado", "{0}\n{1}"), ChoosingForLabel, Base);

		Text_Status->SetText(Status);
	}
}

void UBattleActionSelectorWidget::OnClickAguardar() { BeginSelectingType(EActionType::Aguardar); }
void UBattleActionSelectorWidget::OnClickMover()    { BeginSelectingType(EActionType::Mover); }
void UBattleActionSelectorWidget::OnClickAtacar()   { BeginSelectingType(EActionType::Atacar); }
void UBattleActionSelectorWidget::OnClickMagia()    { BeginSelectingType(EActionType::Magia); }
void UBattleActionSelectorWidget::OnClickDefender() { BeginSelectingType(EActionType::Defender); }
void UBattleActionSelectorWidget::OnClickEsquivar() { BeginSelectingType(EActionType::Esquivar); }
void UBattleActionSelectorWidget::OnClickCamuflar() { BeginSelectingType(EActionType::Camuflar); }
void UBattleActionSelectorWidget::OnClickVoar() { BeginSelectingType(EActionType::Voar); }
void UBattleActionSelectorWidget::OnClickSubmergir() { BeginSelectingType(EActionType::Submergir); }

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

FReply UBattleActionSelectorWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	FBattleDebugKeys::Observe(Key, TEXT("widget"));

	// As teclas de depuração NÃO são tratadas aqui: quem as trata é o ouvinte
	// de pré-input do Slate, que já as viu antes desta chamada. Tratar nos
	// dois lugares faria F7 alternar duas vezes por toque — "não funciona" de
	// novo, por motivo oposto.

	if (Key == EKeys::C)
	{
		OnClickCamuflar();
		return FReply::Handled();
	}
	if (Key == EKeys::V)
	{
		OnClickVoar();
		return FReply::Handled();
	}
	if (Key == EKeys::B)
	{
		OnClickSubmergir();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBattleActionSelectorWidget::SetChoosingForLabel(const FText& Label)
{
	if (ChoosingForLabel.EqualTo(Label))
	{
		return;
	}

	ChoosingForLabel = Label;
	RefreshLayoutFromState();
}
