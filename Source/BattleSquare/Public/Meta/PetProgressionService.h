// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"
#include "Battle/BattleState.h"

// T1–T3 (tasks.md, Níveis, Experiência e Evolução): XP, nível derivado,
// bônus de atributo — tudo fora de BattleSim (design.md). Nível NUNCA é
// um campo próprio: é sempre derivado de FOwnedPetInstance::Experience,
// para não ter duas fontes de verdade que podem divergir.
namespace BattlePetProgressionConstants
{
	constexpr int32 ExperienceForWin = 50;
	constexpr int32 ExperienceForLoss = 10; // nunca zero — jogar sempre rende algo
	constexpr int32 ExperienceForDraw = 25;

	constexpr int32 MaxLevel = 10;
	// XP cumulativo necessário para o nível N: (N - 1) * ExperiencePerLevel.
	// Linear de propósito — v1 não precisa de curva sofisticada.
	constexpr int32 ExperiencePerLevel = 100;

	// Bônus percentual de atributo POR NÍVEL acima de 1. Nível 1 = sem
	// bônus (NIVEL-09, zero regressão do catálogo puro).
	constexpr int32 AttributeBonusPercentPerLevel = 5;
}

class BATTLESQUARE_API FPetProgressionService
{
public:
	// XP cumulativo mínimo para estar EM (não "alcançar") o nível dado.
	// Nível 1 = 0.
	static int32 ExperienceRequiredForLevel(int32 Level);

	// Deriva o nível atual a partir de Instance.Experience — nunca acima
	// de BattlePetProgressionConstants::MaxLevel.
	static int32 GetLevel(const FOwnedPetInstance& Instance);

	// Credita XP à instância. Experience continua acumulando mesmo além
	// do necessário para o teto (NIVEL-07) — só GetLevel satura em
	// MaxLevel, o dado bruto nunca é truncado.
	static void GrantExperience(FOwnedPetInstance& Instance, int32 Amount);

	// Bônus de atributo por nível, aplicado a um FPetState já montado —
	// pura, sem side effect além dos 4 campos ajustados.
	static void ApplyLevelBonus(FPetState& State, int32 Level);
};
