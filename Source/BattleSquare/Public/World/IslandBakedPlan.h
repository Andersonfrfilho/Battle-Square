#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Battle/FluidRegistry.h"
#include "World/LandUseLayout.h"
#include "IslandBakedPlan.generated.h"

/**
 * O TRAÇADO ASSADO — o contrato entre o gerador e o mundo.
 *
 * O traçado não depende de jogador, de relógio nem de semente de sessão, então
 * ele se calcula FORA do jogo, uma vez, e o resultado vem embarcado como dado.
 * Montar o mundo inteiro em runtime leva 136 segundos medidos, e o traçado usa
 * `float` — determinismo em ponto flutuante entre plataformas não é garantido,
 * e gerar em runtime poderia produzir ilhas sutilmente diferentes no Mac e no
 * Windows. O assado elimina a pergunta em vez de apostar nela.
 *
 * O argumento decisivo, porém, é a FONTE ÚNICA: carta e mundo lendo o mesmo
 * arquivo não podem divergir em silêncio (L-032 na escala do mundo).
 *
 * ## Por que as estruturas aqui são ESPELHOS, e não os structs do traçado
 *
 * Três motivos, e cada um sozinho já bastaria:
 *
 * 1. **O traçado não muda** (invariante 1 da feature). Pôr `USTRUCT` e
 *    `UPROPERTY` nos structs do gerador seria editá-los para servir a quem os
 *    consome — exatamente a mistura que tornou a depuração cara em agosto.
 * 2. **Metade deles não PODE ganhar reflexão.** `ECrossingKind` e as estruturas
 *    de `FreshWater` vivem dentro de `namespace`, e `UENUM`/`USTRUCT` não
 *    existem lá dentro.
 * 3. **O gerador guarda PARÂMETROS; o mundo precisa do RESULTADO.** O curso do
 *    rio guarda `MeanderTurns` e `JoinBearingRadians`, que são como ele foi
 *    desenhado. Quem constrói a malha quer o ponto e a largura ALI. Assar o
 *    parâmetro obrigaria o mundo a reexecutar o gerador para ler o desenho —
 *    que é justamente o que o assado existe para não fazer.
 *
 * O espelho não é uma segunda fonte de verdade: ele é o retrato da única que
 * há, e o teste de T2 confere campo a campo que o retrato bate com o original.
 */

/**
 * OS PARÂMETROS QUE GERARAM O ASSADO.
 *
 * O modo de falhar desta feature é silencioso: alguém muda o `WorldBudget`, o
 * raio da ilha ou a forma da costa, e o assado continua respondendo — o mundo
 * passa a ser de uma configuração que não existe mais, e nada quebra.
 *
 * Por isso os parâmetros viajam DENTRO do assado, e não só o resumo deles. O
 * resumo diz que algo mudou; só os valores dizem O QUÊ, e "reasse porque algo
 * mudou" manda a pessoa procurar sozinha o que ela já poderia ter lido.
 *
 * Campo novo aqui entra automaticamente no resumo e na mensagem de divergência:
 * as duas coisas percorrem a reflexão desta struct, e não uma lista à parte que
 * alguém teria de lembrar de atualizar.
 */
USTRUCT()
struct BATTLESQUARE_API FIslandParameters
{
	GENERATED_BODY()

	UPROPERTY()
	float LandRadiusUnits = 0.0f;

	UPROPERTY()
	uint8 CoastShape = 0;

	UPROPERTY()
	uint8 Biome = 0;

	/** A semente do cenário do mundo — é ela que cava cada caverna. */
	UPROPERTY()
	int32 ScenerySeed = 0;

	UPROPERTY()
	float WaterCoverage = 0.0f;

	UPROPERTY()
	int32 GroveCount = 0;

	UPROPERTY()
	int32 HiddenClearingCount = 0;

	UPROPERTY()
	int32 BreederCount = 0;

	UPROPERTY()
	int32 FarmsPerSettlement = 0;

	UPROPERTY()
	int32 TendedOrchardCount = 0;

	UPROPERTY()
	int32 WildOrchardCount = 0;

	UPROPERTY()
	int32 RoadsideShopCount = 0;

	UPROPERTY()
	int32 CampCount = 0;

	UPROPERTY()
	int32 RuinCount = 0;

