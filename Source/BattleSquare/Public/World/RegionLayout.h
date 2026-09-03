// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O que cada assentamento da região É.
 *
 * **Cada um tem função PRÓPRIA**, e essa é a regra inteira. Três vilas iguais
 * é uma vila visitada três vezes: a segunda não acrescenta nada, e a viagem
 * até ela vira imposto. Por isso a vila inicial NÃO tem academia — assim a
 * primeira viagem tem motivo.
 *
 * Valores novos vão para o FIM: eles viajam em save e em traçado.
 */
UENUM()
enum class ESettlementKind : uint8
{
	/** Casa. Cura de graça e a Escola do treinador. Sem academia, de propósito. */
	VilaInicial,

	/** Treino PAGO de alguns atributos — o primeiro ralo da economia. */
	VilaDaAcademia,

	/** Troca de pets e o quadro de trabalhos: onde o repetido vira alguma coisa. */
	VilaDoMercado,

	/** A arena e o ranking da região. O clímax, e a chave da fronteira. */
	CidadeGrande,

	/**
	 * Posto de fronteira: a porta que só abre para quem venceu o ranking.
	 *
	 * É o que transforma a região de corredor em lugar que se conquista.
	 */
	PostoDeFronteira
};

struct BATTLESQUARE_API FSettlementPlacement
{
	ESettlementKind Kind = ESettlementKind::VilaInicial;

	/** Centro do assentamento, em coordenada de MUNDO. */
	FVector2D CenterUnits = FVector2D::ZeroVector;
};

/**
 * Onde cada assentamento da região fica — PURO, como o traçado da vila.
 *
 * Existe separado de `VillageLayout` porque as duas perguntas são diferentes:
 * lá é "onde o Centro de Recuperação fica DENTRO da vila", aqui é "onde a vila
 * fica dentro da ILHA". Misturá-las faria a distância entre vilas depender do
 * tamanho de um prédio.
 *
 * As distâncias são FRAÇÃO do raio da terra, nunca metros escritos à mão. Este
 * projeto já pagou por número absoluto escolhido quando só existia um tamanho:
 * os anéis das peças da ilha ficaram para trás quando o raio cresceu, e a
 * cinta de praia deixou de caber no bloco. Fração acompanha.
 */
namespace RegionLayout
{
	/** Os quatro assentamentos e os postos de fronteira, em ordem de visita. */
	BATTLESQUARE_API TArray<FSettlementPlacement> Plan();

	/**
	 * O nome do tipo de assentamento, para o painel e para o despejo.
	 *
	 * Mora aqui, junto do enum, e não em quem imprime. Esta tabela vivia dentro
	 * do teste de despejo — a TERCEIRA que este projeto achou escondida num
	 * teste enquanto a produção precisava dela, depois da do uso do solo e da
	 * do material da ponte. Duas cópias concordam até alguém acrescentar um
	 * tipo, e aí a nova sai como "?" no lugar que importa.
	 */
	BATTLESQUARE_API const TCHAR* KindDebugName(ESettlementKind Kind);

	/** Quantos postos de fronteira a região tem. */
	BATTLESQUARE_API int32 BorderPostCount();

	/** Distância do centro, por tipo. Fração do raio da terra. */
	BATTLESQUARE_API float DistanceFromCenterUnits(ESettlementKind Kind);
}
