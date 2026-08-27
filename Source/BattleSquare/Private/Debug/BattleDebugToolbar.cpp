// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugToolbar.h"

#include "Battle/BattleActionQueueComponent.h"
#include "Battle/BattleArena.h"
#include "Debug/BattleDebugScreen.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#if !UE_BUILD_SHIPPING

namespace
{
	TSharedPtr<SWidget> GToolbar;
	TWeakObjectPtr<UWorld> GToolbarWorld;

	ABattleArena* FindArena()
	{
		UWorld* World = GToolbarWorld.Get();
		if (!World)
		{
			return nullptr;
		}

		TActorIterator<ABattleArena> It(World);
		return It ? *It : nullptr;
	}

	/** Encaminha ao componente, como o botão da tela — DP-ui-01: nenhuma regra aqui. */
	void QueueAction(EActionType Type)
	{
		ABattleArena* Arena = FindArena();
		if (!Arena || !Arena->PlayerActionQueue)
		{
			FBattleDebugScreen::Show(TEXT("clique: sem arena para receber a ação"),
				8.0f, FColor::Red, -1);
			return;
		}

		FBattleDebugScreen::Show(FString::Printf(TEXT("clique (barra): %s"),
			*UEnum::GetDisplayValueAsText(Type).ToString()), 10.0f, FColor::White, -1);

		Arena->PlayerActionQueue->BeginSelectingType(Type);
	}

	FText ControlLabel()
	{
		const ABattleArena* Arena = FindArena();
		const bool bOn = Arena && Arena->IsControllingBothSides();
		// "jogador 1 e jogador 2", e não "os dois lados": na hora de escolher,
		// o que se precisa saber é POR QUEM se está jogando, e "lado" não
		// nomeia ninguém.
		return bOn
			? FText::FromString(TEXT("Controlando jogador 1 e jogador 2 (clique p/ desligar)"))
			: FText::FromString(TEXT("Controlar jogador 1 e jogador 2"));
	}

	TSharedRef<SWidget> MakeButton(const FText& Label, TFunction<void()> OnClick)
	{
		return SNew(SButton)
			.ContentPadding(FMargin(8.0f, 4.0f))
			.OnClicked_Lambda([OnClick]()
			{
				OnClick();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(Label)
			];
	}
}

void FBattleDebugToolbar::Show(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport || GToolbar.IsValid())
	{
		return;
	}

	GToolbarWorld = World;

	TSharedRef<SVerticalBox> Coluna = SNew(SVerticalBox);

	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		MakeButton(FText::FromString(TEXT("Copiar painel (e salvar arquivo)")),
			[]() { FBattleDebugScreen::CopyToClipboard(); })
	];

	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		MakeButton(FText::FromString(TEXT("Limpar painel")),
			[]() { FBattleDebugScreen::Clear(); })
	];

	// O rótulo é lido a cada quadro (Text_Lambda) para dizer o estado ATUAL.
	// Um botão que não mostra se ligou deixa a pessoa clicando duas vezes.
	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(SButton)
		.ContentPadding(FMargin(8.0f, 4.0f))
		.OnClicked_Lambda([]()
		{
			if (ABattleArena* Arena = FindArena())
			{
				Arena->SetControllingBothSides(!Arena->IsControllingBothSides());
			}
			return FReply::Handled();
		})
		[
			SNew(STextBlock).Text_Lambda([]() { return ControlLabel(); })
		]
	];

	// As três ações que o WBP ainda não tem. Ficam aqui até ganharem botão na
	// tela de verdade — sofrer uma regra sem poder usá-la é o pior estado
	// possível para uma mecânica nova.
	TSharedRef<SHorizontalBox> Acoes = SNew(SHorizontalBox);
	Acoes->AddSlot().AutoWidth().Padding(2.0f)
	[
		MakeButton(FText::FromString(TEXT("Camuflar")), []() { QueueAction(EActionType::Camuflar); })
	];
	Acoes->AddSlot().AutoWidth().Padding(2.0f)
	[
		MakeButton(FText::FromString(TEXT("Voar")), []() { QueueAction(EActionType::Voar); })
	];
	Acoes->AddSlot().AutoWidth().Padding(2.0f)
	[
		MakeButton(FText::FromString(TEXT("Submergir")), []() { QueueAction(EActionType::Submergir); })
	];
	Coluna->AddSlot().AutoHeight().Padding(2.0f)[ Acoes ];

	GToolbar = SNew(SBorder)
		.Padding(6.0f)
		.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f))
		[
			Coluna
		];

	// Canto inferior ESQUERDO: o painel de texto mora no superior direito, e o
	// feed de combate no rodapé central.
	GEngine->GameViewport->AddViewportWidgetContent(
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(16.0f, 0.0f, 0.0f, 0.0f))
		[
			GToolbar.ToSharedRef()
		],
		/*ZOrder=*/100);
}

void FBattleDebugToolbar::Hide()
{
	if (GEngine && GEngine->GameViewport && GToolbar.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GToolbar.ToSharedRef());
	}

	GToolbar.Reset();
	GToolbarWorld.Reset();
}

#else

void FBattleDebugToolbar::Show(UWorld*) {}
void FBattleDebugToolbar::Hide() {}

#endif // !UE_BUILD_SHIPPING
