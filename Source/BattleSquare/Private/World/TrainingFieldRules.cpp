// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TrainingFieldRules.h"

#include "Meta/PetAttributeProgression.h"

int32 FTrainingFieldRules::PointsForTime(float SecondsInField, bool bOwnerIsSpecialist, float& InOutCarrySeconds)
{
	if (SecondsInField <= 0.0f)
	{
		return 0;
	}

	// O bônus do especialista rende MAIS TEMPO ÚTIL, não mais pontos por
	// ponto: assim ele se compõe com o resto acumulado em vez de arredondar
	// duas vezes, e cinco segundos de especialista valem exatamente o que
	// deveriam valer em vez de virarem zero.
	const float SegundosEfetivos = bOwnerIsSpecialist
		? SecondsInField * (static_cast<float>(SpecialistBonusPercent) / 100.0f)
		: SecondsInField;

	const float Acumulado = InOutCarrySeconds + SegundosEfetivos;
	const int32 Pontos = FMath::FloorToInt(Acumulado / SecondsPerPoint);

	InOutCarrySeconds = Acumulado - static_cast<float>(Pontos) * SecondsPerPoint;
	return Pontos;
}

void FTrainingFieldRules::ApplyPoints(const FString& Attribute, int32 Points, FOwnedPetInstance& Instance)
{
	if (Points <= 0)
	{
		return;
	}

	// Os mesmos nomes do requisito de golpe, e não uma segunda lista: um campo
	// que treinasse "musculatura" enquanto o golpe exige "musculature" seria
	// um treino que nunca destrava nada, e nada apontaria a causa.
	if (Attribute == TEXT("musculature"))
	{
		Instance.Musculature += Points;
	}
	else if (Attribute == TEXT("personality"))
	{
		Instance.Personality += Points;
	}
	else if (Attribute == TEXT("camouflage"))
	{
		Instance.SkillProficiency[FPetAttributeProgression::Camouflage] += Points;
	}
	else if (Attribute == TEXT("flight"))
	{
		Instance.SkillProficiency[FPetAttributeProgression::Flight] += Points;
	}
	else if (Attribute == TEXT("underground"))
	{
		Instance.SkillProficiency[FPetAttributeProgression::Underground] += Points;
	}
}
