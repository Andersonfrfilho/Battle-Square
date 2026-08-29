// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TrainerSpecialtyRules.h"

#include "Meta/PetMoveRequirements.h"

bool FTrainerSpecialtyRules::IsSpecialistIn(const FTrainerProfile& Profile, const FString& Attribute)
{
	return Profile.Specialties.Contains(Attribute);
}

int32 FTrainerSpecialtyRules::FreeSlots(const FTrainerProfile& Profile)
{
	return FMath::Max(0, MaxSpecialties - Profile.Specialties.Num());
}

bool FTrainerSpecialtyRules::TryLearn(FTrainerProfile& Profile, const FString& Attribute)
{
	// Atributo que o jogo não conhece NÃO gasta vaga. Gastar uma das duas num
	// nome escrito errado seria punição permanente por erro de cadastro — e a
	// vaga não volta.
	if (FPetMoveRequirements::GetAttributeLabel(Attribute).ToString() == Attribute)
	{
		return false;
	}

	if (IsSpecialistIn(Profile, Attribute) || FreeSlots(Profile) <= 0)
	{
		return false;
	}

	Profile.Specialties.Add(Attribute);
	return true;
}
