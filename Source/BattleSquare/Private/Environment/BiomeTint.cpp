// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/BiomeTint.h"

FLinearColor BiomeTint::Of(EIslandBiome Biome)
{
	// Tons distintos por bioma, todos os canais > 0 para nunca colapsar a
	// identidade do prédio. Distintos o bastante para nenhum par de biomas
	// empatar a cor do mesmo prédio.
	switch (Biome)
	{
	case EIslandBiome::Forest:  return FLinearColor(0.85f, 1.05f, 0.80f); // verde de mata
	case EIslandBiome::Desert:  return FLinearColor(1.12f, 1.00f, 0.76f); // areia quente
	case EIslandBiome::Glacier: return FLinearColor(0.84f, 0.94f, 1.16f); // azul frio
	case EIslandBiome::Volcano: return FLinearColor(1.16f, 0.80f, 0.74f); // brasa
	case EIslandBiome::Beach:   return FLinearColor(0.88f, 1.08f, 1.12f); // ciano de orla
	case EIslandBiome::Swamp:   return FLinearColor(0.80f, 0.96f, 0.82f); // verde turvo
	}
	return FLinearColor::White;
}
