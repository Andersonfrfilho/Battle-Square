// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetProgressionService.h"

int32 FPetProgressionService::ExperienceRequiredForLevel(int32 Level)
{
	const int32 ClampedLevel = FMath::Clamp(Level, 1, BattlePetProgressionConstants::MaxLevel);
	return (ClampedLevel - 1) * BattlePetProgressionConstants::ExperiencePerLevel;
}

int32 FPetProgressionService::GetLevel(const FOwnedPetInstance& Instance)
{
	// Começa em 1 (piso) e sobe enquanto o XP acumulado alcançar o
	// limiar do próximo nível — nunca passa de MaxLevel, mesmo com XP
	// muito além do necessário (NIVEL-07).
	int32 Level = 1;
	while (Level < BattlePetProgressionConstants::MaxLevel
		&& Instance.Experience >= ExperienceRequiredForLevel(Level + 1))
	{
		++Level;
	}
	return Level;
}

void FPetProgressionService::GrantExperience(FOwnedPetInstance& Instance, int32 Amount)
{
	// Experience continua acumulando mesmo além do teto — só GetLevel
	// satura; o dado bruto nunca é truncado (NIVEL-07).
	Instance.Experience += Amount;
}

void FPetProgressionService::ApplyLevelBonus(FPetState& State, int32 Level)
{
	const int32 ClampedLevel = FMath::Clamp(Level, 1, BattlePetProgressionConstants::MaxLevel);
	if (ClampedLevel <= 1)
	{
		return; // nível 1: zero regressão, nenhum atributo tocado.
	}

	const int32 BonusPercent = 100 + (ClampedLevel - 1) * BattlePetProgressionConstants::AttributeBonusPercentPerLevel;

	State.Attack = (State.Attack * BonusPercent) / 100;
	State.Defense = (State.Defense * BonusPercent) / 100;
	State.Speed = (State.Speed * BonusPercent) / 100;
	State.MaxHealth = (State.MaxHealth * BonusPercent) / 100;
	// Health começa igual a MaxHealth (mesma regra de TranslatePet) —
	// se o bônus mudou MaxHealth, Health precisa acompanhar aqui, senão
	// o pet entraria com Health "velho" menor que o novo teto.
	State.Health = State.MaxHealth;
}
