// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * O AQUEDUTO: a água que vem de longe porque a vila não tem perto.
 *
 * Ele nasce de uma regra medida. Toda cidade capta água de algum lugar, e a
 * Vila do Bloco Zero estava a **288 metros** de qualquer gota — ela fica no
 * centro da ilha, e o miolo é seco por construção: é ali que o jogador nasce, e
 * um rio cortando a praça inicial seria obstáculo no primeiro passo do jogo.
 *
 * Mover a vila não era opção, e cavar um rio até ela desfaria a decisão. O que
 * uma cidade real faz nessa situação é trazer a água — e trazer a água é uma
 * OBRA, que se vê, que custa, e que diz do lugar que ela é importante o
 * bastante para merecê-la.
 *
 * ## A gravidade é a regra
 *
 * Aqueduto não bombeia: ele desce. A captação tem de estar MAIS ALTA que a
 * cidade, e por isso ela nem sempre é a água mais próxima — é a água mais
 * próxima que ainda está acima.
 *
 * Sem essa amarra o aqueduto vira um cano mágico, e some justamente o que o
 * torna interessante: a obra tem de contornar o terreno.
 */
namespace AqueductLayout
{
	struct BATTLESQUARE_API FAqueduct
	{
		ESettlementKind Serves = ESettlementKind::VilaInicial;

		/** Da captação até a vila, ponto a ponto. */
		TArray<FVector2D> PointsUnits;

		/** Quanto ela desce no percurso. Zero seria água parada num cano. */
		float DropUnits = 0.0f;
	};

	/** Os aquedutos da região. Vila com água perto não tem, e não precisa. */
	BATTLESQUARE_API const TArray<FAqueduct>& Plan();

	/** A partir de que distância a vila precisa de um. */
	BATTLESQUARE_API float ThirstyBeyondUnits(ESettlementKind Kind);

	/** Meia largura da calha — ela é estreita: transporta, não alaga. */
	BATTLESQUARE_API float HalfWidthUnits();
}
