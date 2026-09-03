// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "World/IslandBakedPlan.h"
#include "World/LandUseLayout.h"

/**
 * A CARTA SABE DIZER "ESCONDIDO" SEM DIZER O QUÊ.
 *
 * O gabarito era promessa de completude: ele afirmava um número por tipo de
 * mancha, e esconder qualquer coisa o reprovava. Isso deixava o mundo com duas
 * saídas ruins — ou o segredo aparecia na carta (e mercado-negro com placa não
 * é mercado-negro), ou ele virava exceção no gabarito. Exceção no gabarito é o
 * começo do gabarito não valer.
 *
 * A saída é a terceira contagem. A carta passa a afirmar MOSTRADO, ESCONDIDO e
 * a SOMA:
 *
 * - só o mostrado deixaria esconder e APAGAR terem a mesma cara;
 * - só a soma diria quanto há, e nada sobre o que se vê;
 * - as três juntas prendem as duas pontas.
 *
 * E contar não pode vazar QUAL. Por isso o que existe aqui é contagem, e nunca
 * uma lista de posições do que está escondido: quem quer as posições tem o
 * traçado, que é documento do mundo — a carta é o que se imprime.
 */
namespace IslandChart
{
	/** As três contagens de um tipo de mancha. */
	struct BATTLESQUARE_API FUseCount
	{
		int32 Shown = 0;
		int32 Hidden = 0;

		/**
		 * A soma é DERIVADA, nunca um terceiro campo que alguém preenche.
		 *
		 * Campo próprio poderia discordar dos outros dois, e um total que
		 * discorda das parcelas é pior que total nenhum.
		 */
		int32 Total() const { return Shown + Hidden; }
	};

	BATTLESQUARE_API FUseCount CountOf(TArrayView<const FBakedGroundUse> Patches, EGroundUse Use);
}
