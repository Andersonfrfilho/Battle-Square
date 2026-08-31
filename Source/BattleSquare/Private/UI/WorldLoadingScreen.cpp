// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/WorldLoadingScreen.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WorldLoading"

namespace
{
	TSharedPtr<SWidget> GLoadingWidget;
	FWorldLoadingProgress GProgress;

	/** Fundo opaco: meia-transparência mostraria o mundo pela metade, que é
	 *  exatamente o que esta tela existe para não mostrar. */
	const FLinearColor CorDoFundo(0.05f, 0.07f, 0.065f, 1.0f);
	const FLinearColor CorDoTexto(0.91f, 0.93f, 0.91f, 1.0f);
	const FLinearColor CorDoPasso(0.55f, 0.60f, 0.57f, 1.0f);
	const FLinearColor CorDaFalha(0.90f, 0.45f, 0.30f, 1.0f);
}

FText FWorldLoadingProgress::DescribeCurrentStep() const
{
	// O passo ATUAL, e não a lista inteira: quem está esperando quer saber se
	// a coisa anda, não auditar as quatro etapas.
	if (HasFailed())
	{
		return FText::Format(
			LOCTEXT("Falhou", "Não foi possível preparar o mundo: {Motivo}"),
			FFormatNamedArguments{ { TEXT("Motivo"), FText::FromString(PermanentProblem) } });
	}

	if (!bTypeCatalogReady)  { return LOCTEXT("PassoTipos", "Lendo os tipos de pet…"); }
	if (!bMirrorVerified)    { return LOCTEXT("PassoEspelho", "Conferindo a assinatura dos pets…"); }
	if (!bSceneryBuilt)      { return LOCTEXT("PassoCenario", "Plantando a mata…"); }
	if (!bEncountersPlaced)  { return LOCTEXT("PassoEncontros", "Soltando os adversários…"); }

	return LOCTEXT("PassoPronto", "Pronto.");
}

int32 FWorldLoadingProgress::PercentComplete() const
{
	int32 Prontos = 0;
	Prontos += bTypeCatalogReady ? 1 : 0;
	Prontos += bMirrorVerified ? 1 : 0;
	Prontos += bSceneryBuilt ? 1 : 0;
	Prontos += bEncountersPlaced ? 1 : 0;
	return (Prontos * 100) / 4;
}

void FWorldLoadingScreen::Show(UWorld* World)
{
	// Mesmo defeito do mapa: a global sobrevive ao PIE e o viewport não. Aqui
	// o sintoma seria pior — a tela de carregamento não apareceria no segundo
	// Play, e o jogador voltaria a existir num mundo ainda não montado.
	if (GLoadingWidget.IsValid())
	{
		Hide();
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	GProgress = FWorldLoadingProgress();

	GLoadingWidget =
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SImage).ColorAndOpacity(FSlateColor(CorDoFundo))
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Titulo", "BATTLE SQUARE"))
				.ColorAndOpacity(FSlateColor(CorDoTexto))
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			.Padding(0.0f, 18.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([]() { return GProgress.DescribeCurrentStep(); })
				.ColorAndOpacity_Lambda([]()
				{
					// A falha muda a COR, não só o texto: quem está esperando
					// não relê a linha, mas percebe que ela deixou de ser cinza.
					return FSlateColor(GProgress.HasFailed() ? CorDaFalha : CorDoPasso);
				})
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					return GProgress.HasFailed()
						? FText::GetEmpty()
						: FText::AsPercent(GProgress.PercentComplete() / 100.0f);
				})
				.ColorAndOpacity(FSlateColor(CorDoPasso))
			]
		];

	// ZOrder alto: precisa cobrir a barra de depuração e o painel, que também
	// são widgets de viewport. Uma tela de carregamento por baixo de qualquer
	// coisa não é uma tela de carregamento.
	GEngine->GameViewport->AddViewportWidgetContent(
		GLoadingWidget.ToSharedRef(), /*ZOrder=*/1000);
}

void FWorldLoadingScreen::Update(const FWorldLoadingProgress& Progress)
{
	GProgress = Progress;
}

void FWorldLoadingScreen::Hide()
{
	if (GEngine && GEngine->GameViewport && GLoadingWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GLoadingWidget.ToSharedRef());
	}
	GLoadingWidget.Reset();
}

bool FWorldLoadingScreen::IsVisible()
{
	return GLoadingWidget.IsValid();
}

#undef LOCTEXT_NAMESPACE
