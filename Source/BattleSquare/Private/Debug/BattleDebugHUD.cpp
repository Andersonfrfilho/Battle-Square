// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugHUD.h"
#include "Debug/BattleDebugScreen.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ABattleDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!FBattleDebugScreen::IsEnabled() || !Canvas)
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
