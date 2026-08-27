// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugHUD.h"
#include "Battle/BattleNarration.h"
#include "Debug/BattleDebugScreen.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ABattleDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	// O feed vem ANTES e fora do gate: ele é produto. Esconder a depuração
	// com bs.ShowBattleDebug 0 não pode deixar o jogador sem saber o que
	// aconteceu na partida.
	DrawNarrationFeed();

	if (!FBattleDebugScreen::IsEnabled())
	{
		return;
	}

	const TArray<FBattleDebugScreen::FLine>& Lines = FBattleDebugScreen::GetLines();
	if (Lines.IsEmpty())
	{
		return;
	}

	// Altura FIXA: painel que cresce a cada linha muda de tamanho o tempo todo
	// e ainda assim não rola. Com teto de linhas e altura constante, as mais
	// antigas saem por cima e a caixa fica parada.
	const float PanelHeight = LineHeightUnits * (FBattleDebugScreen::MaxLines + 2) + MarginUnits;
	const float PanelLeft = Canvas->SizeX - PanelWidthUnits - MarginUnits;
	const float PanelTop = MarginUnits;

	// Fundo escuro translúcido: sem ele o texto some sobre o céu claro.
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f), PanelLeft, PanelTop, PanelWidthUnits, PanelHeight);

	float CursorY = PanelTop + MarginUnits * 0.5f;
	DrawText(TEXT("[F9 copia  |  F10 limpa  |  bs.ShowBattleDebug 0 esconde]"),
		FLinearColor(0.6f, 0.6f, 0.6f), PanelLeft + 8.0f, CursorY);
	CursorY += LineHeightUnits;

	for (const FBattleDebugScreen::FLine& Line : Lines)
	{
		DrawText(Line.Text, FLinearColor(Line.Color), PanelLeft + 8.0f, CursorY);
		CursorY += LineHeightUnits;
	}
}

void ABattleDebugHUD::DrawNarrationFeed()
{
	const TArray<FBattleNarrationFeed::FLine>& Lines = FBattleNarrationFeed::GetLines();
	if (Lines.IsEmpty() || !Canvas)
	{
		return;
	}

	const float FeedWidth = FMath::Min(FeedWidthUnits, Canvas->SizeX - MarginUnits * 2.0f);
	const float FeedHeight = FeedLineHeightUnits * FBattleNarrationFeed::MaxLines + MarginUnits;
	const float FeedLeft = (Canvas->SizeX - FeedWidth) * 0.5f;
	const float FeedTop = Canvas->SizeY - FeedHeight - MarginUnits * 2.0f;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), FeedLeft, FeedTop, FeedWidth, FeedHeight);

	// Desenhadas de baixo para cima: a frase mais RECENTE fica no rodapé, que
	// é onde o olho já está esperando a próxima.
	float CursorY = FeedTop + MarginUnits * 0.5f
		+ FeedLineHeightUnits * (FBattleNarrationFeed::MaxLines - Lines.Num());

	for (const FBattleNarrationFeed::FLine& Line : Lines)
	{
		DrawText(Line.Text.ToString(), FLinearColor(Line.Color), FeedLeft + 12.0f, CursorY, nullptr, FeedTextScale);
		CursorY += FeedLineHeightUnits;
	}
}
