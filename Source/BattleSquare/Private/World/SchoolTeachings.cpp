// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SchoolTeachings.h"

#include "Meta/PetMoveRequirements.h"

TArray<FString> SchoolTeachings::BuildLessonLines(
	const FLoadedPetRecord& Record, const FOwnedPetInstance& Owned)
{
	TArray<FString> Linhas;

	for (const FLoadedPetMove& Golpe : Record.Moves)
	{
		if (FPetMoveRequirements::IsMet(Golpe.RequiresAttribute, Golpe.RequiresValue, Owned))
		{
			Linhas.Add(FString::Printf(TEXT("  ✓ %s"), *Golpe.Name));
			continue;
		}

		// A LIÇÃO INTEIRA numa linha: o que falta, quanto falta, e onde se
		// treina. `DescribeRequirement` é a MESMA função da tela de batalha —
		// duas frases sobre o mesmo requisito divergiriam na primeira edição.
		Linhas.Add(FString::Printf(TEXT("  🔒 %s — %s (treine no pátio: campo de %s)"),
			*Golpe.Name,
			*FPetMoveRequirements::DescribeRequirement(
				Golpe.RequiresAttribute, Golpe.RequiresValue).ToString(),
			*FPetMoveRequirements::GetAttributeLabel(Golpe.RequiresAttribute).ToString()));
	}

	return Linhas;
}
