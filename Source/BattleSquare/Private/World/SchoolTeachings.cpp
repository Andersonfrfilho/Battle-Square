// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SchoolTeachings.h"

#include "Balance/TypeEffectivenessTable.h"
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

namespace
{
	/** "Fisica/Terra" -> "Terra": o jogador lê o elemento, não o caminho. */
	FString NomeDoElementoNaLicao(const FString& Tipo)
	{
		FString Escola;
		FString Elemento;
		return Tipo.Split(TEXT("/"), &Escola, &Elemento) ? Elemento : Tipo;
	}
}

TArray<FString> SchoolTeachings::BuildStrategyLines(const FString& PetType,
	const FTypeEffectivenessTable& Table, const TArray<FString>& AllTypes)
{
	TArray<FString> BomContra;
	TArray<FString> RuimContra;
	TArray<FString> CuidadoCom;

	for (const FString& Outro : AllTypes)
	{
		if (Outro == PetType)
		{
			continue;
		}

		const FString Elemento = NomeDoElementoNaLicao(Outro);

		// ATACANDO: acima de 100 é vantagem, abaixo é desvantagem — o neutro
		// não entra na lição, porque lição sobre o neutro é ruído.
		const int32 Atacando = Table.GetPercent(PetType, Outro);
		if (Atacando > 100)
		{
			BomContra.AddUnique(Elemento);
		}
		else if (Atacando < 100)
		{
			RuimContra.AddUnique(Elemento);
		}

		// APANHANDO: só o golpe FORTE (150+) vira aviso. Listar toda
		// desvantagem leve deixaria o "cuidado" sem peso — aviso de tudo é
		// aviso de nada.
		if (Table.GetPercent(Outro, PetType) >= 150)
		{
			CuidadoCom.AddUnique(Elemento);
		}
	}

	TArray<FString> Linhas;
	if (BomContra.Num() > 0)
	{
		Linhas.Add(FString::Printf(TEXT("  bom contra: %s"),
			*FString::Join(BomContra, TEXT(", "))));
	}
	if (RuimContra.Num() > 0)
	{
		Linhas.Add(FString::Printf(TEXT("  ruim contra: %s"),
			*FString::Join(RuimContra, TEXT(", "))));
	}
	if (CuidadoCom.Num() > 0)
	{
		Linhas.Add(FString::Printf(TEXT("  ⚠ apanha feio de: %s"),
			*FString::Join(CuidadoCom, TEXT(", "))));
	}

	return Linhas;
}

TArray<FString> SchoolTeachings::BuildTerrainLines()
{
	// Cada frase tem o teste que a prova, e é por isso que são ESTAS:
	// - dano de casa, voar escapa, camuflar não —
	//   `BattleSim.Concealment.LeavingTheGround.AvoidsCellDamage`;
	// - submergir exige água FUNDA — a poça rasa nasceu depois, e `Water`
	//   continua sendo a única que serve (comentário de `ShallowWater`);
	// - gelo nega com certeza, lama é aposta — é a diferença que faz congelar
	//   valer um slot (comentário de `Mud`).
	// Frase sem teste correspondente não entra: a escola só ensina o que a
	// batalha cobra.
	return {
		TEXT("  terreno: casa de DANO fere quem pisa — voar escapa, camuflar NAO"),
		TEXT("  terreno: submergir exige agua FUNDA; gelo nega, lama e aposta"),
	};
}
