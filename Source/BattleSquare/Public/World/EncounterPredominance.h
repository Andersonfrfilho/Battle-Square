// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/IslandGeography.h"

/**
 * A FAUNA DO LUGAR (decisão 62): predominância, NUNCA exclusividade.
 *
 * "Nosso primeiro bioma é natureza comum: mais tipos comuns, natureza,
 * plantas... alguns aquáticos, cavernas, noturnos desses ambientes." O
 * noturno JÁ existia (peso por hora, `WorldTimeOfDay`); isto é a outra
 * metade — o peso por LUGAR — e os dois se MULTIPLICAM em vez de um
 * substituir o outro: o pet aquático-noturno aparece de noite E perto
 * d'água, que é exatamente o que a frase pede.
 *
 * PURO: recebe tipo e arredores, devolve peso. Quem sabe onde há água e
 * caverna é quem chama — o traçado assado já responde as duas perguntas.
 */
namespace EncounterPredominance
{
	/** O que há em volta do ponto onde o encontro vai nascer. */
	struct BATTLESQUARE_API FSpawnSurroundings
	{
		bool bNearWater = false;
		bool bNearCave = false;
		EIslandBiome Biome = EIslandBiome::Forest;
	};

	/**
	 * O peso do LUGAR para este tipo, em porcento (100 = neutro).
	 *
	 * NUNCA zero — predominância não é exclusividade, e são as palavras da
	 * própria decisão: o pet de fogo na mata é raro, não impossível. Zerar
	 * seria trancar, e trancar em silêncio é o defeito de sempre.
	 *
	 * Tipo desconhecido pesa 100: ausência não decide nada.
	 */
	BATTLESQUARE_API int32 PlaceWeightPercent(const FString& PetType,
		const FSpawnSurroundings& Surroundings);
}
