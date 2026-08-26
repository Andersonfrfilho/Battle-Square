// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FWorldTraversalMotionParams
{
	/** X = frente/trás, Y = direita/esquerda, em [-1, 1]. */
	FVector2D MovementInput = FVector2D::ZeroVector;

	FRotator CameraRotation = FRotator::ZeroRotator;
};

/**
 * A única parte do traversal que é decisão nossa: para ONDE mover. Quem move é
 * o UCharacterMovementComponent (DP-trav-02). Pura — sem UWorld, sem ator, sem tempo.
 */
class BATTLESQUARE_API FWorldTraversalMotion
{
public:
	/** Direção no espaço do mundo, normalizada. Vetor nulo para entrada nula. */
	static FVector ComputeMoveDirection(const FWorldTraversalMotionParams& Params);
};
