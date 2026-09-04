// Copyright 2026 Anderson. All Rights Reserved.

#include "World/MountFatigue.h"

float MountFatigue::FatigueForStretch(
	float StretchLengthUnits, float SlopeCostWeight, float BaseRatePerUnit)
{
	const float Comprimento = FMath::Max(0.0f, StretchLengthUnits);
	const float Peso = FMath::Max(0.0f, SlopeCostWeight);
	const float Taxa = FMath::Max(0.0f, BaseRatePerUnit);
	// Proporcional ao PESO de custo: a mesma subida cansa mais que a descida,
	// na proporcao que a trilha ja usa — nao uma segunda tabela.
	return Comprimento * Peso * Taxa;
}
