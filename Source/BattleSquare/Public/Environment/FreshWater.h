// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/IslandGeography.h"

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

		/**
		 * Onde ele alaga e onde ele despenca, em PROGRESSO do curso (0 a 1).
		 *
		 * Era raio, e raio deixou de servir: a bacia virou uma árvore de
		 * verdade, e um galho que corre para o lado passa duas vezes pelo mesmo
		 * raio. A pergunta "que ponto do rio está no raio X" perdeu a resposta
		 * única — e o sintoma foi nenhum curso achar a própria foz.
		 *
		 * Progresso é o parâmetro natural de uma linha desenhada, e sobrevive a
		 * qualquer forma que a árvore tome.
		 */
		float LakeAtProgress = 0.0f;
		float FallAtProgress = 0.0f;

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

		/**
		 * O CURSO DESENHADO, ponto a ponto.
		 *
		 * Ele existe porque a parametrização por raio não consegue desenhar uma
		 * raiz. Com `ponto = f(raio)`, todo curso corre do centro para fora e
		 * os galhos só podem se abrir em ÂNGULO — o que desenha espinha de
		 * peixe, sempre, por mais galhos que se acrescente.
		 *
		 * A árvore agora é gerada crescendo da FOZ para dentro, ramificando; o
		 * curso guarda os pontos, e `PointAt` procura neles. O raio continua
		 * sendo o parâmetro de consulta porque meio mundo já pergunta assim, e
		 * ele funciona porque cada passo é obrigado a entrar — um curso que
		 * voltasse para fora teria dois pontos no mesmo raio.
		 */
		TArray<FVector2D> PointsUnits;

		bool FlowsToTheSea() const { return JoinRadiusUnits <= 0.0f; }

		/**
		 * Este curso tem cachoeira.
		 *
		 * Nem todo tem, e forçar uma em cada foz inventa acidente onde o
		 * terreno não tem. Quem pergunta por queda pergunta ISTO antes — foi
		 * ler o valor sem checar que pôs uma trilha mirando o mar aberto.
		 */
		bool HasFall() const { return FallAtProgress >= 0.0f; }
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
	BATTLESQUARE_API bool IsRapidsAtProgress(const FRiverCourse& Course, float Progress);

	/** O declive do leito num ponto, em altura por unidade andada. */
	BATTLESQUARE_API float BedGradientAtProgress(const FRiverCourse& Course, float Progress);

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
	/**
	 * Um PATAMAR da cachoeira — um degrau da escada de poços.
	 *
	 * A morfologia diz que cachoeira alta raramente é um degrau só: a parede
	 * recua cavando poços sucessivos em alturas diferentes, e o que sobra é
	 * uma escada. Modelá-la como um tombo único é desenhar a exceção.
	 *
	 * E é o que torna a queda ESCALÁVEL: um paredão de vinte metros não se
	 * sobe; uma escada de quatro degraus de cinco, sim.
	 */
	struct BATTLESQUARE_API FFallStep
	{
		FVector2D CenterUnits = FVector2D::ZeroVector;

		/** Altura do topo deste patamar sobre o pé da queda. */
		float HeightUnits = 0.0f;

		/** A lâmina d'água que cai neste degrau. */
		float HalfWidthUnits = 0.0f;
	};

	BATTLESQUARE_API TArray<FFallStep> PlanFallSteps(const FRiverCourse& Course);

	/**
	 * As PEDRAS da queda: os blocos que a água arrancou da parede.
	 *
	 * Elas não são enfeite. São o apoio — o que transforma "subir a cachoeira"
	 * de escalada em degrau improvisado, e o que explica de onde vieram os
	 * patamares: cada pedra no poço é um pedaço que faltou lá em cima.
	 */
	struct BATTLESQUARE_API FFallStone
	{
		FVector2D CenterUnits = FVector2D::ZeroVector;
		float RadiusUnits = 0.0f;

		/** Altura do topo dela, para o pé encontrar apoio. */
		float TopHeightUnits = 0.0f;

		/** Se ela serve de DEGRAU da subida, e não é só pedra no rio. */
		bool bIsStep = false;
	};

	BATTLESQUARE_API TArray<FFallStone> PlanFallStones(const FRiverCourse& Course);

	/**
	 * O caminho que SOBE a cachoeira, pela margem, em ziguezague.
	 *
	 * Mesma regra da trilha de serra: para vencer uma altura dentro do declive
	 * que uma pessoa aguenta, o caminho vai de lado. Aqui ele é mais apertado,
	 * porque a margem de uma queda é estreita — e é por isso que as pedras
	 * viram degrau: onde a perna não cabe, sobe-se um lance.
	 */
	BATTLESQUARE_API TArray<FVector2D> PlanFallClimb(const FRiverCourse& Course);

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
		/**
		 * O curso da passagem, ponto a ponto.
		 *
		 * Era um segmento reto entre duas grutas — três riscos no mapa, e a
		 * coisa mais fora de lugar nele. Rede de água subterrânea de calcário
		 * é DENDRÍTICA: ela dissolve a pedra seguindo fratura, e o desenho que
		 * sai é raiz. É aqui que esse padrão pertence de verdade.
		 */
		TArray<FVector2D> PointsUnits;

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

	/**
	 * QUANTA ÁGUA um bioma tem, como fração da área de terra.
	 *
	 * É o botão que faltava. A densidade da bacia era um número de atratores
	 * escolhido por tentativa — e "quantos atratores" não quer dizer nada para
	 * quem desenha um mundo. "Seis por cento da terra é água doce" quer.
	 *
	 * Tudo o mais deriva daqui: quantos cursos, quantos córregos, quantas
	 * galerias. Trocar de bioma passa a ser trocar UM número, e é o que permite
	 * reusar o mesmo gerador no pântano e no deserto.
	 */
	BATTLESQUARE_API float WaterCoverageForBiome(EIslandBiome Biome);

	/** Quanto da terra ESTÁ molhado de fato, medido na máscara. */
	BATTLESQUARE_API float MeasuredWaterCoverage();

	/** Onde, no plano do mundo, o rio cruza este raio. */
	BATTLESQUARE_API FVector2D PointAtProgress(const FRiverCourse& Course, float Progress);

	/** O comprimento do curso, para quem precisa converter distância em progresso. */
	BATTLESQUARE_API float CourseLengthUnits(const FRiverCourse& Course);

	/**
	 * Quão largo o rio está neste raio.
	 *
	 * É AQUI que o lago existe: perto de `LakeRadiusUnits` a função sobe até
	 * `LakeHalfWidthUnits()` e volta, com transição suave. Lago é largura.
	 */
	BATTLESQUARE_API float HalfWidthAtProgress(const FRiverCourse& Course, float Progress);

	/** Se neste raio a água está despencando. */
	BATTLESQUARE_API bool IsFallAtProgress(const FRiverCourse& Course, float Progress);

	/**
	 * O ponto do curso mais perto de uma posição, e a que distância ele está.
	 *
	 * **É esta a pergunta que o mundo sempre quis fazer.** Quem planta uma
	 * árvore, quem traça uma trilha e quem decide se um ponto está molhado
	 * perguntam "estou perto da água?", e estavam sendo obrigados a traduzir
	 * isso para "que raio é esse?" — uma tradução que deixou de valer no dia em
	 * que a água passou a correr para os lados.
	 */
	BATTLESQUARE_API float NearestOn(const FRiverCourse& Course, const FVector2D& PositionUnits,
		float& OutProgress);

	/**
	 * A que distância da MARGEM de qualquer água doce este ponto está.
	 *
	 * Negativo quer dizer dentro da água. Percorre todos os cursos e todos os
	 * córregos: é a única pergunta que basta para plantar, traçar e molhar.
	 */
	BATTLESQUARE_API float DistanceToFreshWater(const FVector2D& PositionUnits);

	/**
	 * Este ponto está molhado — respondido por uma GRADE, em tempo constante.
	 *
	 * A pergunta é feita milhões de vezes: uma por aresta do traçado de
	 * trilhas, uma por árvore plantada. Respondê-la percorrendo os cursos custa
	 * o mundo inteiro a cada vez, e foi o que fez a bateria estourar o tempo.
	 *
	 * O mapa é FIXO, então a água é desenhada uma vez numa grade e depois só se
	 * consulta. É a mesma troca que a tabela de alturas do traçado fez, e pelo
	 * mesmo motivo.
	 */
	BATTLESQUARE_API bool IsFreshWaterAt(const FVector2D& PositionUnits);

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
