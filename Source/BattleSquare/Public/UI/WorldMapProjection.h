// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/IslandGeography.h"
#include "World/WorldDiscovery.h"

/**
 * O TERRENO de um pedaço do mundo, para o mapa desenhar.
 *
 * O mapa mostrava marcadores sobre dois círculos — terra e água — e por isso
 * dizia onde as COISAS estão sem dizer como é o LUGAR. Quem olhava não sabia
 * se o caminho até o campo de Voo atravessa mata fechada ou clareira, que é
 * justamente o que um mapa serve para responder antes de andar.
 */
UENUM()
enum class EWorldMapTerrain : uint8
{
	/** Chão aberto: dá para atravessar reto. */
	Clareira,
	/** Mata: tronco e pedra em pé, e é onde a arena nasce cheia de bloqueio. */
	Mata,
	/** A margem — molhado, e onde a lama aparece em clima úmido. */
	Margem,
	Agua,
	/** Serra: o que fecha o horizonte e não se atravessa. */
	Relevo,
	/** Areia seca do deserto. Atravessa-se, mas não há o que colher. */
	Areia,
	/** Gelo do glaciar. É onde a aurora aparece. */
	Gelo,
	/** Rocha escura do vulcão. */
	Lava,

	/**
	 * Quantos terrenos existem. NÃO é terreno.
	 *
	 * A legenda e o teste dela varrem daqui, em vez de repetir a lista à mão.
	 * Repetida, um terreno novo entrava no mapa e ficava de fora da legenda —
	 * mancha na tela que nada explica, e nada quebrando para avisar.
	 */
	Count
};

/** Um pedaço do mundo, do tamanho que `TerrainTileSideUnits` disser. */
struct BATTLESQUARE_API FWorldMapTerrainTile
{
	FVector2D WorldXY = FVector2D::ZeroVector;
	EWorldMapTerrain Kind = EWorldMapTerrain::Clareira;
};

/** O que um marcador do mapa é — a forma diz a categoria, a cor diz qual. */
UENUM()
enum class EWorldMapMarker : uint8
{
	Jogador,
	CampoDeTreino,
	Adversario,
	Relevo
};

struct BATTLESQUARE_API FWorldMapMarkerInfo
{
	FVector2D WorldXY = FVector2D::ZeroVector;
	FLinearColor Color = FLinearColor::White;
	EWorldMapMarker Kind = EWorldMapMarker::Relevo;

	/** Raio no MUNDO, em unidades. O mapa converte para pixel. */
	float WorldRadiusUnits = 100.0f;
};

/** O mundo como o mapa o vê, num instante. */
struct BATTLESQUARE_API FWorldMapSnapshot
{
	FVector2D PlayerXY = FVector2D::ZeroVector;

	/** Para onde o jogador OLHA, em graus. Yaw 0 é o +X do mundo. */
	float PlayerYawDegrees = 0.0f;

	/** Onde a terra acaba — o mapa desenha a água a partir daqui. */
	float ShoreRadiusUnits = IslandGeography::LandRadiusUnits();

	TArray<FWorldMapMarkerInfo> Markers;

	/**
	 * O que o jogador já viu. O mapa só desenha marcador em região descoberta.
	 *
	 * Viaja no retrato, e não é consultado do save por quem desenha: a tela
	 * não busca dado (DP-ui-01). E como o retrato é montado num temporizador
	 * lento, o mapa não fica lendo save a cada quadro.
	 */
	FWorldDiscovery Discovery;

	/**
	 * O mapa RESPEITA a descoberta?
	 *
	 * Falso é o mapa completo — o que ele era antes desta feature, e o que um
	 * teste de outro assunto continua querendo. Explícito, e não deduzido de
	 * "a descoberta está vazia": vazia é o estado de quem nunca andou, e
	 * confundir os dois faria o mapa nascer completo justamente para quem
	 * acabou de começar.
	 */
	bool bHidesUndiscovered = true;

	/**
	 * O terreno, em pedaços do tamanho de uma região de descoberta.
	 *
	 * Mesmo tamanho de propósito: a unidade do que se revela e a unidade do
	 * que se desenha têm de coincidir, senão meia região fica pintada e meia
	 * não — e a borda da descoberta vira serrilha em vez de fronteira.
	 */
	TArray<FWorldMapTerrainTile> Terrain;
};

