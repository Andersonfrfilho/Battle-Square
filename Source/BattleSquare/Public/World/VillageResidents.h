// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * QUEM MORA NAS CASAS (decisão 65) — puro, e determinístico pela geometria.
 *
 * O morador sai da vila e do índice da porta, nunca do relógio: vizinho que
 * troca de nome a cada visita não é vizinho — é sorteio com telhado. Mesma
 * regra 5 da geração procedural que dá o mesmo campeão à mesma Arena.
 *
 * E ele diz UMA coisa VERDADEIRA: cada fala do repertório aponta uma mecânica
 * que existe e tem teste. Morador não fala de segredo — a carta conta e não
 * aponta (J4), e o vizinho tagarela seria a exceção pela porta dos fundos.
 *
 * O diálogo FUNDO — história própria, memória, evolução — é a decisão 15, e
 * é feature própria. Isto é a fundação dela: o morador existe, tem nome
 * estável e tem voz.
 */
namespace VillageResidents
{
	struct BATTLESQUARE_API FResident
	{
		FString Name;

		/** O que ele diz ao receber visita. Verdade do mundo, sempre. */
		FString TipLine;
	};

	/** O morador da porta `DoorIndex` da vila deste tipo. Sempre o mesmo. */
	BATTLESQUARE_API FResident ResidentFor(ESettlementKind Kind, int32 DoorIndex);
}
