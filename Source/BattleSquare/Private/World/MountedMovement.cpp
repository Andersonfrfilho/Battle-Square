// Copyright 2026 Anderson. All Rights Reserved.

#include "World/MountedMovement.h"

float MountedMovement::SpeedOnStretch(float BaseSpeed, float SlopeCostWeight)
{
	if (SlopeCostWeight <= 0.0f)
	{
		return BaseSpeed;
	}
	// Mais custo, mais devagar — a MESMA conta que a trilha usa, so lida como
	// velocidade em vez de custo. A proporcao subida/descida e a dos pesos.
	return BaseSpeed / SlopeCostWeight;
}

float MountedMovement::MountedBaseSpeed(float OnFootSpeed, float MountMultiplier)
{
	// Montar nunca atrapalha: piso em 1. O normal e > 1 (mais rapido montado).
	return OnFootSpeed * FMath::Max(1.0f, MountMultiplier);
}
