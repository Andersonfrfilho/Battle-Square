// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * O que cada prédio da vila É.
 *
 * A lista é CURTA e cada entrada serve a um laço que o jogo já tem — a regra
 * que ordena a spec das cidades. Prédio que não serve a nada é cenário com
 * porta, e este projeto já pagou caro por recursos completos que ninguém
 * alcançava.
 */
UENUM()
enum class EVillageBuilding : uint8
{
	/**
	 * Centro de Recuperação: cura o pet DE GRAÇA, e abriga a coleção.
	 *
	 * Curar e trocar o pet ativo são a mesma visita; duas portas para um gesto
	 * só é atrito sem ganho.
	 */
	CentroDeRecuperacao,

	/** Escola do treinador: tira `bs.Especializar` do console. */
	Escola,

	/** Arena da vila: um oponente com motivo para voltar. */
	Arena,

	/** Praça com o poste de anúncios — diz onde ficam as coisas do mapa. */
	Praca,

	/** Marco de retorno: viagem rápida e marca no mapa. */
	Marco,

	/**
	 * Casa. Não tem função, e ganha o lugar por uma razão só: a vila precisa
	 * se LER de fora — quem chega pela mata vê telhado antes de porta.
	 *
	 * E a regra que vem junto: casa sem função não tem porta. Porta que não
	 * abre é promessa quebrada.
	 */
	Casa,

	/**
	 * Academia: treino PAGO e rápido de alguns atributos.
	 *
	 * Vai para o FIM da lista pelo motivo de sempre — a ordem daqui já foi
	 * lida por quem salva e por quem desenha.
	 *
	 * E ela não existe na vila inicial. Essa ausência é o desenho: é ela que
	 * dá motivo à primeira viagem.
	 */
	Academia,

	/** Mercado: troca de pets por raridade, e o quadro de trabalhos. */
	Mercado,

	/** O portão do posto de fronteira. Só abre para quem venceu o ranking. */
	Portao,

	/**
	 * PALAFITA: casa sobre a água, em estacas.
	 *
	 * O Mercado do Lago não fica ao LADO do lago — metade dele fica em cima.
	 * Comércio anda por água, e a cidade que vive disso põe a porta na margem
	 * do canal, não na rua.
	 *
	 * Ela não pede chão plano, e é a única construção deste mundo que não pede:
	 * a estaca resolve o desnível, e é para isso que ela existe.
	 */
	Palafita,

	/**
	 * PASSARELA: a rua da cidade sobre a água.
	 *
	 * Sem ela as palafitas são casas ilhadas, e uma cidade em que não se anda
	 * de uma casa à outra não é cidade. Aqui a ponte não é exceção — é o
	 * quarteirão.
	 */
	Passarela,

	/**
	 * CHINAMPA: a lavoura flutuante.
	 *
	 * A referência é asteca, e ela é melhor que a veneziana para este mundo:
	 * Veneza pôs PALÁCIO sobre a água, os astecas puseram ROÇA. Uma cidade que
	 * planta no lago é uma cidade que come do lago — e isso liga a coisa nova à
	 * fazenda que a vila já tem, em vez de acrescentar um cenário bonito ao
	 * lado dela.
	 *
	 * É larga e rasa, como canteiro: chinampa é chão feito de limo e estaca,
	 * não casa. Vista de cima, o que se lê é horta em faixas com água entre
	 * elas.
	 */
	Chinampa
};

struct BATTLESQUARE_API FVillagePlacement
{
	EVillageBuilding Building = EVillageBuilding::Casa;

	/** Centro do prédio, relativo ao centro da vila. */
	FVector2D OffsetUnits = FVector2D::ZeroVector;

	/** Meio-lado da pegada quadrada. */
	FVector2D HalfExtentUnits = FVector2D(200.0f, 200.0f);

	/** Altura, para o telhado se ler de longe. */
	float HeightUnits = 400.0f;