	UPROPERTY()
	float StraightGalleryShare = 0.0f;

	UPROPERTY()
	int32 GraveyardsPerSettlement = 0;

	UPROPERTY()
	int32 ForgottenGraveyardCount = 0;

	UPROPERTY()
	float ForestDensity = 0.0f;

	/**
	 * As fronteiras das FAIXAS DE TERRENO.
	 *
	 * Estão aqui, e não lidas de `IslandGeography` na hora de construir, por
	 * duas razões que puxam para o mesmo lado: elas são parâmetros da ilha, e
	 * portanto mudá-las tem de envelhecer o assado; e com elas dentro, a faixa
	 * de cada ponto é derivável do assado SOZINHO — uma fonte de verdade, em
	 * vez de o mundo perguntar ao gerador o que o assado já deveria saber.
	 */
	UPROPERTY()
	float BeachWidthUnits = 0.0f;

	UPROPERTY()
	float BluffInnerRadiusUnits = 0.0f;

	UPROPERTY()
	float BluffOuterRadiusUnits = 0.0f;

	UPROPERTY()
	FVector2D VolcanoCenterUnits = FVector2D::ZeroVector;

	UPROPERTY()
	float VolcanoScorchedRadiusUnits = 0.0f;
};

/**
 * AS FAIXAS DE TERRENO — o que a cor conta ao jogador.
 *
 * A ordem importa: a faixa de um ponto é a PRIMEIRA que o reivindica, e isso
 * é decisão, não acaso. A rocha queimada vem antes de tudo porque o vulcão
 * queima o que já estava ali; o cume vem antes do barranco porque o alto de
 * uma escarpa ainda é alto.
 */
UENUM()
enum class ETerrainBand : uint8
{
	Praia,
	Mata,
	Barranco,
	RochaQueimada,
	Cume,
	Count UMETA(Hidden)
};

/**
 * Uma polilinha.
 *
 * Existe porque `TArray<TArray<FVector2D>>` não é serializável pela reflexão da
 * engine — e um traçado é feito quase todo de linhas.
 */
USTRUCT()
struct BATTLESQUARE_API FBakedPolyline
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> PointsUnits;
};

