// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * O PÁTIO DA ESCOLA: os campos de treino DENTRO da vila (decisão 63).
 *
 * A CI4 nasceu geometricamente impossível — a Escola chamaria uma função que
 * exige estar num campo, e os campos ficavam a vinte mil unidades. O usuário
 * decidiu pelo caminho oposto: a escola TEM os campos.
 *
 * PURO, como todo traçado: quem decide onde cada campo fica não precisa de
 * mundo, e "nenhum campo invade prédio" vira asserção que roda em
 * milissegundos — a mesma separação de `VillageLayout`.
 */
namespace SchoolyardLayout
{
	struct BATTLESQUARE_API FSchoolyardField
	{
		/** Na grafia do dado assinado ("musculature", ...). */
		FString Attribute;

		/** Relativo ao CENTRO da vila, como as peças do traçado. */
		FVector2D OffsetUnits = FVector2D::ZeroVector;

		float RadiusUnits = 0.0f;
	};

	/**
	 * Os campos do pátio deste tipo de assentamento.
	 *
	 * VAZIO onde não há Escola — o pátio é DELA, e vila sem escola com campos
	 * soltos seria a academia de graça que o traçado nega de propósito.
	 *
	 * Ficam na FAIXA DA CLAREIRA atrás da escola — entre o lote e a mata, o
	 * único chão da vila onde nem prédio nem árvore podem nascer.
	 */
	BATTLESQUARE_API TArray<FSchoolyardField> PlanFor(ESettlementKind Kind);
}
