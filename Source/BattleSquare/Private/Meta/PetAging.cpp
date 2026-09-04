// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetAging.h"

int32 PetAging::RawAgeInDays(int32 GenesisWorldAgeDays, int32 CurrentWorldAgeDays)
{
	return FMath::Max(0, CurrentWorldAgeDays - GenesisWorldAgeDays);
}

int32 PetAging::EffectiveAgeInDays(
	int32 RawAgeDays, int32 CareActs, const FAgingConfig& Config)
{
	// Cuidado ATIVO (decisão 34): cada ato desconta dias do envelhecimento. É
	// a AÇÃO que conta, não o tempo — sem cuidar, o desconto é zero.
	const int32 DescontoDoCuidado =
		FMath::Max(0, FMath::FloorToInt(FMath::Max(0, CareActs) * Config.CareDaysPerAct));
	return FMath::Max(0, RawAgeDays - DescontoDoCuidado);
}

bool PetAging::IsDeceased(int32 EffectiveAgeDays, const FAgingConfig& Config)
{
	// Tempo de vida não-positivo nunca mata: config degenerada faz pet imortal,
	// não pet natimorto.
	if (Config.LifespanDays <= 0)
	{
		return false;
	}
	return EffectiveAgeDays >= Config.LifespanDays;
}
