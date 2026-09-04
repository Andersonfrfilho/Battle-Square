// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A IDADE DO MUNDO no cliente (mundo-vivo, MV1).
 *
 * O servidor e dono do relogio (a data de nascimento mora la, MV1 backend); o
 * cliente so LE e exibe. O ponto crucial e o contrapeso obrigatorio da task:
 * quando o backend esta fora do ar, a idade e DESCONHECIDA — nunca zero. Zero
 * pareceria um mundo recem-nascido, e um recem-nascido e uma afirmacao; "nao
 * sei" e a verdade quando a leitura falhou.
 *
 * Por isso "conhecida" e "desconhecida" sao estados DISTINTOS aqui, e um mundo
 * de idade zero NAO se confunde com uma leitura que nao chegou.
 */
namespace WorldAge
{
	/** A idade do mundo como o cliente a conhece — ou a ausencia dela. */
	struct BATTLESQUARE_API FWorldAge
	{
		/** A leitura chegou? Falso e o fallback: backend fora, idade ignorada. */
		bool bKnown = false;

		/** Dias de idade — so significa algo quando bKnown e verdadeiro. */
		int32 AgeInDays = 0;
	};

	/** O fallback: idade desconhecida. O estado inicial e o de falha de leitura. */
	BATTLESQUARE_API FWorldAge Unknown();

	/** Uma idade que chegou do servidor. */
	BATTLESQUARE_API FWorldAge Known(int32 AgeInDays);

	/** O que o jogador le na tela — "desconhecida" jamais vira "0 dias". */
	BATTLESQUARE_API FText Describe(const FWorldAge& Age);
}
