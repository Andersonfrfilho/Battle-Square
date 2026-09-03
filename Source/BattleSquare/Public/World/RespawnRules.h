// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * ONDE SE ACORDA depois de cair — PURO, como todo traçado deste mundo.
 *
 * A decisão 60: o Marco é ponto de renascimento, e quem cai acorda no Centro
 * de Recuperação MAIS PERTO de onde caiu — nunca no de casa. A fresta que isso
 * fecha tem nome: se acordar levasse à vila natal, morrer seria viagem rápida
 * de graça, exatamente o que a decisão 17 proíbe pela porta dos fundos.
 */
namespace RespawnRules
{
	/** Onde acordar: a vila, e o ponto exato — na PORTA do Centro, não dentro. */
	struct BATTLESQUARE_API FWakeSpot
	{
		ESettlementKind Kind{};
		FVector2D VillageCenterUnits = FVector2D::ZeroVector;
		FVector2D WakeUnits = FVector2D::ZeroVector;
	};

	/**
	 * O Centro de Recuperação mais perto de onde se caiu.
	 *
	 * Devolve false quando nenhuma vila da região tem o prédio — resposta
	 * válida, nunca um lugar inventado: acordar numa vila sem hospital seria a
	 * carta prometendo o que o traçado não tem.
	 */
	BATTLESQUARE_API bool NearestRecoveryCenter(const FVector2D& FellAtUnits, FWakeSpot& OutSpot);
}
