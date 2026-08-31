// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O que fica montado à volta de quem anda — e o que se desfaz atrás.
 *
 * A ilha vai de 6000 para 20000 unidades de raio. Plantar tudo de uma vez
 * seria vinte vezes a área de hoje viva ao mesmo tempo, e o pedido do usuário
 * foi explícito: "não podemos deixar ficar devagar, devemos recarregar por
 * mapa". Este arquivo é o "por mapa".
 *
 * É PLANEJADOR PURO: conta sobre números, sem ator, sem mundo e sem `UObject`.
 * Quem monta e quem derruba é o GameMode; aqui só se decide O QUE deveria
 * estar montado. Separar assim é o que permite testar a residência inteira em
 * milissegundos, em vez de andar pela ilha olhando se a mata some.
 */
namespace RegionResidency
{
	/**
	 * O lado do pedaço, contado em REGIÕES de descoberta.
	 *
	 * Múltiplo, e não número solto: `FWorldDiscovery::RegionSizeUnits()` já
	 * significa "região" para o mapa e para a memória de onde se esteve. Um
	 * pedaço com lado próprio criaria uma segunda malha desalinhada da
	 * primeira, e "região" passaria a querer dizer duas coisas.
	 *
	 * Oito (6400 unidades) por aritmética de componentes: a mata de hoje tem
	 * ~400 instâncias num disco de 3200 de raio, o que dá ~500 plantas por
	 * pedaço deste tamanho. Menor que isso, cada pedaço carregaria as ~28
	 * malhas instanciadas da mata com quatro instâncias em cada — muita
	 * contabilidade para pouca planta.
	 */
	/**
	 * Lado do pedaço, em unidades. ABSOLUTO.
	 *
	 * Era `8 * RegionSizeUnits`, e a amarração quebrou quando a região virou
	 * fração da ilha: numa ilha de 1 km o pedaço daria 320 metros, e os nove
	 * vivos cobririam quase a ilha inteira — o streaming deixaria de streamar.
	 *
	 * Bloco é granularidade de STREAMING, decidida por desempenho. Região é
	 * resolução de MEMÓRIA e mapa, decidida pelo tamanho do mundo. Transmite-se
	 * fino e lembra-se grosso.
	 *
	 * 6.400 é o valor que a conta antiga dava com a ilha de hoje — zero
	 * mudança no mundo atual.
	 */
	constexpr float ChunkSideDefaultUnits = 6400.0f;

	/**
	 * Quantos pedaços de distância continuam montados.
	 *
	 * Um: o pedaço de quem anda mais os oito à volta, 3 por 3, cobrindo 19200
	 * unidades de lado. O horizonte da mata não chega perto disso, então
	 * ninguém vê a borda; e são nove pedaços vivos em vez dos vinte e cinco de
	 * um raio 2.
	 */
	constexpr int32 ResidentRadiusInChunks = 1;

	/** O lado do pedaço em unidades de mundo. */
	BATTLESQUARE_API float ChunkSideUnits();

	/** Em que pedaço cai o ponto. */
	BATTLESQUARE_API FIntPoint ChunkAt(const FVector2D& PositionUnits);

	/** O centro daquele pedaço, que é onde o ator dele nasce. */
	BATTLESQUARE_API FVector2D ChunkCenterUnits(const FIntPoint& Chunk);

	/**
	 * O pedaço encosta em terra em algum ponto.
	 *
	 * Pedaço inteiramente no mar não ganha ator nenhum: a água já é o fim do
	 * mundo, e povoar o oceano com mata vazia custaria o mesmo que povoar a
	 * ilha. É esta pergunta que faz o custo acompanhar a TERRA e não a área do
	 * quadrado que a contém.
	 */
	BATTLESQUARE_API bool TouchesLand(const FIntPoint& Chunk);

	/**
	 * A semente daquele pedaço.
	 *
	 * Deriva da semente do mundo e das coordenadas, então o mesmo pedaço
	 * nasce igual toda vez que se volta — sair andando e voltar não pode
	 * reembaralhar a floresta que se acabou de atravessar.
	 */
	BATTLESQUARE_API uint32 ChunkSeed(uint32 WorldSeed, const FIntPoint& Chunk);

	/** O que montar e o que derrubar para chegar ao estado certo. */
	struct FResidencyChange
	{
		/** Pedaços que deveriam existir e ainda não existem. */
		TArray<FIntPoint> ToBuild;

		/** Pedaços vivos que saíram do alcance. */
		TArray<FIntPoint> ToDrop;
	};

	/**
	 * A diferença entre o que está vivo e o que deveria estar.
	 *
	 * Devolve DELTA, não a lista inteira: quem chama aplica só o que mudou, e
	 * andar de um lado a outro do mesmo pedaço não derruba nem remonta nada.
	 * Uma função que devolvesse "os nove certos" obrigaria o chamador a
	 * refazer esta mesma conta para saber o que já tinha.
	 */
	BATTLESQUARE_API FResidencyChange PlanChange(
		const TSet<FIntPoint>& LiveChunks, const FVector2D& PlayerPositionUnits);
}
