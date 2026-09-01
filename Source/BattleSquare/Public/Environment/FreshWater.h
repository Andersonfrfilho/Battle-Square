// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Environment/IslandFeatureLayout.h"

/**
 * A água DOCE da ilha: os rios, os lagos e as quedas.
 *
 * O mar já existia — é o anel de `AWorldBoundaryWater` na borda. Faltava a
 * água que se bebe, e ela não é o mar em outro lugar: o mar é um limite (não
 * se entra nele, ele diz onde o mundo acaba), enquanto o rio é um caminho
 * (atravessa a ilha, se acompanha, se cruza).
 *
 * Um rio nasce numa MONTANHA e morre no mar, porque é essa a única versão que
 * não precisa ser explicada: água desce. A nascente não é inventada — sai do
 * mesmo `IslandFeatureLayout::Plan()` que planta os montes, e por isso um monte
 * que mudar de lugar leva o rio junto em vez de deixá-lo brotando do nada.
 *
 * O LAGO não é outra coisa: é o rio ficando largo. Fossem duas entidades,
 * bastaria alguém editar uma para o lago descolar do rio e virar uma poça no
 * meio do mato — e essa é exatamente a família de defeito que já custou L-032 e
 * L-033 aqui. Uma função de largura, um curso só.
 *
 * Tudo aqui é função pura sobre números: o curso não é ator, não é sorteio de
 * partida e não guarda estado. Quem desenha (o `AForestBackdrop` de cada
 * pedaço) pergunta "por onde passa o rio aqui dentro" e recebe a resposta.
 */
namespace FreshWater
{
	/**
	 * Um curso d'água inteiro, da nascente à foz.
	 *
	 * Ele é descrito em coordenadas POLARES — raio a partir do centro da ilha,
	 * e um rumo que varia com o raio — porque descer a montanha é literalmente
	 * afastar-se do centro. Em coordenadas cartesianas a mesma curva precisaria
	 * de um ponto de controle a cada dobra, e cada ponto seria uma chance de o
	 * rio subir de volta o morro.
	 */
	struct BATTLESQUARE_API FRiverCourse
	{
		/** Raio da nascente, na saia do monte. */
		float SourceRadiusUnits = 0.0f;

		/** Raio da foz, na linha da costa. */
		float MouthRadiusUnits = 0.0f;

		/** O rumo da nascente, em radianos, medido do centro da ilha. */
		float BearingRadians = 0.0f;

		/** Quanto o rumo abre para cada lado ao longo do percurso. */
		float MeanderRadians = 0.0f;

		/** Quantas curvas completas ele faz da nascente à foz. */
		float MeanderTurns = 0.0f;

		/** Em que raio ele alaga e vira lago. */
		float LakeRadiusUnits = 0.0f;

		/** Em que raio ele despenca. */
		float FallRadiusUnits = 0.0f;

		/**
		 * A ORDEM do curso, no sentido de Strahler.
		 *
		 * 1 é galho de cabeceira; 2 é o tronco em que dois galhos entram. A
		 * largura sai daqui, e é o que faz a água ENGROSSAR rio abaixo em vez
		 * de ter a mesma calha do nascedouro até o mar.
		 */
		int32 Order = 1;

		/**
		 * Onde este curso ENTRA em outro. Zero quer dizer que ele vai ao mar.
		 *
		 * É o que transforma seis linhas paralelas numa RAIZ. Rio de verdade é
		 * dendrítico: galhos finos convergindo num tronco. Sem isto a ilha
		 * tinha um pente, não uma bacia — e a água corria lado a lado sem
		 * nunca se encontrar.
		 */
		float JoinRadiusUnits = 0.0f;

		/** O rumo do tronco em que ele entra, para onde a curva converge. */
		float JoinBearingRadians = 0.0f;

		bool FlowsToTheSea() const { return JoinRadiusUnits <= 0.0f; }
	};

	/** Largura de meia calha do rio comum. */
	BATTLESQUARE_API float RiverHalfWidthUnits();

	/** Largura de meia calha no meio do lago. */
	BATTLESQUARE_API float LakeHalfWidthUnits();

	/** Até onde, para cada lado do raio da queda, a água ainda está caindo. */
	BATTLESQUARE_API float FallHalfLengthUnits();

	/** A que distância da margem corre a trilha que acompanha o rio. */
	BATTLESQUARE_API float TrailOffsetUnits();

	/** Largura de meia trilha. */
	BATTLESQUARE_API float TrailHalfWidthUnits();

