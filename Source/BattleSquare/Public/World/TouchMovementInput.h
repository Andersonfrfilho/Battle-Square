// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace TouchMovement
{
	/** Abaixo disto o dedo está parado — sem isto, tremor de dedo vira deriva. */
	inline constexpr float TouchDeadZoneScreenUnits = 12.0f;

	/** Além disto satura: mais longe não é mais rápido. */
	inline constexpr float TouchMaxRadiusScreenUnits = 120.0f;
}

struct FTouchMovementParams
{
	/** Onde o dedo encostou, em coordenadas de tela. */
	FVector2D TouchOriginScreen = FVector2D::ZeroVector;

	/** Onde o dedo está agora, em coordenadas de tela. */
	FVector2D TouchCurrentScreen = FVector2D::ZeroVector;
};

/**
 * Converte o gesto num eixo 2D no MESMO formato que o teclado produz
 * (DP-mobile-02). O que se faz com o eixo continua sendo
 * FWorldTraversalMotion::ComputeMoveDirection — toque não é um segundo
 * caminho de movimento, é outra forma de colher o mesmo eixo.
 */
class BATTLESQUARE_API FTouchMovementInput
{
public:
	/** X = frente/trás, Y = direita/esquerda, comprimento em [0, 1]. */
	static FVector2D ComputeMovementAxis(const FTouchMovementParams& Params);
};
