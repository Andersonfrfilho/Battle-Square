// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/IslandGeography.h"

/**
 * O ORÇAMENTO DO MUNDO: quanto de cada coisa um bioma tem.
 *
 * Esta é a peça que faltava para o gerador servir a mais de uma ilha. Antes
 * cada número morava no módulo que o usava — quantos bosques em `LandUseLayout`,
 * quanta água em `FreshWater`, quantas cavernas em `IslandFeatureLayout` — e
 * fazer um deserto significaria caçar constante por constante em seis arquivos.
 *
 * Aqui é um lugar só, e a pergunta que se responde é sempre a mesma: **quanto
 * disto este bioma tem?**
 *
 * ## As três regras que este arquivo protege
 *
 * 1. **Porcentagem e contagem, nunca posição.** Onde as coisas ficam é
 *    decidido pelos geradores, olhando o terreno. Aqui só se diz QUANTO.
 *
 * 2. **Fração de área, nunca unidades.** Um número absoluto escolhido quando
 *    só existia um tamanho é a armadilha mais cara deste projeto — ela apareceu
 *    medida em sete lugares diferentes.
 *
 * 3. **Todo valor tem de ser verificável no despejo.** Orçamento que ninguém
 *    mede é desejo: a água pedia 6% e entregava 35%, e só um número na tela
 *    mostrou.
 */
namespace WorldBudget
{
	/** Fração da TERRA coberta por água doce. */
	BATTLESQUARE_API float WaterCoverage(EIslandBiome Biome);

	/** Quantos bosques, clareiras fechadas e criadouros. */
	BATTLESQUARE_API int32 GroveCount(EIslandBiome Biome);
	BATTLESQUARE_API int32 HiddenClearingCount(EIslandBiome Biome);
	BATTLESQUARE_API int32 BreederCount(EIslandBiome Biome);

	/** Fazendas por assentamento, e pomares. */
	BATTLESQUARE_API int32 FarmsPerSettlement(EIslandBiome Biome);
	BATTLESQUARE_API int32 TendedOrchardCount(EIslandBiome Biome);
	BATTLESQUARE_API int32 WildOrchardCount(EIslandBiome Biome);

	/** Lojas de beira de estrada e acampamentos. */
	BATTLESQUARE_API int32 RoadsideShopCount(EIslandBiome Biome);
	BATTLESQUARE_API int32 CampCount(EIslandBiome Biome);

	/**
	 * Templos em pé e templos em ruínas.
	 *
	 * Os em pé saem do PANTEÃO — um por deus que tenha lugar neste bioma — e
	 * por isso não têm número próprio. As ruínas têm: elas falam do passado, e
	 * quanto passado um lugar tem é decisão de mundo.
	 */
	BATTLESQUARE_API int32 RuinCount(EIslandBiome Biome);

	/**
	 * QUE FRAÇÃO das galerias pode ir em linha reta.
	 *
	 * Reta não é defeito por existir: fratura de calcário às vezes é reta
	 * mesmo, e uma rede em que NADA é reto lê tão fabricada quanto uma em que
	 * tudo é. O defeito era a proporção — 123 de 158, porque a costura riscava
	 * enquanto o gerador cavava.
	 *
	 * É parâmetro de bioma porque a rocha muda: calcário fraturado do pântano
	 * abre galeria reta com frequência, o basalto da geleira quase nunca.
	 */
	BATTLESQUARE_API float StraightGalleryShare(EIslandBiome Biome);

	/**
	 * QUE FRAÇÃO das grutas se liga a alguma outra por baixo.
	 *
	 * "Algumas" é medição, nunca impressão. Era sobra da distância — emendava
	 * quem tivesse chegado perto, e a proporção caía onde caísse (2 de 8,
	 * medido em 03/09/2026). Número que ninguém escolheu é número que ninguém
	 * pode defender.
	 *
	 * Nunca zero e NUNCA TODAS, e o "todas" é o contrapeso que importa: com o
	 * subsolo inteiro ligado, achar passagem deixa de ser achado — vira andar
	 * num corredor só. Por isso o gerador também trava o teto, e não confia no
	 * teste para isso.
	 *
	 * É parâmetro de bioma porque quem liga é a rocha: o calcário do pântano
	 * dissolve e junta tudo, o tubo de lava do vulcão já nasce corredor, e o
	 * basalto da geleira quase não abre caminho.
	 */
	BATTLESQUARE_API float LinkedGrottoShare(EIslandBiome Biome);

	/**
	 * QUANTOS MERCADOS-NEGROS, e o piso que o dono do mundo fixou.
	 *
	 * O piso é decisão do usuário — "pelo menos 3 bem espalhados" —, e por
	 * isso ele é função própria, e não um número escrito dentro de quem
	 * planta: piso guardado no ponto de uso é piso que a próxima edição
	 * esquece. Quantos passam disso é decisão de bioma.
	 *
	 * "Bem espalhados" NÃO mora aqui: aqui só se diz quanto (regra 1 deste
	 * arquivo). Onde eles ficam é do gerador, e o afastamento sai do raio da
	 * ilha.
	 */
	BATTLESQUARE_API int32 BlackMarketFloor();
	BATTLESQUARE_API int32 BlackMarketCount(EIslandBiome Biome);

	/** Quantos cemitérios cada assentamento tem. */
	BATTLESQUARE_API int32 GraveyardsPerSettlement(EIslandBiome Biome);

	/** E quantos ficaram para trás, sem vila nenhuma em volta. */
	BATTLESQUARE_API int32 ForgottenGraveyardCount(EIslandBiome Biome);

	/**
	 * Quanto a mata é densa, como multiplicador da densidade base.
	 *
	 * Zero é bioma sem árvore: o deserto e a geleira não têm mata para adensar,
	 * e um bosque num deles seria contradição.
	 */
	BATTLESQUARE_API float ForestDensity(EIslandBiome Biome);
}
