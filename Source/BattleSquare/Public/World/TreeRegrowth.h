// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * CORTAR DEIXA MARCA, E A MARCA CICATRIZA (mundo-vivo, MV3).
 *
 * Uma árvore derrubada não some para sempre nem volta amanhã de graça: ela é
 * uma EXCEÇÃO com prazo, exatamente como `FBattleState` trata terreno temporário
 * de arena (base calculada + exceção que expira). Aqui a base é a mata que a
 * semente planta; a exceção é "esta foi cortada", e o prazo a apaga.
 *
 * O truque que dispensa o servidor de ter relógio próprio: o corte é carimbado
 * com a IDADE DO MUNDO (MV1) em que aconteceu. A rebrota é entao uma subtracao
 * pura de idades contra um prazo — determinística, sem relogio no meio, e o
 * prazo e numero de config (MV4), nao literal.
 */
namespace TreeRegrowth
{
	/**
	 * A árvore cortada em `CutAtWorldAgeDays` já rebrotou, vista de
	 * `CurrentWorldAgeDays`, dado o prazo `DeadlineDays`?
	 *
	 * Rebrotou quando o mundo envelheceu o prazo INTEIRO desde o corte. Prazo
	 * não-positivo (config degenerada) rebrota na hora — nunca trava a mata
	 * cortada para sempre. Idade atual anterior ao corte (relógio incoerente)
	 * NÃO rebrota: na dúvida a árvore continua cortada, que é o estado que
	 * alguém de fato causou.
	 */
	BATTLESQUARE_API bool HasRegrown(
		int32 CutAtWorldAgeDays, int32 CurrentWorldAgeDays, int32 DeadlineDays);
}
