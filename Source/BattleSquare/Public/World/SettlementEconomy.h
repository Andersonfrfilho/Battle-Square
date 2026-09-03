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

	/**
	 * O valor base de UM PET, em moeda.
	 *
	 * Hoje é o preço base da economia, para todos: raridade como número é
	 * decisão de conteúdo em aberto (a spec fala em "troca de pets por
	 * raridade" sem dizer quanto vale cada uma). Quando ela existir, muda AQUI
	 * — quem vende já multiplica pela porcentagem do lugar e não precisa saber.
	 */
	BATTLESQUARE_API int32 PetBaseValue();

	/**
	 * Quanto ESTE lugar paga por um pet, em moeda — a conta pronta.
	 *
	 * Existe para a tarefa de venda não refazer `valor × porcentagem / 100` no
	 * call site: a conta refeita é a tabela duplicada com outra roupa
	 * (invariante 15). Zero é "aqui não se vende".
	 */
	BATTLESQUARE_API int32 SalePayout(ESettlementKind Kind);

	/**
	 * O prêmio de UMA vitória na Arena deste lugar, em moeda — a conta pronta.
	 *
	 * Mesmo desenho do `SalePayout`, pelo mesmo motivo (invariante 15). Zero é
	 * "aqui não há Arena". A vila paga pouco (é o dinheiro do começo); o prêmio
	 * grande mora na cidade.
	 */
	BATTLESQUARE_API int32 RankingPrize(ESettlementKind Kind);
}
