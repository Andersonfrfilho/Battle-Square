// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"
#include "World/SettlementEconomy.h"

/**
 * DESASTRE INTERROMPE A VILA, QUE SE RECONSTRÓI (mundo-por-biomas, MB4 —
 * decisão 52).
 *
 * Liga dois sistemas que hoje não se falam: o evento de mundo (`WorldEvents` —
 * terremoto, furacão, tsunami, tudo puro e determinístico) e o serviço de um
 * assentamento (`SettlementEconomy::Offers`). Uma vila atingida acima de um
 * limiar fica INTERROMPIDA: para de oferecer serviço ENQUANTO o desastre passa.
 *
 * A decisão 52 é clara: a vila NÃO se abandona — ela se RECONSTRÓI (a única
 * exceção seria morrer toda a população, que não se modela aqui). Por isso a
 * interrupção é DERIVADA da janela do desastre (função pura de semente, lugar,
 * hora, como o próprio `WorldEvents`): passado o evento, a magnitude cai e a
 * vila volta a oferecer sozinha — reconstrução automática, nunca abandono
 * permanente. E, sendo derivada, o desastre não pode apagar o save de quem já
 * visitou: não há nada gravado para apagar.
 *
 * O limiar é número de config (invariante 2), passado aqui; a regra é pura.
 */
namespace SettlementDisaster
{
	/**
	 * A vila está INTERROMPIDA por um desastre agora, dada a magnitude que a
	 * atinge e o limiar? Limiar não-positivo (config degenerada) NUNCA
	 * interrompe — vila que some por um zero esquecido é pior que vila que
	 * resiste. É temporário por construção: quando a magnitude cai (o desastre
	 * passa), volta a ser falso — a vila se reconstruiu (decisão 52).
	 */
	BATTLESQUARE_API bool IsDisrupted(float DisasterMagnitude, float Threshold);

	/**
	 * O assentamento oferece este serviço AGORA?
	 *
	 * Interrompido pelo desastre, não oferece — os serviços caem ENQUANTO ele
	 * passa. Fora disso, é o `SettlementEconomy::Offers` de sempre. Uma fonte
	 * para a tabela de serviço; esta regra só a suspende durante a interrupção.
	 */
	BATTLESQUARE_API bool OffersDuringDisaster(
		ESettlementKind Kind, ESettlementService Service, bool bDisrupted);
}
