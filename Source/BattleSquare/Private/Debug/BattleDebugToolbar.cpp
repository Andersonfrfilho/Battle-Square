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

	int32 ChoosingPlayerNumber()
	{
		const ABattleArena* Arena = FindArena();
		return Arena ? static_cast<int32>(Arena->GetSideBeingChosen()) + 1 : 1;
	}

	EVisibility PlayerTwoPadVisibility()
	{
		// Aparece durante TODO o modo duplo, não só na vez do jogador 2: o
		// painel é o caminho para chegar ao jogador 2, e escondê-lo até a vez
		// dele fazia o caminho depender de um botão que parecia encerrar tudo.
		const ABattleArena* Arena = FindArena();
		return (Arena && Arena->IsControllingBothSides())
			? EVisibility::Visible : EVisibility::Collapsed;
	}

	/**
	 * O rótulo do confirmar diz a CONSEQUÊNCIA, não a mecânica.
	 *
	 * "Confirmar turno" na vez do jogador 1 fazia parecer que a batalha ia
	 * resolver — então ninguém clicava, e sem clicar não dava para chegar ao
	 * jogador 2. O botão estava certo; o texto é que mentia.
	 */
	FText CommitLabel()
	{
		const ABattleArena* Arena = FindArena();
		if (!Arena || !Arena->IsControllingBothSides())
		{
			return FText::FromString(TEXT("Confirmar turno"));
		}

		return (ChoosingPlayerNumber() == 1)
			? FText::FromString(TEXT("Confirmar jogador 1  →  passar ao jogador 2"))
			: FText::FromString(TEXT("Confirmar jogador 2  →  RESOLVER o turno"));
	}

	FText PadHeaderLabel()
	{
		return FText::FromString(FString::Printf(
			TEXT("── AÇÕES DO JOGADOR %d ──"), ChoosingPlayerNumber()));
	}

	void ConfirmDirection(EBattleDirection Direction)
	{
		ABattleArena* Arena = FindArena();
		if (!Arena || !Arena->PlayerActionQueue)
		{
			return;
		}

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("clique (jogador %d): direção"), ChoosingPlayerNumber()),
			10.0f, FColor::White, -1);
		Arena->PlayerActionQueue->ConfirmDirection(Direction);
	}

	void CommitTurn()
	{
		ABattleArena* Arena = FindArena();
		if (!Arena || !Arena->PlayerActionQueue)
		{
			return;
		}

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("clique (jogador %d): confirmar"), ChoosingPlayerNumber()),
			10.0f, FColor::White, -1);
		Arena->PlayerActionQueue->Commit();
	}

	FText BothSidesLabel()
	{
		const ABattleArena* Arena = FindArena();
		const bool bOn = Arena && Arena->IsControllingBothSides();
		return bOn
			? FText::FromString(TEXT("Controlando jogador 1 E jogador 2 (clique p/ desligar)"))
			: FText::FromString(TEXT("Controlar também o jogador 2"));
	}

	FText ControlLabel()
	{
		const ABattleArena* Arena = FindArena();
		const int32 Atual = Arena ? static_cast<int32>(Arena->GetControlledPlayerNumber()) : 1;
		const int32 Proximo = (Atual == 1) ? 2 : 1;

		// Diz o estado ATUAL e o que o clique faz. Só o estado deixaria a
		// pessoa adivinhando o efeito; só o efeito, adivinhando onde está.
		return FText::FromString(FString::Printf(
			TEXT("Controlando jogador %d  —  clique para o jogador %d"), Atual, Proximo));
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

	// Os rótulos são lidos a cada quadro (Text_Lambda) para dizer o estado
	// ATUAL. Botão que não mostra se ligou deixa a pessoa clicando duas vezes.
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
			SNew(STextBlock).Text_Lambda([]() { return BothSidesLabel(); })
		]
	];

	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(SButton)
		.ContentPadding(FMargin(8.0f, 4.0f))
		.OnClicked_Lambda([]()
		{
			if (ABattleArena* Arena = FindArena())
			{
				Arena->SwapControlledPlayer();
			}
			return FReply::Handled();
		})
		[
			SNew(STextBlock).Text_Lambda([]() { return ControlLabel(); })
		]
	];

	// PAINEL DO JOGADOR 2 — aparece só quando é a vez dele.
	TSharedRef<SVerticalBox> PadJogadorDois = SNew(SVerticalBox);

	PadJogadorDois->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(STextBlock)
		.Text_Lambda([]() { return PadHeaderLabel(); })
		.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.1f)))
	];

	const TPair<const TCHAR*, EActionType> Tipos[] = {
		{ TEXT("Aguardar"), EActionType::Aguardar },
		{ TEXT("Mover"), EActionType::Mover },
		{ TEXT("Atacar"), EActionType::Atacar },
		{ TEXT("Magia"), EActionType::Magia },
		{ TEXT("Defender"), EActionType::Defender },
		{ TEXT("Esquivar"), EActionType::Esquivar },
	};

	TSharedRef<SHorizontalBox> LinhaTipos = SNew(SHorizontalBox);
	for (const TPair<const TCHAR*, EActionType>& Tipo : Tipos)
	{
		const EActionType Valor = Tipo.Value;
		LinhaTipos->AddSlot().AutoWidth().Padding(1.0f)
		[
			MakeButton(FText::FromString(Tipo.Key), [Valor]() { QueueAction(Valor); })
		];
	}
	PadJogadorDois->AddSlot().AutoHeight()[ LinhaTipos ];

	// Direções: mesma disposição da grade, para o botão de cima ser em cima.
	const TPair<const TCHAR*, EBattleDirection> Direcoes[] = {
		{ TEXT("↖"), EBattleDirection::CimaEsquerda },
		{ TEXT("↑"), EBattleDirection::Cima },
		{ TEXT("↗"), EBattleDirection::CimaDireita },
		{ TEXT("←"), EBattleDirection::Esquerda },
		{ TEXT("→"), EBattleDirection::Direita },
		{ TEXT("↙"), EBattleDirection::BaixoEsquerda },
		{ TEXT("↓"), EBattleDirection::Baixo },
		{ TEXT("↘"), EBattleDirection::BaixoDireita },
	};

	TSharedRef<SHorizontalBox> LinhaDirecoes = SNew(SHorizontalBox);
	for (const TPair<const TCHAR*, EBattleDirection>& Dir : Direcoes)
	{
		const EBattleDirection Valor = Dir.Value;
		LinhaDirecoes->AddSlot().AutoWidth().Padding(1.0f)
		[
			MakeButton(FText::FromString(Dir.Key), [Valor]() { ConfirmDirection(Valor); })
		];
	}
	PadJogadorDois->AddSlot().AutoHeight()[ LinhaDirecoes ];

	PadJogadorDois->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(SButton)
		.ContentPadding(FMargin(8.0f, 4.0f))
		.OnClicked_Lambda([]() { CommitTurn(); return FReply::Handled(); })
		[
			SNew(STextBlock).Text_Lambda([]() { return CommitLabel(); })
		]
	];

	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(SBox)
		.Visibility_Lambda([]() { return PlayerTwoPadVisibility(); })
		[
			PadJogadorDois
		]
	];

	// As três ações que o WBP ainda não tem, para QUEM estiver escolhendo no
	// momento. Ficam aqui até ganharem botão na tela de verdade — sofrer uma
	// regra sem poder usá-la é o pior estado possível para uma mecânica nova.
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
