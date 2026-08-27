// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BattleDebugHUD.generated.h"

/**
 * Desenha o painel de FBattleDebugScreen como uma CAIXA fixa na tela.
 *
 * HUD em vez de UMG de propósito: não depende de nenhum asset autorado, então
 * funciona em qualquer nível assim que o GameMode o declara — inclusive nos
 * níveis de teste que ainda não têm interface nenhuma.
 */
UCLASS()
class BATTLESQUARE_API ABattleDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Canto superior direito, para não brigar com os botões de ação. */
	UPROPERTY(EditDefaultsOnly, Category = "Depuração")
	float MarginUnits = 16.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Depuração")
	float PanelWidthUnits = 520.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Depuração")
	float LineHeightUnits = 18.0f;
};