/** Um curso d'água, já DESENHADO: ponto, largura ali, e o que há naquele ponto. */
USTRUCT()
struct BATTLESQUARE_API FBakedRiver
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> PointsUnits;

	/**
	 * A meia-largura em CADA ponto, na mesma ordem.
	 *
	 * Vai ponto a ponto, e não como parâmetro, porque a largura é função do
	 * progresso ao longo do curso — reconstruí-la no mundo exigiria refazer a
	 * conta do gerador, e duas contas concordam até a primeira edição.
	 */
	UPROPERTY()
	TArray<float> HalfWidthUnits;

	/**
	 * A FUNDURA em cada ponto, na mesma ordem dos pontos.
	 *
	 * Assada, e não estimada por quem lê. `TrailLayout` calculava fundura pela
	 * largura (`largura × 0,065`) e o comentário dela defendia isso dizendo
	 * que a alternativa seria "uma segunda fonte da mesma verdade" — o que era
	 * certo enquanto não houvesse fundura de verdade. A partir daqui, é a
	 * estimativa que seria a segunda fonte.
	 *
	 * ⚠️ **Ela VARIA ao longo do curso, e é isso que a largura nunca soube.**
	 * O declive do leito decide: corredeira é rasa, remanso é fundo. Um trecho
	 * largo e íngreme é raso, e a estimativa por largura o dava como fundo.
	 *
	 * Vazia é assado ANTIGO, gravado antes desta feature — e o hash de
	 * parâmetros acusa a divergência nomeando o parâmetro.
	 */
	UPROPERTY()
	TArray<float> DepthUnits;

	/** Onde há corredeira, na mesma ordem dos pontos. */
	UPROPERTY()
	TArray<bool> bIsRapids;

	/**
	 * DE QUE FLUIDO a água é, PONTO A PONTO.
	 *
	 * Ponto a ponto, e não um fluido por curso, porque termal é propriedade da
	 * POSIÇÃO: um rio nasce fervendo na saia do vulcão e chega frio ao mar. Um
	 * fluido por curso decidiria isso pelo meio do percurso, e um rio que só
	 * passa perto do vulcão sairia frio — quebrando exatamente a promessa de
	 * "a água termal perto do vulcão é reconhecida como termal".
	 */
	UPROPERTY()
	TArray<uint8> FluidByPoint;

	/**
	 * PARA ONDE A ÁGUA CORRE em cada ponto, como `EBattleDirection`.
	 *
	 * Lido da ORDEM da polilinha, que já é o sentido do fluxo — da nascente
	 * para a foz. Deduzi-lo de novo a partir do raio, do declive ou da
	 * geometria seria uma segunda verdade, e ela concordaria com esta até a
	 * primeira edição, com o rio correndo para trás num trecho que ninguém
	 * olhou.
	 *
	 * Oito rumos, e a escolha foi MEDIDA: encaixar os 5.480 trechos da bacia
	 * em oito direções troca de rumo em 2% dos passos e vai-e-volta em 0,3%.
	 * O rio não treme. E o consumidor é 8-direcional de qualquer forma —
	 * não há como empurrar alguém a 12 graus numa grade de casas.
	 */
	UPROPERTY()
	TArray<uint8> FlowDirectionByPoint;

	/**
	 * QUANTO ela corre, em partes por mil do declive do leito.
	 *
	 * Sai de `BedGradientAtProgress`, que é o que faz a água correr depressa —
	 * medido: mediana 0,056 e máximo 0,329, ou 56 e 329 por mil. Inteiro,
	 * porque o núcleo não tem float, e por MIL pela mesma razão das
	 * densidades: por cento perderia a diferença entre um fio manso e um
	 * quase parado.
	 */
	UPROPERTY()
	TArray<uint8> FlowStrengthByPoint;

	/** A ordem de Strahler: 1 é cabeceira, e é daqui que a água engrossa. */
	UPROPERTY()
	int32 Order = 1;

	UPROPERTY()
	bool bFlowsToTheSea = false;

	/**
	 * Lago e queda, quando EXISTEM.
	 *
	 * O booleano vem junto de propósito: marcar ausência com valor fora de
	 * faixa vaza, e já pôs uma vila fora da ilha neste projeto. Quem lê a
	 * posição pergunta o booleano antes.
	 */
	UPROPERTY()
	bool bHasLake = false;

	UPROPERTY()
	FVector2D LakeCenterUnits = FVector2D::ZeroVector;

	UPROPERTY()
	bool bHasFall = false;

	UPROPERTY()
	FVector2D FallCenterUnits = FVector2D::ZeroVector;

	UPROPERTY()
	float PlungePoolHalfWidthUnits = 0.0f;

	UPROPERTY()
	float PlungePoolDepthUnits = 0.0f;
};

/**
 * Um CÓRREGO: o fio de água que liga uma fonte a um rio, ou um rio a outro.
 *
 * Tem largura PRÓPRIA, e não a do rio. Ele se atravessa a pé — é isso que o
 * separa do rio, que precisa de ponte —, e assar só a linha faria o mundo
 * desenhá-lo com a calha de um rio, apagando a diferença que lhe dá sentido.
 */
USTRUCT()
struct BATTLESQUARE_API FBakedBrook
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> PointsUnits;

	UPROPERTY()
	float HalfWidthUnits = 0.0f;

	/** De que fluido este córrego é. Ele é curto: um só basta. */
	UPROPERTY()
	uint8 Fluid = 0;
};

/**
 * Um AQUEDUTO: a obra que leva água até a vila que não tem nenhuma perto.
 *
 * Guarda a QUEDA junto da linha. Sem ela o aqueduto vira um cano mágico, e some
 * justamente o que o torna interessante — a obra tem de descer, e descer é o
 * que faz a água andar.
 */
USTRUCT()
struct BATTLESQUARE_API FBakedAqueduct
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> PointsUnits;

	UPROPERTY()
	float DropUnits = 0.0f;
};

/** Uma fonte: o olho d'água e o poço que ele forma. */
USTRUCT()
struct BATTLESQUARE_API FBakedSpring
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2D CenterUnits = FVector2D::ZeroVector;

	UPROPERTY()
	float PoolHalfWidthUnits = 0.0f;

	/** De que fluido esta fonte é — uma fonte termal não é uma fonte comum. */
	UPROPERTY()
	uint8 Fluid = 0;
};

