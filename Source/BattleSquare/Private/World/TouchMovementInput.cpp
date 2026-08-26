// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TouchMovementInput.h"

FVector2D FTouchMovementInput::ComputeMovementAxis(const FTouchMovementParams& Params)
{
	const FVector2D ScreenDelta = Params.TouchCurrentScreen - Params.TouchOriginScreen;
	const double DragLength = ScreenDelta.Size();

	if (DragLength <= TouchMovement::TouchDeadZoneScreenUnits)
	{
		return FVector2D::ZeroVector;
	}

	// Em coordenadas de tela o Y cresce para BAIXO. Arrastar para cima é
	// andar para frente, então o sinal do Y inverte ao virar eixo de jogo.
	const FVector2D GameAxis(-ScreenDelta.Y, ScreenDelta.X);

	// Além do raio máximo satura em 1: mais longe não pode ser mais rápido.
	const double ClampedLength = FMath::Min(DragLength, static_cast<double>(TouchMovement::TouchMaxRadiusScreenUnits));
	const double Magnitude = ClampedLength / TouchMovement::TouchMaxRadiusScreenUnits;

	return GameAxis.GetSafeNormal() * Magnitude;
}