	/**
	 * Fica SOBRE A ÁGUA, e por isso não recebe o chão achatado.
	 *
	 * Todo o resto deste mundo assume terreno plano embaixo. Esta é a exceção,
	 * e ela precisa ser declarada — senão o achatamento do lote seca o lago
	 * para caber a casa, que é o avesso da ideia.
	 */
	bool bOverWater = false;
};

/**
 * O traçado da vila — PURO, e é o ponto.
 *
 * Quem decide onde cada prédio fica não precisa de `UWorld`, de malha nem de
 * Editor: é aritmética sobre um lote. Assim "nenhum prédio invade o outro" e
 * "tudo cabe no lote" se verificam headless, que é onde este projeto consegue
 * verificar de verdade.
 *
 * Mesma separação que tornou `FArenaFromWorld` e `ScenaryClimate` testáveis.
 */
namespace VillageLayout
{
	/**
	 * O lote da vila ocupa uma FRAÇÃO do bloco.
	 *
	 * O bloco tem 6400 unidades (64 m). Vila que enche o bloco não deixa lugar
	 * para a floresta que vem em volta, e o jogador sairia de casa direto para
	 * o bloco vizinho.
	 */
	BATTLESQUARE_API float PlotHalfExtentUnits();

	/**
	 * A praça fica no MEIO, e os prédios em volta dela.
	 *
	 * Não é gosto: a praça é o poste de anúncios, e ele precisa ser a primeira
	 * coisa que se encontra ao entrar. Um poste num canto é um poste que
	 * ninguém lê.
	 */
	BATTLESQUARE_API TArray<FVillagePlacement> Plan();

	/**
	 * O traçado de UM tipo de assentamento.
	 *
	 * Cada tipo tem prédios próprios, e é isso que faz a viagem valer. Um
	 * traçado só, repetido quatro vezes, seria a mesma vila quatro vezes — e
	 * a spec chama isso de imposto de caminhada.
	 */
	BATTLESQUARE_API TArray<FVillagePlacement> PlanFor(ESettlementKind Kind);

	/** O lote de um tipo. A cidade grande é MAIOR; o posto é quase só portão. */
	BATTLESQUARE_API float PlotHalfExtentUnitsFor(ESettlementKind Kind);

	/** A pegada cabe no lote DAQUELE tipo. */
	BATTLESQUARE_API bool FitsInPlotFor(ESettlementKind Kind, const FVillagePlacement& Placement);

	/** A clareira de um tipo: o lote dele mais a folga. */
	BATTLESQUARE_API float ClearingHalfExtentUnitsFor(ESettlementKind Kind);

	/** Os dois prédios que não podem faltar, porque tiram regra do console. */
	BATTLESQUARE_API bool HasBuilding(const TArray<FVillagePlacement>& Placements,
		EVillageBuilding Building);

	/** Duas pegadas se invadem. */
	BATTLESQUARE_API bool Overlaps(const FVillagePlacement& First, const FVillagePlacement& Second);

	/** A pegada inteira cabe no lote. */
	BATTLESQUARE_API bool FitsInPlot(const FVillagePlacement& Placement);

	/**
	 * A CLAREIRA da vila: o lote mais uma folga.
	 *
	 * Maior que o lote de propósito. Com a folga exata, a mata encosta na
	 * parede — e uma árvore colada no Centro de Recuperação faz a vila parecer
	 * engolida pelo mato em vez de aberta nele.
	 */
	BATTLESQUARE_API float ClearingHalfExtentUnits();

	/**
	 * Aqui não se planta.
	 *
	 * O gerador da mata pergunta isto antes de pôr cada instância. Sem a
	 * pergunta, nasce árvore dentro da praça — e "remover a árvore da praça"
	 * viraria caso especial que nunca acaba.
	 *
	 * A conta é em coordenada de MUNDO porque a vila fica na origem e a mata é
	 * plantada por pedaço: cada pedaço só sabe onde ELE está.
	 */
	BATTLESQUARE_API bool BlocksPlanting(const FVector2D& WorldXY);
}