	/** Todos os rios da ilha, um por montanha. */
	BATTLESQUARE_API TArray<FRiverCourse> Plan();

	/**
	 * Quantos rios desce cada monte.
	 *
	 * Existe para o teste perguntar em vez de transcrever. Um monte derrama
	 * para os dois flancos, e um rio por monte deixava a ilha com três fios
	 * de água que nenhuma trilha jamais cruzava.
	 */
	/**
	 * Só os TRONCOS: os cursos que chegam ao mar.
	 *
	 * Existe porque a bacia virou uma árvore, e quase toda pergunta antiga —
	 * "onde é o lago", "onde é a queda", "o rio desce até o mar" — é sobre o
	 * tronco. Galho de cabeceira morre na junção e não tem nada disso.
	 *
	 * Sem esta função, cada pergunta dessas teria de filtrar por conta própria,
	 * e a primeira que esquecesse afirmaria uma coisa falsa em silêncio.
	 */
	BATTLESQUARE_API TArray<FRiverCourse> PlanTrunks();

	/**
	 * CORREDEIRA: o trecho em que o leito é íngreme demais para correr manso.
	 *
	 * Não é uma faixa escolhida a dedo — ela é DERIVADA do relevo. Onde o chão
	 * desce depressa, a água acelera e quebra; onde ele achata, ela alarga e
	 * acalma. Perguntar ao terreno é o que faz a corredeira aparecer no lugar
	 * certo mesmo quando alguém mexer no relevo.
	 *
	 * Isso também dá função ao trecho entre o lago e a queda, que hoje é liso:
	 * água que sai de um lago e cai vinte metros adiante não vai mansa até a
	 * borda.
	 */
	BATTLESQUARE_API bool IsRapidsAt(const FRiverCourse& Course, float RadiusUnits);

	/** O declive do leito num ponto, em altura por unidade andada. */
	BATTLESQUARE_API float BedGradientAt(const FRiverCourse& Course, float RadiusUnits);

	/** A partir de que declive o trecho é corredeira. */
	BATTLESQUARE_API float RapidsGradient();

	/**
	 * O POÇO da cachoeira: fundo e estreito, nunca uma bacia.
	 *
	 * A morfologia diz duas coisas que decidem o desenho: a profundidade é
	 * proporcional à ALTURA DA QUEDA, e o poço aprofunda cerca de dez vezes
	 * mais rápido do que alarga. Poço de cachoeira é FURO — modelá-lo como um
	 * lago redondo seria desenhar a coisa errada.
	 */
	BATTLESQUARE_API float PlungePoolHalfWidthUnits(const FRiverCourse& Course);
	BATTLESQUARE_API float PlungePoolDepthUnits(const FRiverCourse& Course);

	BATTLESQUARE_API int32 RiversPerMountain();

	/**
	 * Uma FONTE: onde a água brota do chão, longe de qualquer monte.
	 *
	 * Ela existe porque a ilha só tinha água que descia de montanha, e isso
	 * deixava o miolo inteiro seco. Fonte é a água do lugar plano — e é o que
	 * permite haver água doce perto de quem mora.
	 */
	struct BATTLESQUARE_API FSpring
	{
		FVector2D CenterUnits = FVector2D::ZeroVector;
		float PoolHalfWidthUnits = 0.0f;
	};

	BATTLESQUARE_API TArray<FSpring> PlanSprings();

	/**
	 * Um CÓRREGO: o fio de água que liga uma fonte a um rio, ou um rio a outro.
	 *
	 * Sem eles a ilha tem seis rios paralelos e nada entre eles — água que
	 * corre lado a lado sem nunca se encontrar não é bacia, é listras.
	 *
	 * O córrego é estreito de propósito: ele se atravessa a pé, e por isso não
	 * corta a ilha. Quem precisa de ponte é o rio.
	 */
	struct BATTLESQUARE_API FBrook
	{
		TArray<FVector2D> PointsUnits;
		float HalfWidthUnits = 0.0f;
	};

	BATTLESQUARE_API TArray<FBrook> PlanBrooks();

	/**
	 * O que passa por um trecho de água.
	 *
	 * A largura decide, e é a mesma largura que já desenha o rio — não há uma
	 * segunda tabela dizendo onde o barco cabe. Duas tabelas concordariam até
	 * a primeira vez que alguém alargasse um rio (L-032).
	 */
	enum class ENavigability : uint8
	{
		/** Nem canoa: aqui se anda. */
		APe,

		/** Só barco pequeno — o córrego e o rio na cabeceira. */
		BarcoPequeno,

