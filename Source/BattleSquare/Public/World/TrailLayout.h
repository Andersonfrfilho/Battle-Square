// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * Onde uma trilha termina.
 *
 * Nem toda trilha vai de vila a vila. Quem anda vai ver a cachoeira e subir o
 * monte — e uma cachoeira sem caminho é a cachoeira que o relato de jogo disse
 * nunca ter visto.
 *
 * E é isto que faz a PONTE existir: os rios correm do monte para o mar, e
 * enquanto todo destino ficava ENTRE eles, nenhuma trilha precisava atravessar
 * um. Um caminho até a queda tem de chegar na margem.
 */
enum class ETrailDestination : uint8
{
	Assentamento,
	Cachoeira,
	Monte
};

/** Uma trilha ligando dois lugares. */
struct BATTLESQUARE_API FTrailRoute
{
	ESettlementKind From = ESettlementKind::VilaInicial;
	ESettlementKind To = ESettlementKind::VilaInicial;

	ETrailDestination Destination = ETrailDestination::Assentamento;

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
	/**
	 * O que a trilha faz onde ela encontra água.
	 *
	 * Não é sempre ponte, e essa era a distorção: com água em toda parte, a
	 * região ganhou 483 pontes. Quem cruza um córrego de três metros molha o
	 * pé; ponte ali é obra sem motivo, e denuncia que ninguém pensou na
	 * travessia.
	 */
	enum class ECrossingKind : uint8
	{
		/** VAU: raso o bastante para passar andando. Não constrói nada. */
		Vau,

		/** PONTE: fundo demais para passar, e a margem permite apoiar. */
		Ponte,

		/**
		 * BARRANCO: a margem é alta, e a travessia é um corte na terra.
		 *
		 * Ele existe porque ponte precisa de duas margens no mesmo nível. Onde
		 * uma delas é um degrau, o que se faz é cavar a descida — e isso é uma
		 * coisa diferente de construir, com outra silhueta.
		 */
		Barranco
	};

	struct BATTLESQUARE_API FCrossing
	{
		FVector2D CenterUnits = FVector2D::ZeroVector;
		ECrossingKind Kind = ECrossingKind::Vau;

		/** A fundura estimada da água ali, que é o que decide. */
		float DepthUnits = 0.0f;
	};

	/** Onde cada trilha encontra água, e o que se faz ali. */
	BATTLESQUARE_API TArray<FCrossing> Crossings();

	/** Só as travessias que viram ponte — o que o mundo constrói. */
	BATTLESQUARE_API TArray<FVector2D> BridgePoints();

	/** A fundura a partir da qual não se passa a pé. */
	BATTLESQUARE_API float WadableDepthUnits();

	/** O tamanho do passo do traçado — também a distância entre pontos. */
	BATTLESQUARE_API float StepUnits();
}
