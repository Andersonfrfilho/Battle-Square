// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Battle/FluidRegistry.h"

class UIslandBakedPlan;

/**
 * O QUE O PÉ ENCONTRA num ponto da ilha.
 *
 * A água precisa MOLHAR. Um rio desenhado que não muda nada ao ser pisado é
 * enfeite: ele lê como obstáculo e se comporta como chão, e é essa promessa
 * quebrada que faz a travessia — as 56 do traçado — perder o sentido.
 */
enum class EWaterFooting : uint8
{
	/** Terra. */
	Seco,

	/** VAU: raso o bastante para passar andando, molhando o pé. */
	Vau,

	/** FUNDO: aqui não se anda. Atravessa-se nadando, ou por obra. */
	Fundo
};

namespace WaterFooting
{
	/**
	 * O que há sob os pés nesta posição.
	 *
	 * A LARGURA decide, e é a mesma largura que desenha o rio — passa por
	 * `FreshWater::NavigabilityForHalfWidth`, que já é a regra. Uma segunda
	 * tabela de fundura concordaria com a primeira até alguém alargar um rio.
	 */
	BATTLESQUARE_API EWaterFooting At(const UIslandBakedPlan& Baked,
		const FVector2D& PositionUnits);

	/**
	 * O que o pé encontra PARA QUEM TEM ESTA ALTURA.
	 *
	 * A água não é funda: ela é funda **para alguém**. Um pet miúdo se molha
	 * onde um corpulento passa seco, e a mesma poça é vau para um e nado para
	 * o outro.
	 *
	 * `At` sem altura continua existindo e usa a altura padrão — é o mundo
	 * perguntando "e para uma pessoa comum?", que é a pergunta certa quando
	 * ninguém em particular está pisando.
	 */
	BATTLESQUARE_API EWaterFooting AtForHeight(const UIslandBakedPlan& Baked,
		const FVector2D& PositionUnits, float HeightUnits);

	/**
	 * A CINTURA de quem tem esta altura — o limiar entre andar e nadar.
	 *
	 * **40% da altura**, e não uma constante. As três âncoras que este projeto
	 * tinha brigavam entre si — o limiar do traçado (100), a meia-altura da
	 * cápsula do jogador (88), e a fundura das travessias de vau (todas abaixo
	 * de 94) —, e nenhuma podia ganhar sem tornar as outras erradas.
	 *
	 * Como fração, elas param de brigar: cada uma vira a resposta certa para
	 * uma altura diferente.
	 */
	BATTLESQUARE_API float WaistDepthUnitsFor(float HeightUnits);

	/**
	 * QUANTA ÁGUA há sob os pés, em unidades.
	 *
	 * Sai da MESMA medição que decide se o pé passa — uma função própria que
	 * percorresse os rios de novo seria a segunda fonte da mesma verdade.
	 *
	 * Zero é "não sei", e não "raso": é terra seca, é córrego e fonte (que não
	 * têm fundura por ponto), e é o mar, que não está no traçado como curso.
	 * Quem imprime o número decide não imprimir o zero — ausência não ocupa
	 * linha no painel.
	 */
	BATTLESQUARE_API float DepthUnitsAt(const UIslandBakedPlan& Baked,
		const FVector2D& PositionUnits);

	/** A altura de uma pessoa comum deste mundo, para quem não disse a sua. */
	BATTLESQUARE_API float DefaultHeightUnits();

	/**
	 * Quanto o passo rende aqui, como fração do passo em terra.
	 *
	 * Fração, e não velocidade em unidades: número absoluto escolhido quando só
	 * existia uma velocidade é a armadilha que este projeto já mediu em sete
	 * lugares.
	 */
	BATTLESQUARE_API float SpeedMultiplierFor(EWaterFooting Footing);

	BATTLESQUARE_API const TCHAR* DebugName(EWaterFooting Footing);

	/**
	 * Quanto o passo rende AQUI, INDO PARA LÁ.
	 *
	 * `SpeedMultiplierFor` sozinho atrasa igual nos dois sentidos, e uma água
	 * que atrapalha tanto quem desce quanto quem sobe não é uma corrente — é
	 * um pântano com desenho de rio. O sentido só vira jogo quando ele
	 * DIFERENCIA: descer o curso rende mais que subir, e é essa diferença que
	 * transforma "onde eu atravesso" numa escolha.
	 *
	 * `Heading` é para onde se anda; vetor nulo (parado) devolve o fator seco
	 * da água, sem bônus nem castigo — quem não anda não anda a favor de nada.
	 *
	 * O empurrão entra pela COMPONENTE AO LONGO do rumo, como na balsa: a
	 * parte perpendicular empurra de lado, e empurrar de lado não é atraso.
	 *
	 * **Há piso**, e ele é a mesma lição da travessia: correnteza forte o
	 * bastante para ZERAR o passo prende o jogador dentro do rio para sempre,
	 * e o que ele vê não é uma água difícil — é um jogo travado.
	 */
	BATTLESQUARE_API float SpeedMultiplierAlong(EWaterFooting Footing,
		const FVector2D& Heading, const FVector2D& Flow, int32 StrengthPerMille);

	/**
	 * DE QUE FLUIDO é a água que se está pisando.
	 *
	 * O mundo tinha seis fluidos e tratava todos como uma água só. Quem entra
	 * numa fonte termal na saia do vulcão sentia o mesmo que quem atravessa um
	 * córrego de montanha — e o registro, que existe para separá-los, não
	 * chegava até aqui.
	 *
	 * Devolve `Nenhum` em terra firme. Fora da costa é ÁGUA SALGADA: o mar não
	 * está no traçado como curso, ele é o que sobra depois que a terra acaba.
	 */
	BATTLESQUARE_API EFluidKind FluidAt(const UIslandBakedPlan& Baked,
		const FVector2D& PositionUnits);

	/**
	 * PARA ONDE E COM QUE FORÇA a água corre neste ponto do mundo.
	 *
	 * Devolve o rumo já normalizado e a força em partes por mil; vetor nulo em
	 * água parada e em terra.
	 *
	 * ## Por que aqui o rumo é um VETOR, e na grade são oito
	 *
	 * A fonte é a MESMA — a ordem da polilinha do curso, que já é o sentido do
	 * fluxo (invariante 12). O que muda é o consumidor: numa grade de casas só
	 * existem oito passos, e lá o rumo é quantizado; no mundo aberto a balsa
	 * anda em qualquer direção, e quantizar ali inventaria um degrau que a
	 * água não tem.
	 *
	 * Duas REPRESENTAÇÕES da mesma verdade não são duas verdades. O que seria
	 * duas verdades é deduzir o sentido de outra coisa — do raio, do declive —,
	 * e é isso que a invariante proíbe.
	 */
	BATTLESQUARE_API FVector2D FlowAt(const UIslandBakedPlan& Baked,
		const FVector2D& PositionUnits, int32& OutStrengthPerMille);
}
