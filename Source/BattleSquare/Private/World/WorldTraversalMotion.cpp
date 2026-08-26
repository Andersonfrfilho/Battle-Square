// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldTraversalMotion.h"

FVector FWorldTraversalMotion::ComputeMoveDirection(const FWorldTraversalMotionParams& Params)
{
	if (Params.MovementInput.IsNearlyZero())
	{
		// Normalizar zero é indefinido, e devolver lixo aqui viraria deriva
		// com o jogador sem tocar em nada (P1, critério 3).
		return FVector::ZeroVector;
	}

	// Só o yaw entra na conta: com o pitch, olhar para baixo empurraria o
	// personagem para dentro do chão.
	const FRotator YawOnlyRotation(0.0, Params.CameraRotation.Yaw, 0.0);
	const FVector ForwardDirection = FRotationMatrix(YawOnlyRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawOnlyRotation).GetUnitAxis(EAxis::Y);

	const FVector CombinedDirection =
		ForwardDirection * Params.MovementInput.X + RightDirection * Params.MovementInput.Y;

	// Normalizar é o que impede a diagonal de ser mais rápida que andar reto.
	return CombinedDirection.GetSafeNormal();
}
