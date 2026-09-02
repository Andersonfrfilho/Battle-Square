#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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

	/** Onde há corredeira, na mesma ordem dos pontos. */
	UPROPERTY()
	TArray<bool> bIsRapids;

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

/** Uma fonte: o olho d'água e o poço que ele forma. */
USTRUCT()
struct BATTLESQUARE_API FBakedSpring
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2D CenterUnits = FVector2D::ZeroVector;

	UPROPERTY()
	float PoolHalfWidthUnits = 0.0f;
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
	TArray<FBakedPolyline> Brooks;

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
	TArray<FBakedPolyline> Aqueducts;

	/** A altura no índice de grade, sem o consumidor refazer a conta do índice. */
	float HeightAtCell(int32 Column, int32 Row) const;

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
	 * Carrega o assado E confere que ele é da configuração de agora.
	 *
	 * Devolve nulo quando diverge, depois de dizer alto qual parâmetro mudou.
	 * Devolver o assado velho com um aviso seria pior que nenhum aviso: o
	 * mundo subiria inteiro, parecendo certo, sendo de outra configuração.
	 */
	BATTLESQUARE_API UIslandBakedPlan* LoadForWorld();
}
