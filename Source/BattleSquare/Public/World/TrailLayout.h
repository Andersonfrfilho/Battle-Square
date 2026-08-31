// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/** Uma trilha ligando dois assentamentos. */
struct BATTLESQUARE_API FTrailRoute
{
	ESettlementKind From = ESettlementKind::VilaInicial;
	ESettlementKind To = ESettlementKind::VilaInicial;

	/** O caminho, ponto a ponto. Nunca reto: o terreno é que o entorta. */
	TArray<FVector2D> PointsUnits;
};

/**
 * As trilhas — CALCULADAS, não desenhadas.
 *
 * A regra vem da spec da montaria: **a trilha vai pelo caminho barato**. Ela
 * não é uma curva bonita que alguém traçou; ela é o caminho de menor custo
 * sobre o relevo, e curva porque o TERRENO a curva. Contornar o morro é o
 * resultado, não a decisão.
 *
 * Por isso ela usa `IslandGeography::TravelCostBetween`, a mesma conta que
 * cobra o cansaço de quem anda. Se o traçador usasse outra, a trilha passaria
 * pelo caminho que ela mesma diz ser caro — e ninguém entenderia por quê.
 *
 * Trilha, e não estrada: uma faixa sem árvore com o chão de outra cor. Asfalto
 * e meio-fio implicariam uma civilização que este mundo não tem.
 */
namespace TrailLayout
{
	/** Todas as trilhas da região. */
	BATTLESQUARE_API const TArray<FTrailRoute>& Plan();

	/** Meia largura da faixa limpa. Fração do lote, nunca metros à mão. */
	BATTLESQUARE_API float HalfWidthUnits();

	/** O ponto está em cima de alguma trilha. */
	BATTLESQUARE_API bool IsOnTrail(const FVector2D& PositionUnits);

	/**
	 * Onde uma trilha cruza um rio — é ali que a PONTE fica.
	 *
	 * A ponte não é enfeite: sem ela a trilha entra na água, e um caminho que
	 * afunda é pior que caminho nenhum, porque promete passagem.
	 */
	BATTLESQUARE_API TArray<FVector2D> BridgePoints();

	/** O tamanho do passo do traçado — também a distância entre pontos. */
	BATTLESQUARE_API float StepUnits();
}
