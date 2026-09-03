// Copyright 2026 Anderson. All Rights Reserved.

#include "World/GroundWorkRules.h"

bool GroundWorkRules::IsWorkPlace(EGroundUse Uso)
{
	return Uso == EGroundUse::Fazenda
		|| Uso == EGroundUse::Criadouro
		|| Uso == EGroundUse::Pomar;
}

EActionType GroundWorkRules::FacilitatingSkillFor(EGroundUse Uso)
{
	switch (Uso)
	{
	case EGroundUse::Fazenda:   return EActionType::Escavar;
	case EGroundUse::Pomar:     return EActionType::Voar;
	case EGroundUse::Criadouro: return EActionType::Camuflar;
	default: break;
	}

	return EActionType::Aguardar;
}

int32 GroundWorkRules::PayFor(int32 BasePay, int32 PetBonusPercent, bool bPetHasSkill)
{
	const int32 Base = FMath::Max(0, BasePay);
	if (!bPetHasSkill)
	{
		return Base;
	}

	return Base * (100 + FMath::Max(0, PetBonusPercent)) / 100;
}
