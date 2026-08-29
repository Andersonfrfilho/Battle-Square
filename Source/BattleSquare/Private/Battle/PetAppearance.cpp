// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetAppearance.h"

namespace
{
	/** Nomes de tipo como aparecem no catálogo de pets e em PetSkills.json. */
	const FString TipoFogo = TEXT("Fogo");
	const FString TipoAgua = TEXT("Agua");
	const FString TipoPlanta = TEXT("Planta");
}

FPetAppearance FPetAppearance::ForType(const FString& PetType)
{
	FPetAppearance Aparencia;

	if (PetType.Equals(TipoFogo, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.95f, 0.35f, 0.05f);
		Aparencia.CrestShape = EPetCrestShape::Chama;
		Aparencia.CrestScale = FVector(0.18f, 0.18f, 0.46f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -10.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoAgua, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.10f, 0.70f, 0.90f);
		Aparencia.CrestShape = EPetCrestShape::Barbatana;
		// Achatada no eixo de frente: barbatana é lâmina, não espinho.
		Aparencia.CrestScale = FVector(0.06f, 0.30f, 0.30f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -55.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoPlanta, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.20f, 0.78f, 0.25f);
		Aparencia.CrestShape = EPetCrestShape::Folha;
		Aparencia.CrestScale = FVector(0.32f, 0.07f, 0.38f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -34.0f);
		return Aparencia;
	}

	return Aparencia;
}
