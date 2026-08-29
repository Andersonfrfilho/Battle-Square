// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugToolbar.h"

#include "Battle/BattleActionQueueComponent.h"
#include "Battle/BattleArena.h"
#include "Balance/PetSkillCatalog.h"
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

	// Tipo aguardando direção, para o pad de duas etapas do jogador 2.
	EActionType GPendingType = EActionType::Aguardar;
	bool GbAwaitingDirection = false;

	void AddPlayerTwo(EActionType Type, EBattleDirection Direction)
	{
		if (ABattleArena* Arena = FindArena())
		{
			FBattleAction Action;
			Action.Type = Type;
			Action.Direction = Direction;
			Arena->AddPlayerTwoAction(Action);
		}
	}

	void ChoosePlayerTwoType(EActionType Type)
	{
		FBattleDebugScreen::Show(
			FString::Printf(TEXT("clique (jogador 2): %s"),
				*UEnum::GetDisplayValueAsText(Type).ToString()), 10.0f, FColor::Orange, -1);

		if (BattleActionRequiresDirection(Type))
		{
			// Espera a direção, como a tela do jogador 1 faz.
			GPendingType = Type;
			GbAwaitingDirection = true;
			return;
		}

		GbAwaitingDirection = false;
		AddPlayerTwo(Type, EBattleDirection::Nenhuma);
	}

	void ChoosePlayerTwoDirection(EBattleDirection Direction)
	{
		if (!GbAwaitingDirection)
		{
			FBattleDebugScreen::Show(
				TEXT("jogador 2: escolha o tipo antes da direção"), 6.0f, FColor::Red, -1);
			return;
		}

		GbAwaitingDirection = false;
		AddPlayerTwo(GPendingType, Direction);
	}

	/** Skills do SEU pet: encaminha à fila do jogador 1, como um botão da tela. */
	void ChoosePlayerOneSkill(EActionType Tipo)
	{
		ABattleArena* Arena = FindArena();
		if (!Arena || !Arena->PlayerActionQueue)
		{
			return;
		}

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("clique (você): %s"),
				*UEnum::GetDisplayValueAsText(Tipo).ToString()), 10.0f, FColor::Cyan, -1);

		// DP-ui-01: encaminha, não decide. Se o pet não tiver a skill, quem
		// recusa é a fila — e o botão nem estará visível.
		Arena->PlayerActionQueue->BeginSelectingType(Tipo);
	}

	bool SkillIsAvailableToPlayerOne(EActionType Tipo)
	{
		const ABattleArena* Arena = FindArena();
		if (!Arena)
		{
			return false;
		}

		return Arena->GetAvailableActionsForSide(Arena->GetSideBeingChosen()).Contains(Tipo);
	}

	/**
	 * Nome do golpe no slot, ou o número quando o pet não tem nome para ele.
	 *
	 * Pet cadastrado antes dos golpes existirem chega sem nenhum — mostrar
	 * "golpe 1" é honesto; esconder o botão faria o jogador achar que perdeu a
	 * ação de atacar.
	 */
	FText PlayerMoveLabel(int32 Slot)
	{
		const ABattleArena* Arena = FindArena();
		if (!Arena)
		{
			return FText::AsNumber(Slot + 1);
		}

		const uint8 Lado = Arena->GetSideBeingChosen();
		const TArray<FString> Nomes = Arena->GetMoveNamesForSide(Lado);
		const FText Nome = Nomes.IsValidIndex(Slot)
			? FText::FromString(Nomes[Slot])
			: FText::Format(NSLOCTEXT("BattleToolbar", "GolpeSemNome", "golpe {Numero}"),
				FFormatNamedArguments{ { TEXT("Numero"), FText::AsNumber(Slot + 1) } });

		if (Arena->IsMoveUnlockedForSide(Lado, Slot))
		{
			return Nome;
		}

		// TRANCADO aparece, com o que falta. Sumir com o botão diria "este
		// golpe não existe"; dizer o requisito diz "este golpe eu ainda vou
		// ter" — que é a diferença entre uma ausência e um objetivo.
		return FText::Format(
			NSLOCTEXT("BattleToolbar", "GolpeTrancado", "🔒 {Nome} ({Requisito})"),
			FFormatNamedArguments{
				{ TEXT("Nome"), Nome },
				{ TEXT("Requisito"), Arena->GetMoveRequirementTextForSide(Lado, Slot) },
			});
	}

	/** Botão de golpe trancado fica visível e INERTE. */
	bool IsPlayerMoveEnabled(int32 Slot)
	{
		const ABattleArena* Arena = FindArena();
		return !Arena || Arena->IsMoveUnlockedForSide(Arena->GetSideBeingChosen(), Slot);
	}

	void ChoosePlayerMove(int32 Slot)
	{
		ABattleArena* Arena = FindArena();
		if (!Arena || !Arena->PlayerActionQueue)
		{
			return;
		}

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("clique: golpe %d"), Slot + 1), 10.0f, FColor::Cyan, -1);

		Arena->PlayerActionQueue->ConfirmMove(Slot);
	}

	FText PlayerOneSkillsHeader()
	{
		const ABattleArena* Arena = FindArena();
		if (!Arena)
		{
			return FText::GetEmpty();
		}

		const TArray<EActionType> Universais = FPetSkillCatalog::GetUniversalActions();
		TArray<FString> Nomes;
		for (EActionType Tipo : Arena->GetAvailableActionsForSide(Arena->GetSideBeingChosen()))
		{
			if (!Universais.Contains(Tipo))
			{
				Nomes.Add(UEnum::GetDisplayValueAsText(Tipo).ToString());
			}
		}

		// Dizer que NÃO tem skill é informação, não vazio: sem esta linha, um
		// pet sem skill e um catálogo quebrado ficariam idênticos na tela.
		return FText::FromString(Nomes.IsEmpty()
			? FString(TEXT("SUAS SKILLS — este pet não tem skill própria"))
			: FString::Printf(TEXT("SUAS SKILLS — %s"), *FString::Join(Nomes, TEXT(", "))));
	}

	bool SkillIsAvailableToPlayerTwo(EActionType Tipo)
	{
		const ABattleArena* Arena = FindArena();
		if (!Arena)
		{
			return true;
		}

		const TArray<EActionType> Disponiveis = Arena->GetAvailableActionsForSide(1);
		return Disponiveis.IsEmpty() || Disponiveis.Contains(Tipo);
	}

	FText PlayerTwoHeader()
	{
		const ABattleArena* Arena = FindArena();
		const int32 Escolhidas = Arena ? Arena->GetPlayerTwoActions().Num() : 0;

		if (GbAwaitingDirection)
		{
			return FText::FromString(FString::Printf(
				TEXT("JOGADOR 2 — %d/3  ·  escolha a direção"), Escolhidas));
		}

		return FText::FromString(Escolhidas == 0
			? FString(TEXT("JOGADOR 2 — sem ações (o bot decide por ele)"))
			: FString::Printf(TEXT("JOGADOR 2 — %d/3 escolhidas"), Escolhidas));
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

	// GOLPES — a escolha do turno desde DP-golpe-05.
	//
	// Aparecem sempre, e não só depois de clicar em Atacar: a tela precisa
	// dizer o que o pet SABE fazer antes de a pessoa decidir atacar, senão a
	// escolha do golpe vira surpresa depois do compromisso.
	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("── GOLPES (escolha o tipo antes) ──")))
		.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.3f)))
	];

	TSharedRef<SHorizontalBox> LinhaGolpes = SNew(SHorizontalBox);
	for (int32 Slot = 0; Slot < 4; ++Slot)
	{
		LinhaGolpes->AddSlot().AutoWidth().Padding(1.0f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(8.0f, 4.0f))
			.IsEnabled_Lambda([Slot]() { return IsPlayerMoveEnabled(Slot); })
			.OnClicked_Lambda([Slot]() { ChoosePlayerMove(Slot); return FReply::Handled(); })
			[
				SNew(STextBlock).Text_Lambda([Slot]() { return PlayerMoveLabel(Slot); })
			]
		];
	}
	Coluna->AddSlot().AutoHeight().Padding(2.0f, 0.0f, 2.0f, 8.0f)[ LinhaGolpes ];

	// SUAS SKILLS — só as que o SEU pet tem.
	//
	// Sem esta linha, o jogador não via a identidade do próprio pet em lugar
	// nenhum: as skills existiam na regra e só no teclado. Skill que não
	// aparece é skill que não é característica de ninguém.
	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(STextBlock)
		.Text_Lambda([]() { return PlayerOneSkillsHeader(); })
		.ColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.75f, 1.0f)))
	];

	const TPair<const TCHAR*, EActionType> Skills[] = {
		{ TEXT("Camuflar"), EActionType::Camuflar },
		{ TEXT("Voar"), EActionType::Voar },
		{ TEXT("Submergir"), EActionType::Submergir },
	};

	TSharedRef<SHorizontalBox> LinhaSkills = SNew(SHorizontalBox);
	for (const TPair<const TCHAR*, EActionType>& Skill : Skills)
	{
		const EActionType Valor = Skill.Value;
		LinhaSkills->AddSlot().AutoWidth().Padding(1.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([Valor]()
			{
				return SkillIsAvailableToPlayerOne(Valor)
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeButton(FText::FromString(Skill.Key), [Valor]() { ChoosePlayerOneSkill(Valor); })
			]
		];
	}
	Coluna->AddSlot().AutoHeight().Padding(2.0f, 0.0f, 2.0f, 8.0f)[ LinhaSkills ];

	// PAINEL DO JOGADOR 2 — sempre visível, sem modo para ligar e sem fase.
	//
	// Escolhe-se aqui as ações dele, e o turno fecha pelo botão normal de
	// confirmar do jogador 1. Sem ações aqui, o bot decide por ele, como
	// sempre. As versões anteriores exigiam ligar um modo e confirmar em duas
	// etapas — e cada etapa era uma chance de o caminho parecer destrutivo.
	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		SNew(STextBlock)
		.Text_Lambda([]() { return PlayerTwoHeader(); })
		.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.1f)))
	];

	const TPair<const TCHAR*, EActionType> Tipos[] = {
		{ TEXT("Aguardar"), EActionType::Aguardar },
		{ TEXT("Mover"), EActionType::Mover },
		{ TEXT("Atacar"), EActionType::Atacar },
		{ TEXT("Magia"), EActionType::Magia },
		{ TEXT("Defender"), EActionType::Defender },
		{ TEXT("Esquivar"), EActionType::Esquivar },
		{ TEXT("Camuflar"), EActionType::Camuflar },
		{ TEXT("Voar"), EActionType::Voar },
		{ TEXT("Submergir"), EActionType::Submergir },
	};

	TSharedRef<SHorizontalBox> LinhaTipos = SNew(SHorizontalBox);
	for (const TPair<const TCHAR*, EActionType>& Tipo : Tipos)
	{
		const EActionType Valor = Tipo.Value;
		LinhaTipos->AddSlot().AutoWidth().Padding(1.0f)
		[
			SNew(SBox)
			// Skill que o pet não tem some do painel. A recusa mora na fila
			// (DP-skill-02); esconder aqui evita o clique que só produziria
			// recusa — botão que nunca funciona é pior que botão ausente.
			.Visibility_Lambda([Valor]()
			{
				return SkillIsAvailableToPlayerTwo(Valor)
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeButton(FText::FromString(Tipo.Key), [Valor]() { ChoosePlayerTwoType(Valor); })
			]
		];
	}
	Coluna->AddSlot().AutoHeight()[ LinhaTipos ];

	// Direções na disposição da grade: cima em cima. Direção lida como lista
	// vira direção errada — esta tela já ensinou isso uma vez.
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
			MakeButton(FText::FromString(Dir.Key), [Valor]() { ChoosePlayerTwoDirection(Valor); })
		];
	}
	Coluna->AddSlot().AutoHeight()[ LinhaDirecoes ];

	Coluna->AddSlot().AutoHeight().Padding(2.0f)
	[
		MakeButton(FText::FromString(TEXT("Limpar ações do jogador 2")), []()
		{
			GbAwaitingDirection = false;
			if (ABattleArena* Arena = FindArena())
			{
				Arena->ClearPlayerTwoActions();
			}
		})
	];

	Coluna->AddSlot().AutoHeight().Padding(2.0f, 8.0f, 2.0f, 2.0f)
	[
		MakeButton(FText::FromString(TEXT("Copiar painel")),
			[]() { FBattleDebugScreen::CopyToClipboard(); })
	];

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
