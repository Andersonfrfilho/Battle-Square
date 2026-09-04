// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"
#include "World/SettlementEconomy.h"

/**
 * CIDADE ABANDONADA DEPOIS DO DESASTRE (mundo-por-biomas, MB4).
 *
 * Liga dois sistemas que hoje não se falam: o evento de mundo (`WorldEvents` —
 * terremoto, furacão, tsunami, tudo puro e determinístico) e o serviço de um
 * assentamento (`SettlementEconomy::Offers`). Uma vila atingida acima de um
 * limiar é abandonada: para de oferecer o que oferecia.
 *
 * O ABANDONO É DERIVADO, NUNCA SALVO: ele é função pura de (semente, lugar,
 * hora), exatamente como o próprio `WorldEvents`. Por isso o desastre não pode
 * apagar retroativamente o save de quem já visitou a vila — não há nada
 * gravado para apagar. É o contrapeso da MB4, satisfeito por construção.
 *
 * O limiar é número de config (invariante 2), passado aqui; a regra é pura.
 */
namespace SettlementDisaster
{
	/**
	 * A vila foi abandonada, dada a magnitude do desastre que a atingiu e o
	 * limiar? Limiar não-positivo (config degenerada) NUNCA abandona — vila que
	 * some por um zero esquecido no arquivo é pior que vila que resiste.
	 */
	BATTLESQUARE_API bool IsAbandoned(float DisasterMagnitude, float Threshold);

	/**
	 * O assentamento oferece este serviço, DEPOIS do desastre?
	 *
	 * Abandonado, não oferece nada — os serviços caem. De pé, é o
	 * `SettlementEconomy::Offers` de sempre. Uma fonte para a tabela de
	 * serviço (SettlementEconomy); esta regra só a apaga quando a vila caiu.
	 */
	BATTLESQUARE_API bool OffersAfterDisaster(
		ESettlementKind Kind, ESettlementService Service, bool bAbandoned);
}