/** Uma trilha, com a ALTURA do chão em cada ponto. */
USTRUCT()
struct BATTLESQUARE_API FBakedTrail
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> PointsUnits;

	/**
	 * A altura do terreno em cada ponto, na mesma ordem.
	 *
	 * Vai junto porque reamostrar a malha de alturas daria o declive da MALHA,
	 * não o da trilha: a malha anda 1.555 unidades por casa e o barranco tem
	 * 2.520 de largura. Medir na grade errada já fez "não existe corredeira"
	 * parecer defeito de traçado.
	 */
	UPROPERTY()
	TArray<float> GroundHeightUnits;

	UPROPERTY()
	bool bFellBackToStraightLine = false;
};

/** Onde uma trilha encontra água, e o que se faz ali. */
USTRUCT()
struct BATTLESQUARE_API FBakedCrossing
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2D CenterUnits = FVector2D::ZeroVector;

	/**
	 * O tipo, como número.
	 *
	 * `ECrossingKind` vive dentro de `namespace TrailLayout`, onde `UENUM` não
	 * existe. O valor é o do enum, e `KindOf()` devolve ele tipado — a conversão
	 * mora num lugar só.
	 */
	UPROPERTY()
	uint8 Kind = 0;

	/**
	 * DE QUE a ponte é feita, como `EBridgeMaterial`.
	 *
	 * Viaja como `uint8` pelo mesmo motivo do tipo: o enum vive dentro de um
	 * `namespace`, onde `UENUM` não existe.
	 *
	 * Zero é `Nenhum`, que é o que toda travessia que não é ponte carrega — e
	 * é também o que um assado ANTIGO traz, gravado antes desta feature. As
	 * duas coisas coincidem de propósito: uma travessia sem material declarado
	 * não é uma ponte quebrada, é uma travessia que não é ponte.
	 */
	UPROPERTY()
	uint8 BridgeMaterial = 0;

	/**
	 * Dá para passar por aqui?
	 *
	 * Falso SÓ na ponte destruída. É a única travessia que existe e não serve
	 * — e é por isso que a pergunta precisa existir: quem lê `Kind == Ponte`
	 * concluiria que há passagem, e concluiria errado.
	 */
	bool CanBeCrossed() const
	{
		// O VALOR, e não o tipo. `EBridgeMaterial` vive dentro de
		// `namespace TrailLayout`, e este cabeçalho não o inclui — pelo mesmo
		// motivo que `Kind` viaja como `uint8`: metade destas estruturas não
		// pode ganhar reflexão, e incluir o traçado aqui inverteria a
		// dependência (o assado é lido pelo mundo, não pelo gerador).
		//
		// O 3 é `EBridgeMaterial::Destruida`, e o `static_assert` no `.cpp`
		// reprova a compilação se alguém reordenar o enum.
		constexpr uint8 DestruidaComoNumero = 3;
		return BridgeMaterial != DestruidaComoNumero;
	}

	UPROPERTY()
	float DepthUnits = 0.0f;
};

/** Uma mancha de uso do solo: o motivo de andar até ali. */
USTRUCT()
struct BATTLESQUARE_API FBakedGroundUse
{
	GENERATED_BODY()

	UPROPERTY()
	EGroundUse Use = EGroundUse::Nenhum;

	UPROPERTY()
	FVector2D CenterUnits = FVector2D::ZeroVector;

	UPROPERTY()
	float HalfExtentUnits = 0.0f;

	UPROPERTY()
	bool bYieldsWater = false;

	UPROPERTY()
	EDeity Deity = EDeity::MaeNatureza;
};

/** A planta de uma caverna: a grade de paredes já cavada. */
USTRUCT()
struct BATTLESQUARE_API FBakedCave
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2D CenterUnits = FVector2D::ZeroVector;

	UPROPERTY()
	int32 Columns = 0;

	UPROPERTY()
	int32 Rows = 0;

	UPROPERTY()
	int32 EntranceColumn = 0;

	/** As paredes de cada casa, em máscara de bits, linha a linha. */
	UPROPERTY()
	TArray<uint8> Walls;
};

/**
 * O traçado inteiro da ilha, assado.
 *
 * Uma instância só, gravada por `Tools/bake_island.sh`. O jogo a carrega e
 * constrói o mundo a partir dela — nunca reexecutando o gerador.
 */
