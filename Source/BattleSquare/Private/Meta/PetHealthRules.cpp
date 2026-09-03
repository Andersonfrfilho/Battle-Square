// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetHealthRules.h"

int32 FPetHealthRules::PercentAfterBattle(int32 Health, int32 MaxHealth)
{
	if (MaxHealth <= 0)
	{
		return FullHealthPercent;
	}

	return FMath::Clamp(Health * 100 / MaxHealth, 0, FullHealthPercent);
}

int32 FPetHealthRules::StartingHealthFor(int32 HealthPercent, int32 MaxHealth)
{
	if (MaxHealth <= 0)
	{
		return MaxHealth;
	}

	// Negativo é o save antigo: não informado é CHEIO.
	if (HealthPercent < 0 || HealthPercent >= FullHealthPercent)
	{
		return MaxHealth;
	}

	return FMath::Max(1, MaxHealth * HealthPercent / 100);
}
