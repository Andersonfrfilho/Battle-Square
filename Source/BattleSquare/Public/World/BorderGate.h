// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * O PORTÃO SÓ ABRE PARA QUEM VENCEU O RANKING (mundo-por-biomas, MB1).
 *
 * O Posto de Fronteira é "a porta que só abre para quem venceu o ranking" — até
 * agora, só um comentário no enum. Esta regra o torna real, e SÓ ele: nenhuma
 * outra passagem da ilha (vila–vila, vila–cidade) trava, porque só o Posto a
 * spec amarra ao ranking. O contrapeso — travar tudo faria a ilha inteira uma
 * prisão — é parte da própria regra.
 *
 * Recorte deliberado: não há placar nem pontuação aqui. A regra lê um único
 * booleano por região ("venceu o ranking?"); QUEM vira esse booleano é outra
 * frente.
 */
namespace BorderGate
{
	/**
	 * Este assentamento deixa passar, dado se a região teve o ranking vencido?
	 *
	 * Só o Posto de Fronteira consulta o ranking; todo o resto passa sempre. O
	 * Posto passa apenas quando `bRegionRankingWon` — e barra quando não.
	 */
	BATTLESQUARE_API bool AllowsPassage(ESettlementKind Kind, bool bRegionRankingWon);
}