UCLASS()
class BATTLESQUARE_API UIslandBakedPlan : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * A impressão digital dos parâmetros que geraram este assado.
	 *
	 * O modo de falhar desta feature é silencioso: alguém muda o `WorldBudget`,
	 * o raio da ilha ou a forma da costa, e o assado continua respondendo — o
	 * mundo passa a ser de uma configuração que não existe mais, sem nada
	 * quebrar. Um aviso silencioso aqui seria pior que nenhum.
	 */
	UPROPERTY()
	uint32 ParameterHash = 0;

	/** Os valores por extenso, para a guarda poder NOMEAR o que divergiu. */
	UPROPERTY()
	FIslandParameters Parameters;

	/** O lado da grade de alturas, em casas. */
	UPROPERTY()
	int32 HeightGridSide = 0;

	/** O raio de terra da ilha — a grade cobre de `-Raio` a `+Raio` nos dois eixos. */
	UPROPERTY()
	float LandRadiusUnits = 0.0f;

	/**
	 * O raio de terra em cada GRAU de rumo — a linha da costa.
	 *
	 * A ilha deixou de ser um círculo, então um raio só não diz onde a terra
	 * acaba. Sem isto a faixa de praia sairia a uma distância fixa do centro, e
	 * numa reentrância ela cairia no mar.
	 */
	UPROPERTY()
	TArray<float> CoastRadiusByDegree;

	/** As alturas, linha a linha. Tem `HeightGridSide * HeightGridSide` casas. */
	UPROPERTY()
	TArray<float> GroundHeightUnits;

	UPROPERTY()
	TArray<FBakedRiver> Rivers;

	UPROPERTY()
	TArray<FBakedBrook> Brooks;

	UPROPERTY()
	TArray<FBakedSpring> Springs;

	UPROPERTY()
	TArray<FBakedTrail> Trails;

	UPROPERTY()
	TArray<FBakedCrossing> Crossings;

	UPROPERTY()
	TArray<FBakedGroundUse> GroundUses;

	UPROPERTY()
	TArray<FBakedCave> Caves;

	UPROPERTY()
	TArray<FBakedPolyline> UnderwaterLinks;

	UPROPERTY()
	TArray<FBakedAqueduct> Aqueducts;

private:
	/** Guardado: `BandAt` é perguntado uma vez por casa ao erguer o relevo. */
	mutable float CachedHighestWalkable = -1.0f;

public:
	/** A altura no índice de grade, sem o consumidor refazer a conta do índice. */
	float HeightAtCell(int32 Column, int32 Row) const;

	/**
	 * A altura do chão numa posição qualquer, interpolada entre as quatro
	 * casas em volta.
	 *
	 * Mora aqui, e não no ator do relevo, porque o rio, a trilha e a travessia
	 * precisam da MESMA altura que a superfície desenha. Cada um com a sua
	 * interpolação daria quatro chãos que concordam até a primeira edição — e
	 * a divergência apareceria como rio flutuando alguns centímetros.
	 */
	float HeightAt(const FVector2D& PositionUnits) const;

	/** Onde a terra acaba no rumo dado. Interpola entre os graus vizinhos. */
	float CoastRadiusAt(float BearingRadians) const;

	/**
	 * A FAIXA DE TERRENO num ponto — a única resposta desta pergunta.
	 *
	 * Mora aqui, e não no ator, porque quem pinta o chão e quem diz ao jogador
	 * em que terreno ele está têm de dar a mesma resposta. Duas tabelas
	 * concordam até a primeira edição (L-032).
	 */
	ETerrainBand BandAt(const FVector2D& PositionUnits) const;

	/** O nome da faixa, para o painel. */
	static const TCHAR* BandDebugName(ETerrainBand Band);

	/**
	 * EM QUE TEMPLO OU RUÍNA se está, ou `INDEX_NONE` se em nenhum.
	 *
	 * A regra mora aqui, e não na tela: a tela não decide regra (DP-ui-01).
	 * Com ela no painel, "a linha some ao sair" era comportamento que só um
	 * humano podia conferir — e conferir a olho o que uma função pura responde
	 * é gastar a rodada de alguém com o que a máquina cobra melhor.
	 *
	 * O alcance sai da meia-extensão da própria mancha: alcance fixo anunciaria
	 * uma ruína pequena de longe e uma grande só em cima dela.
	 */
	int32 SacredAt(const FVector2D& PositionUnits) const;

	/**
	 * O chão mais alto em que se PODE ESTAR — fora da mancha queimada.
	 *
	 * É a régua do cume. O ponto mais alto da ilha é o vulcão, e medir por ele
	 * tornava a faixa de cume inalcançável: tudo acima do limiar era queimado,
	 * e queimado vem antes.
	 */
	float HighestWalkableHeightUnits() const;
};

