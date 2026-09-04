// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O PET ENVELHECE (mundo-vivo, MV6). As três perguntas da spec, respondidas
 * pelas decisões do usuário:
 *
 *  - a idade tem FIM? SIM — o pet morre de velho (decisão 32);
 *  - o tempo passa com o jogo FECHADO? SIM (decisão 33), e por isso a idade é
 *    a MESMA subtração da idade do mundo que a mata usa (MV1): um carimbo de
 *    nascimento em idade-do-mundo, e idade = agora − nascimento. Offline-safe
 *    por construção, sem contador tickando;
 *  - o cuidado é ATIVO ou passivo? ATIVO (decisão 34): conviver e dar atenção
 *    REDUZ o envelhecimento e faz o pet viver mais. O tempo sozinho não salva;
 *    é a AÇÃO de cuidar que estica a vida.
 *
 * Os números (tempo de vida, quanto cada cuidado rende) são config (invariante
 * 2), passados aqui; a regra pura não lê `.ini` e é testável sem mundo.
 */
namespace PetAging
{
	/** Os números do envelhecimento — vêm de config, não do código. */
	struct BATTLESQUARE_API FAgingConfig
	{
		/** Tempo de vida natural do pet, em dias de mundo, sem cuidado nenhum. */
		int32 LifespanDays = 120;

		/** Quantos dias de vida cada ATO de cuidado desconta do envelhecimento. */
		float CareDaysPerAct = 2.0f;
	};

	/**
	 * A idade CRUA do pet: quanto o mundo envelheceu desde o nascimento dele.
	 * Nunca negativa (pet "do futuro" por relógio torto conta como zero).
	 */
	BATTLESQUARE_API int32 RawAgeInDays(int32 GenesisWorldAgeDays, int32 CurrentWorldAgeDays);

	/**
	 * A idade EFETIVA: a crua menos o que o cuidado ativo desfez. Cuidar
	 * rejuvenesce o relógio da morte — mas nunca abaixo de zero (cuidar não
	 * torna o pet mais novo que recém-nascido).
	 */
	BATTLESQUARE_API int32 EffectiveAgeInDays(
		int32 RawAgeDays, int32 CareActs, const FAgingConfig& Config);

	/**
	 * O pet morreu de velho? Só quando a idade EFETIVA alcança o tempo de vida.
	 * Tempo de vida não-positivo (config degenerada) NUNCA mata — pet imortal é
	 * melhor que pet que nasce morto por um zero esquecido no arquivo.
	 */
	BATTLESQUARE_API bool IsDeceased(int32 EffectiveAgeDays, const FAgingConfig& Config);
}
