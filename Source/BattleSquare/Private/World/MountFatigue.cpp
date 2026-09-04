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


float MountFatigue::WeightMultiplier(
	float PetWeight, float ReferenceWeight, float MaxMultiplier)
{
	if (ReferenceWeight <= 0.0f)
	{
		return 1.0f;
	}
	const float Bruto = FMath::Max(0.0f, PetWeight) / ReferenceWeight;
	// Piso baixo (pet leve cansa menos, mas ainda cansa) e TETO finito: peso
	// nunca torna o trajeto impossivel, so mais cansativo — o contrapeso da MT3.
	const float Teto = FMath::Max(1.0f, MaxMultiplier);
	return FMath::Clamp(Bruto, 0.25f, Teto);
}

float MountFatigue::FatigueWithWeight(float BaseFatigue, float WeightMult)
{
	return FMath::Max(0.0f, BaseFatigue) * FMath::Max(0.0f, WeightMult);
}
