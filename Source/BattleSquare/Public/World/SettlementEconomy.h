// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * O que um assentamento faz com o dinheiro.
 *
 * Valores novos no FIM: eles viajam em save.
 */
UENUM()
enum class ESettlementService : uint8
{
	/** Curar o pet. Só a vila inicial faz de graça, e para sempre. */
	Cura,

	/** Treino rápido de atributo. O dreno principal. */
	Academia,

	/** Vender o pet capturado. Troco, nunca salário. */
	Venda,

	/** O prêmio do ranking daqui. A fonte. */
	PremioDeRanking
};

/**
 * Preços por assentamento — PURO, e em PORCENTAGEM.
 *
 * Nada aqui é moeda escrita à mão, e isso é a regra inteira. Preço absoluto
 * espalhado por lugar é o mesmo defeito que deixou os anéis das peças da ilha
 * para trás quando o raio da ilha cresceu: número escolhido quando só existia
 * um caso. Com multiplicador, equilibrar a economia é mexer numa tabela.
 *
 * A decisão que este módulo carrega: **a cidade grande tem toda função e o
 * pior preço de cada uma.** É o que mantém as três vilas vivas depois que ela
 * abre. Cidade barata e completa mataria as vilas no instante em que abrisse,
 * e a região viraria um corredor até ela.
 *
 * A carteira ainda não existe. Enquanto não existir, isto é tabela com teste,
 * não preço na tela — prédio com porta que não abre é promessa quebrada.
 */
namespace SettlementEconomy
{
	/** O lugar oferece o serviço. */
	BATTLESQUARE_API bool Offers(ESettlementKind Kind, ESettlementService Service);

	/**
	 * Quanto o serviço custa aqui, em porcentagem do preço base.
	 *
	 * Zero é DE GRAÇA, e é o caso da cura em casa. Lugar que não oferece
	 * responde zero também, e por isso `Offers` vem sempre antes: quem
	 * confundir os dois faz a cidade curar de graça.
	 */
	BATTLESQUARE_API int32 PricePercent(ESettlementKind Kind, ESettlementService Service);

	/**
	 * Quanto o lugar PAGA, em porcentagem do valor base.
	 *
	 * Vale para venda e para prêmio de ranking — os dois lados de fonte. O
	 * mercado paga o melhor pelo pet; a cidade paga o melhor prêmio.
	 */
	BATTLESQUARE_API int32 PayoutPercent(ESettlementKind Kind, ESettlementService Service);
}