/**
 * Converte mundo em mapa.
 *
 * PURA, e separada do desenho de propósito: o erro que este tipo de código
 * comete não é de pintura, é de EIXO. Este projeto já pagou por um —
 * "Baixo" andava para a direita porque coluna virou X e linha virou Y — e
 * custou o usuário jogar e descrever o que viu. Aqui o mapeamento tem teste
 * que fixa os quatro pontos cardeais, e ele roda sem abrir o editor.
 *
 * CONVENÇÃO DO MUNDO: +X é o norte, +Y é o leste (é o mesmo mapeamento que a
 * arena usa para a câmera, e discordar dele faria o mapa mentir sobre o
 * mundo).
 */
class BATTLESQUARE_API FWorldMapProjection
{
public:
	/**
	 * Modo do mapa. Não é preferência: são usos diferentes.
	 *
	 * `SeguindoOOlhar` põe para cima o que está à frente — é o que serve
	 * enquanto se anda, porque casa com o que a tela mostra. `NorteAcima` fixa
	 * a orientação, e é o que serve para planejar: um mapa que gira enquanto
	 * se olha para ele não se memoriza.
	 */
	enum class EMode : uint8
	{
		SeguindoOOlhar,
		NorteAcima
	};

	/**
	 * Posição no mapa, em coordenadas de TELA normalizadas: (0,0) é o centro,
	 * X cresce para a direita, Y cresce para BAIXO — a convenção do Slate.
	 *
	 * `RangeUnits` é o raio do mundo que cabe no mapa. Fora dele o resultado
	 * passa de 1, e quem desenha decide se recorta ou empurra para a borda.
	 */
	static FVector2D ToMapSpace(const FVector2D& WorldXY, const FWorldMapSnapshot& Snapshot,
		EMode Mode, float RangeUnits);

	/**
	 * Este marcador deve APARECER?
	 *
	 * Mora aqui, e não em quem desenha, porque a resposta é a mesma para o
	 * minimapa e para o mapa completo — e duas cópias concordariam até a
	 * primeira edição, com o sintoma sendo um adversário visível num mapa e
	 * escondido no outro.
	 *
	 * O JOGADOR é sempre visível. Ele está onde está; escondê-lo por causa de
	 * uma região não marcada seria o mapa negar a única coisa que ele sabe
	 * com certeza.
	 */
	static bool IsMarkerVisible(const FWorldMapMarkerInfo& Marker,
		const FWorldMapSnapshot& Snapshot);

	/**
	 * A cor de cada terreno no mapa.
	 *
	 * Mora aqui, junto da projeção, e não em quem desenha: o minimapa e o mapa
	 * completo precisam pintar IGUAL, e a legenda precisa mostrar a MESMA cor
	 * que o mapa usa. Três cópias produziriam uma legenda que descreve um mapa
	 * que não existe — pior que legenda nenhuma.
	 */
	/**
	 * Quanto do mundo o mapa mostra além da beira da terra.
	 *
	 * Sessenta por cento a mais, para o mar aparecer à volta da ilha: mapa
	 * cortado exatamente na areia não mostra que ali ACABA — mostra só que a
	 * folha acabou.
	 */
	static constexpr float TerrainMarginFactor = 1.6f;

	/** Quantos pedaços de terreno o mapa desenha de ponta a ponta. */
	static constexpr int32 TerrainTilesAcross = 48;

	/**
	 * O lado de um pedaço de terreno, para a ilha que existe.
	 *
	 * Acompanha a ilha em vez de ser fixo: com a ilha em 20000 de raio, um
	 * pedaço fixo de 800 daria seis mil manchas de menos de um pixel — custo
	 * de desenho que ninguém vê, e o pedido foi explícito que nada pode ficar
	 * devagar. Quarenta e oito de ponta a ponta é mancha visível e conta
	 * constante, cresça a ilha o quanto crescer.
	 */
	static float TerrainTileSideUnits(float LandRadiusUnits);

	static FLinearColor ColorForTerrain(EWorldMapTerrain Terrain);

	/** O nome que o jogador lê na legenda. */
	static FText LabelForTerrain(EWorldMapTerrain Terrain);

	/**
	 * O terreno que o mapa desenha para cada bioma da ilha.
	 *
	 * O mapa deduzia mata CONTANDO troncos plantados. Com a ilha montada por
	 * pedaços residentes, só os nove à volta do jogador existem — e o mapa
	 * passaria a esvaziar e repovoar atrás de quem anda, dizendo que o
	 * deserto do outro lado da ilha é clareira porque ninguém está lá.
	 *
	 * A geografia sabe o que cada ponto é sem que nada esteja montado. É ela
	 * quem responde, e esta função é a única tradução de bioma para terreno.
	 */
	static EWorldMapTerrain TerrainForBiome(EIslandBiome Biome);

	/** Para onde a seta do jogador aponta na tela, em graus horários. */
	static float PlayerArrowAngleDegrees(const FWorldMapSnapshot& Snapshot, EMode Mode);
};
