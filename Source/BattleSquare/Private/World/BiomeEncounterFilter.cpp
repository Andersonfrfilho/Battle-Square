// Copyright 2026 Anderson. All Rights Reserved.

#include "World/BiomeEncounterFilter.h"

#include "Balance/PetTypeIdentity.h"
#include "Data/PetDataLoader.h"

const TArray<FString>& BiomeEncounterFilter::ElementsOf(EIslandBiome Biome)
{
	// A tabela da spec, fonte única. Os quatro biomas nomeados são exatos;
	// Deserto e Praia herdam o arquétipo mais próximo.
	static const TArray<FString> Floresta = { TEXT("Planta"), TEXT("Fantasma") };
	static const TArray<FString> Pantano  = { TEXT("Agua"),   TEXT("Planta")   };
	static const TArray<FString> Geleira  = { TEXT("Luz"),    TEXT("Agua")     };
	static const TArray<FString> Vulcao   = { TEXT("Fogo"),   TEXT("Terra")    };
	static const TArray<FString> Deserto  = { TEXT("Ar"),     TEXT("Luz")      };
	static const TArray<FString> Praia    = { TEXT("Agua"),   TEXT("Ar")       };

	switch (Biome)
	{
	case EIslandBiome::Forest:  return Floresta;
	case EIslandBiome::Swamp:   return Pantano;
	case EIslandBiome::Glacier: return Geleira;
	case EIslandBiome::Volcano: return Vulcao;
	case EIslandBiome::Desert:  return Deserto;
	case EIslandBiome::Beach:   return Praia;
	}
	return Floresta;
}

bool BiomeEncounterFilter::FitsBiome(const FString& PetType, EIslandBiome Biome)
{
	const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(PetType);
	if (!Identidade.IsValid())
	{
		return false;
	}
	return ElementsOf(Biome).Contains(Identidade.Element);
}

TArray<FLoadedPetRecord> BiomeEncounterFilter::FilterByBiome(
	const TArray<FLoadedPetRecord>& Catalog, EIslandBiome Biome, bool bHasBiome)
{
	// Sem bioma (encontro sem mundo): o catálogo de sempre, inteiro. Filtrar
	// aqui esvaziaria a batalha aberta direto, sem lugar nenhum.
	if (!bHasBiome)
	{
		return Catalog;
	}

	TArray<FLoadedPetRecord> Filtrado;
	for (const FLoadedPetRecord& Registro : Catalog)
	{
		if (FitsBiome(Registro.Type, Biome))
		{
			Filtrado.Add(Registro);
		}
	}
	return Filtrado;
}
