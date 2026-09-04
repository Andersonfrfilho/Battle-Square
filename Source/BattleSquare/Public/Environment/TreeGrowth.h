// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A ÁRVORE CRESCE COM A IDADE DO MUNDO (mundo-vivo, MV2).
 *
 * O tamanho de cada árvore é semente + IDADE DO MUNDO — nunca a idade da árvore
 * individual. Duas árvores plantadas no mesmo instante (a mesma semente) crescem
 * sempre JUNTAS; dar a cada uma seu próprio relógio criaria uma segunda fonte de
 * idade para a mesma pergunta, o defeito de L-032/L-033. Por isso a idade que
 * entra aqui é a do MUNDO, uma só, e ela multiplica apenas a ESCALA — a posição
 * e a espécie seguem vindo só da semente (o contrapeso da MV2).
 *
 * A curva é lógica; os NÚMEROS dela (quantos dias até a maturidade, o tamanho de
 * muda) são configuração (invariante 2), lidos de `DefaultGame.ini` e passados
 * aqui — a regra pura não lê `.ini` nenhum, e é isso que a torna testável sem
 * mundo.
 */
namespace TreeGrowth
{
	/** Os números da curva de crescimento — vêm de config, não do código. */
	struct BATTLESQUARE_API FGrowthConfig
	{
		/** Dias de idade do mundo até a árvore atingir o tamanho adulto. */
		int32 DaysToMaturity = 30;

		/** O tamanho de uma muda recém-nascida, como fração do adulto. */
		float SaplingScale = 0.35f;
	};

	/**
	 * O multiplicador de escala para uma árvore, dada a idade do mundo.
	 *
	 * Cresce de `SaplingScale` (no dia 0) até 1.0 (na maturidade) e SATURA ali —
	 * a árvore não cresce para sempre. Monótono: mundo mais velho nunca encolhe
	 * a mata. Idade desconhecida/negativa NÃO some com a árvore: cai no adulto
	 * (1.0), porque uma mata que existe há tempo demais para saber é uma mata
	 * madura, nunca uma muda nem um vazio.
	 */
	BATTLESQUARE_API float ScaleFactorFor(int32 WorldAgeInDays, const FGrowthConfig& Config);
}
