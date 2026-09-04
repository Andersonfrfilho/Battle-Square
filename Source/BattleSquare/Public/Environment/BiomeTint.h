// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/IslandGeography.h"

/**
 * A COR DE UM BIOMA (mundo-por-biomas, MB3) — a fonte ÚNICA.
 *
 * A vila é molde: o mesmo prédio muda de paleta por bioma. Mas não existia
 * nenhuma função bioma→cor no projeto (a arena "veste o bioma" escolhendo flora
 * e chão por bioma, não tingindo cor). Esta é essa fonte, e é UMA: a vila a
 * chama, e quem mais precisar tingir por bioma a chama também — nunca uma
 * segunda tabela cor×bioma (L-032/L-033).
 *
 * É um MULTIPLICADOR, não uma cor absoluta: a identidade do prédio (o vermelho
 * do Centro, o roxo do Mercado) sobrevive, só desloca de clima. Todo canal é
 * positivo de propósito — zerar um canal colapsaria prédios diferentes na mesma
 * cor, o que a MB3 proíbe.
 */
namespace BiomeTint
{
	/** O tom que o bioma empresta — multiplica a cor-base de qualquer peça. */
	BATTLESQUARE_API FLinearColor Of(EIslandBiome Biome);
}
