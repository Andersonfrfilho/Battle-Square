// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * CANSAÇO DE QUEM CARREGA ALGUÉM EM CIMA (montaria-e-trilhas, MT2).
 *
 * Montar cansa, e cansa MAIS na subida que na descida — na MESMA proporção que
 * `UphillCostWeight`/`DownhillCostWeight` já usam para custo, nunca uma tabela
 * paralela. O cansaço é um valor NOVO e isolado: não é a barra de HP de batalha
 * e não zera ao entrar em combate (o contrapeso, garantido por ser um acumulador
 * próprio, alheio ao combate).
 *
 * A taxa-base de cansaço por metro é config (invariante 2); a regra é pura.
 */
namespace MountFatigue
{
	/**
	 * O cansaço ganho ao percorrer um trecho, dado o comprimento, o PESO de
	 * custo daquele trecho (subida pesa mais) e a taxa-base por unidade.
	 *
	 * Proporcional ao peso: subir a mesma distância cansa mais que descer, na
	 * proporção dos pesos. Nunca negativo.
	 */
	BATTLESQUARE_API float FatigueForStretch(
		float StretchLengthUnits, float SlopeCostWeight, float BaseRatePerUnit);

	/**
	 * O multiplicador de cansaço pelo PESO do pet montado (MT3).
	 *
	 * Mais pesado, mais cansa — mas SEMPRE finito: preso entre um piso e um TETO
	 * (MaxMultiplier), para que peso nunca torne um trajeto IMPOSSÍVEL, só mais
	 * cansativo. É o contrapeso mandatório da MT3. Peso de referência não-positivo
	 * devolve 1 (neutro), nunca divide por zero.
	 */
	BATTLESQUARE_API float WeightMultiplier(
		float PetWeight, float ReferenceWeight, float MaxMultiplier);

	/** O cansaço-base já pesado pelo peso do montado. */
	BATTLESQUARE_API float FatigueWithWeight(float BaseFatigue, float WeightMult);
}
