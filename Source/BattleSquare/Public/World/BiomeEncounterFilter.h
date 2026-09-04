// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/IslandGeography.h"

struct FLoadedPetRecord;

/**
 * O PET SELVAGEM MUDA COM O BIOMA (mundo-por-biomas, MB2).
 *
 * Cada bioma tem seus elementos, pela tabela decidida na spec ("As quatro ilhas,
 * e por que essas"). O pet de Vulcão não aparece na Geleira: o encontro filtra o
 * catálogo pelo bioma do LUGAR onde ele acontece.
 *
 * Fonte ÚNICA da tabela bioma→elemento — quem precisar dela chama aqui, nunca
 * redeclara (L-032/L-033). Os quatro biomas da spec são exatos; Deserto e Praia,
 * que a tabela não nomeia, herdam o arquétipo mais próximo (Deserto = o alto,
 * aberto e claro de Planalto/Penhascos; Praia = a borda molhada e ventada).
 */
namespace BiomeEncounterFilter
{
	/** Os elementos que aparecem num bioma (fonte única da tabela da spec). */
	BATTLESQUARE_API const TArray<FString>& ElementsOf(EIslandBiome Biome);

	/** Este tipo de pet (`Escola/Elemento`) cabe no bioma? */
	BATTLESQUARE_API bool FitsBiome(const FString& PetType, EIslandBiome Biome);

	/**
	 * O catálogo filtrado pelo bioma do encontro.
	 *
	 * `bHasBiome` falso (encontro SEM mundo — EncounterLocation vazio, batalha
	 * aberta direto) NÃO filtra nada: sem lugar, não há bioma a consultar, e o
	 * catálogo de sempre passa inteiro. É o contrapeso da MB2.
	 */
	BATTLESQUARE_API TArray<FLoadedPetRecord> FilterByBiome(
		const TArray<FLoadedPetRecord>& Catalog, EIslandBiome Biome, bool bHasBiome);
}
