// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/TreeGrowth.h"

float TreeGrowth::ScaleFactorFor(int32 WorldAgeInDays, const FGrowthConfig& Config)
{
	// Config degenerada (maturidade em zero dia ou negativa) não divide por
	// zero: a mata já nasce adulta. É também o caminho da idade desconhecida,
	// que o chamador traduz para uma maturidade não-positiva de propósito.
	if (Config.DaysToMaturity <= 0 || WorldAgeInDays < 0)
	{
		return 1.0f;
	}

	// Fração do caminho até a maturidade, presa em [0,1] — satura no adulto.
	const float Progresso = FMath::Clamp(
		static_cast<float>(WorldAgeInDays) / static_cast<float>(Config.DaysToMaturity),
		0.0f, 1.0f);

	// Muda -> adulto, linear. Piso na muda, teto em 1.0.
	const float Muda = FMath::Clamp(Config.SaplingScale, 0.0f, 1.0f);
	return FMath::Lerp(Muda, 1.0f, Progresso);
}
