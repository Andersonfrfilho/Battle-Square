// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/IslandBakedPlan.h"

/**
 * O ACHADO (cidades-do-interior, decisão 57): dinheiro achado no MUNDO, fora da
 * trilha. É o que dá destino a ruína, clareira fechada e mercado-negro — sem o
 * achado, sair da trilha não pagava nada, e esses lugares eram só cenário.
 *
 * O valor é DETERMINÍSTICO da semente do LUGAR (regra 5 da geração procedural —
 * nunca do relógio nem de índice de laço): o mesmo canto do mundo guarda sempre
 * o mesmo achado, e dois jogadores acham o mesmo ali. Os números (mínimo e
 * máximo) são config (invariante 2).
 */
namespace WorldFinding
{
	/** Os números do achado — de config, não do código. */
	struct BATTLESQUARE_API FFindingConfig
	{
		int32 MinAmount = 10;
		int32 MaxAmount = 60;
	};

	/**
	 * Este uso do solo GUARDA achado? Só os destinos fora da trilha: ruína,
	 * clareira fechada e mercado-negro. Trilha, vila e fazenda não — o achado é
	 * prêmio por sair do caminho, não por andar nele (o contrapeso).
	 */
	BATTLESQUARE_API bool HasFinding(EGroundUse Use);

	/**
	 * Quanto se acha num lugar, dada a semente daquele lugar. Determinístico e
	 * dentro de [Min, Max]. Config degenerada (Min > Max) devolve Min, nunca um
	 * intervalo invertido.
	 */
	BATTLESQUARE_API int32 AmountAt(uint32 PlaceSeed, const FFindingConfig& Config);
}