		/** Barco grande: o rio maduro e o lago. */
		BarcoGrande
	};

	BATTLESQUARE_API ENavigability NavigabilityForHalfWidth(float HalfWidthUnits);

	/**
	 * Uma passagem SUBTERRÂNEA entre duas águas.
	 *
	 * Existe porque nem toda ligação cabe na superfície: dois rios podem ter
	 * um espigão entre eles, e cavar um córrego por cima seria a água subindo
	 * o morro. Por baixo, ela não sobe — atravessa.
	 *
	 * E dá função a uma coisa que já existia sem ter: as grutas de cachoeira
	 * eram cenário bonito com água dentro. Agora elas são as BOCAS da
	 * passagem, e entrar numa delas leva a algum lugar.
	 */
	struct BATTLESQUARE_API FUnderwaterLink
	{
		FVector2D FromUnits = FVector2D::ZeroVector;
		FVector2D ToUnits = FVector2D::ZeroVector;

		/** Passagem de pedra é apertada: barco grande não entra. */
		ENavigability Navigability = ENavigability::BarcoPequeno;
	};

	BATTLESQUARE_API TArray<FUnderwaterLink> PlanUnderwaterLinks();

	/**
	 * Toda a água doce da ilha está ligada — por cima ou por baixo.
	 *
	 * É a pergunta que o pedido "dá para ir de barco a todo lugar" vira em
	 * código: um grafo, e ele é conexo ou não é. Sem esta função, a resposta
	 * seria olhar o mapa e achar que sim.
	 */
	BATTLESQUARE_API bool IsWaterNetworkConnected();

	/** Onde, no plano do mundo, o rio cruza este raio. */
	BATTLESQUARE_API FVector2D PointAt(const FRiverCourse& Course, float RadiusUnits);

	/**
	 * Quão largo o rio está neste raio.
	 *
	 * É AQUI que o lago existe: perto de `LakeRadiusUnits` a função sobe até
	 * `LakeHalfWidthUnits()` e volta, com transição suave. Lago é largura.
	 */
	BATTLESQUARE_API float HalfWidthAt(const FRiverCourse& Course, float RadiusUnits);

	/** Se neste raio a água está despencando. */
	BATTLESQUARE_API bool IsFallAt(const FRiverCourse& Course, float RadiusUnits);

	/**
	 * As grutas das cachoeiras — uma ao lado de cada queda.
	 *
	 * Ela sai como `FFeaturePlacement` de caverna, e não como um tipo novo,
	 * porque é UMA CAVERNA: quem planta já sabe plantar caverna, e `Overlaps` e
	 * `FitsOnLand` já sabem conferi-la. Um tipo próprio obrigaria a escrever de
	 * novo o mesmo despacho e as mesmas conferências, e a segunda cópia
	 * concorda com a primeira até alguém mexer numa delas (L-032).
	 *
	 * O caminho é de mão única: a gruta LÊ o plano da ilha, e o plano da ilha
	 * nunca lê a água. Acrescentar a gruta ao `IslandFeatureLayout::Plan()`
	 * fecharia o ciclo, já que é dele que os rios nascem.
	 *
	 * Ela fica ao LADO da queda: a água caindo ocupa o lugar imediatamente
	 * abaixo, e uma gruta ali seria uma boca com o rio entrando por dentro. Do
	 * lado, a queda fica à vista de quem está na boca — que é a única razão de a
	 * gruta estar ali e não em qualquer outro lugar.
	 *
	 * O lugar é PROCURADO, não calculado. Tentou-se afastar da margem por uma
	 * distância fixa, e não existe distância fixa que sirva: perto do degrau o
	 * rio vem torto de tanto serpentear, a perpendicular à corrente ganha
	 * componente radial e joga a gruta rio acima, dentro do lago. A busca anda
	 * ao redor da queda, do mais perto para o mais longe, e para no primeiro
	 * lugar que sobra da água, da terra, dos campos de treino e das outras
	 * peças da ilha — com folga, porque um lugar que apenas cabe sai de lugar
	 * na primeira mexida na serpentina do rio.
	 *
	 * Onde não houver lugar, não há gruta. A lista pode vir com menos grutas do
	 * que quedas, e vir vazia é resposta válida: plantar onde não cabe devolve
	 * exatamente a quina dentro da água que a busca existe para evitar.
	 */
	BATTLESQUARE_API TArray<IslandFeatureLayout::FFeaturePlacement> PlanGrottoes();
}
