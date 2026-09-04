// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * ANDAR MONTADO USA O CUSTO DE TRAJETO QUE JÁ EXISTE (montaria-e-trilhas, MT1).
 *
 * Montar não inventa uma segunda física de ladeira: a velocidade sai da MESMA
 * conta de custo que a trilha já usa (`UphillCostWeight`/`DownhillCostWeight`).
 * Montar só multiplica a velocidade-base — a PROPORÇÃO entre subida e descida
 * continua sendo a dos pesos, nunca uma tabela paralela.
 *
 * O multiplicador de montaria é config (invariante 2), passado aqui; a regra é
 * pura.
 */
namespace MountedMovement
{
	/**
	 * A velocidade efetiva num trecho, dada a velocidade-base e o PESO de custo
	 * daquele trecho (o mesmo que a trilha usa: subida pesa mais que descida).
	 *
	 * Mais custo, mais devagar. Peso não-positivo (impossível pela API, mas
	 * defensivo) devolve a base, nunca divide por zero.
	 */
	BATTLESQUARE_API float SpeedOnStretch(float BaseSpeed, float SlopeCostWeight);

	/**
	 * A velocidade-base MONTADO: a de a pé multiplicada pelo ganho de montaria.
	 * Multiplicador <= 1 não faz montar ser mais lento — piso em 1 (montar
	 * nunca atrapalha), mas o normal é > 1.
	 */
	BATTLESQUARE_API float MountedBaseSpeed(float OnFootSpeed, float MountMultiplier);
}