namespace IslandBakedPlan
{
	/** Onde o assado mora. Um lugar só — quem grava e quem lê leem daqui. */
	BATTLESQUARE_API const TCHAR* AssetPath();

	/**
	 * Quantos pontos por curso d'água.
	 *
	 * É o mesmo número de amostras que a carta usa. Assar numa resolução e
	 * desenhar a carta em outra faria as duas divergirem em silêncio no ponto
	 * exato em que a conferência contra a carta é o aceite da feature.
	 */
	BATTLESQUARE_API int32 RiverSampleCount();

	/** O lado da grade de alturas, em casas. */
	BATTLESQUARE_API int32 HeightGridSide();

	/** O progresso (0 a 1) da amostra de índice dado. A conta mora num lugar só. */
	BATTLESQUARE_API float ProgressAtSample(int32 Sample);

	/**
	 * Executa TODOS os planos e devolve o traçado assado.
	 *
	 * Leva minutos. É a ferramenta de assar, não caminho de jogo — o jogo
	 * chama `Load()`.
	 */
	BATTLESQUARE_API void BakeInto(UIslandBakedPlan& Out);

	/** Lê os parâmetros vigentes do jogo — os de AGORA, não os do assado. */
	BATTLESQUARE_API FIslandParameters GatherParameters();

	/**
	 * O resumo dos parâmetros.
	 *
	 * Percorre a reflexão da struct, então campo acrescentado lá entra aqui sem
	 * ninguém lembrar — que é o modo pelo qual uma guarda dessas apodrece.
	 */
	BATTLESQUARE_API uint32 HashParameters(const FIslandParameters& Parameters);

	/**
	 * OS PARÂMETROS QUE DIVERGIRAM entre dois conjuntos, por nome.
	 *
	 * Existe porque "reasse, algo mudou" manda a pessoa procurar sozinha o que
	 * ela já poderia ter lido. Percorre a reflexão, então campo novo aparece
	 * aqui sem ninguém lembrar.
	 */
	BATTLESQUARE_API TArray<FString> DescribeParameterDivergence(
		const FIslandParameters& Baked, const FIslandParameters& Current);

	/**
	 * Carrega o assado gravado, SEM conferir os parâmetros.
	 *
	 * É o caminho de quem está inspecionando o assado — a ferramenta de assar,
	 * o teste da guarda. Quem vai CONSTRUIR o mundo usa `LoadForWorld()`.
	 */
	BATTLESQUARE_API UIslandBakedPlan* Load();

	/**
	 * O assado JÁ CARREGADO, ou nulo — nunca força uma carga.
	 *
	 * Existe para quem roda em caminho quente: o coletor de arena é chamado a
	 * cada encontro, e carregar um `UDataAsset` de 676 KB ali dentro é
	 * trabalho de I/O no meio de uma amostragem. Num mundo de teste sem o
	 * assado carregado isso chegou a derrubar o processo.
	 *
	 * Quem constrói o mundo já chamou `LoadForWorld` no início; quando um
	 * encontro acontece, ele está carregado. Nulo aqui é degradar para o
	 * padrão, e não falhar: uma arena sem substância declarada é água doce,
	 * que é exatamente o que ela era antes deste eixo existir.
	 */
	BATTLESQUARE_API UIslandBakedPlan* LoadedOrNull();

	/**
	 * Carrega o assado E confere que ele é da configuração de agora.
	 *
	 * Devolve nulo quando diverge, depois de dizer alto qual parâmetro mudou.
	 * Devolver o assado velho com um aviso seria pior que nenhum aviso: o
	 * mundo subiria inteiro, parecendo certo, sendo de outra configuração.
	 */
	BATTLESQUARE_API UIslandBakedPlan* LoadForWorld();
}
