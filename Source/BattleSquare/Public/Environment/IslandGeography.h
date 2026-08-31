// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryClimate.h"

/**
 * O bioma de um pedaço da ilha.
 *
 * A lista é curta de propósito. Cada valor aqui precisa de um chão, uma
 * paleta e um povoamento próprios para existir na tela — bioma sem isso é
 * nome num enum, e nome num enum não é lugar nenhum.
 */
enum class EIslandBiome : uint8
{
	/** A mata: o lugar de casa, e o único que hoje tem povoamento autoral. */
	Forest,
	/** Areia seca e pedra clara. Nunca enlameia. */
	Desert,
	/** Gelo. É onde a aurora aparece, e onde a neve das montanhas desce. */
	Glacier,
	/** Rocha escura e lava. Seco como o deserto, mas por calor, não por sol. */
	Volcano,
	/** A borda molhada, onde a terra encontra o mar. */
	Beach,

	/**
	 * O pântano: a mata que não consegue drenar para o mar.
	 *
	 * Entra no FIM porque a ordem daqui já foi lida por quem salva e por quem
	 * desenha, e valor no meio renumera o que já está escrito.
	 *
	 * Não é setor, e é de propósito. Pântano não é uma direção da ilha, é uma
	 * ALTURA: terra baixa e plana onde a água doce empoça antes de chegar na
	 * areia. Por isso ele mora na faixa entre a mata e a praia, e só do lado
	 * úmido — onde o deserto encontra o mar sai areia seca, não brejo.
	 */
	Swamp,
};

/**
 * Onde cada coisa fica na ilha — e, agora, o que cada lugar É.
 *
 * Este arquivo existe por uma dívida concreta: o raio da terra estava
 * transcrito em SEIS lugares (dois cabeçalhos, três constantes de arquivo e um
 * teste). Seis cópias de um número concordam até a primeira vez que uma delas
 * muda — e a próxima mudança é justamente crescer a ilha. Uma fonte só (L-032).
 *
 * O clima deixa de ser uma linha de `.ini` para o mundo inteiro e passa a ser
 * função da POSIÇÃO. É o que permite andar do deserto para a geleira sem
 * carregar nada: o lugar responde por si.
 *
 * Tudo aqui é conta pura sobre números, sem ator e sem mundo. É o que torna
 * "a caverna caiu dentro da montanha" uma asserção de milissegundos em vez de
 * uma caminhada dentro do jogo.
 */
namespace IslandGeography
{
	/**
	 * Raio da terra, em unidades. A fonte única.
	 *
	 * Configurável em `DefaultGame.ini`, seção
	 * `[/Script/BattleSquare.BattleSquareGameMode]`, chave
	 * `WorldIslandRadiusUnits`.
	 */
	BATTLESQUARE_API float LandRadiusUnits();

	/** Largura da faixa de praia, medida para dentro a partir da borda. */
	BATTLESQUARE_API float BeachWidthUnits();

	/**
	 * Largura da faixa de pântano, medida para dentro a partir da praia.
	 *
	 * Fração do raio pelo mesmo motivo da praia: um número fixo que hoje é um
	 * brejo vira uma poça quando a ilha crescer.
	 */
	BATTLESQUARE_API float SwampWidthUnits();

	/**
	 * Raio do miolo que continua sendo mata, aconteça o que acontecer.
	 *
	 * Os campos de treino, o ponto de nascimento e o caminho entre eles vivem
	 * aqui. Sortear o bioma do centro faria o jogo às vezes começar num
	 * glaciar sem nenhuma das plantas que ele conhece.
	 */
	BATTLESQUARE_API float HomeRadiusUnits();

	/**
	 * Em quantas fatias a ilha se divide.
	 *
	 * Cinco, e não quatro: com quatro, cada bioma ficaria num quadrante e a
	 * mata — que é a casa — teria o mesmo tamanho do glaciar. Com cinco, dois
	 * setores são de mata e ela continua predominando.
	 */
	constexpr int32 SectorCount = 5;

	/** Em que setor cai o ponto. Zero é o setor que começa no eixo +X. */
	BATTLESQUARE_API int32 SectorAt(const FVector2D& PositionUnits);

	/** O bioma daquele setor, sem olhar a distância do centro. */
	BATTLESQUARE_API EIslandBiome BiomeOfSector(int32 Sector);

	/**
	 * O bioma de um ponto.
	 *
	 * A ordem importa: o miolo é mata, a borda é praia, logo atrás da praia
	 * vem o pântano nos setores de mata, e só depois o setor decide. Sem o
	 * miolo, a casa mudaria de bioma; sem a borda, o deserto encostaria no mar
	 * sem areia molhada no meio; e sem o pântano vir ANTES do setor, ele nunca
	 * seria alcançado, porque o setor de mata responderia mata primeiro.
	 */
	BATTLESQUARE_API EIslandBiome BiomeAt(const FVector2D& PositionUnits);

	/**
	 * O clima daquele bioma.
	 *
	 * O vulcão é `Desert` de propósito: seco, sem neve e sem lama. Ele é seco
	 * por calor de baixo, o deserto é seco por sol de cima, e para tudo o que
	 * o jogo pergunta hoje — nevar, enlamear, esquentar — os dois respondem
	 * igual. Um `EScenaryClimate` novo só para o vulcão seria uma tabela a
	 * mais dizendo o que esta já diz.
	 */
	BATTLESQUARE_API EScenaryClimate ClimateOf(EIslandBiome Biome);

	/** O clima de um ponto — o que substitui a leitura global do `.ini`. */
	BATTLESQUARE_API EScenaryClimate ClimateAt(const FVector2D& PositionUnits);

	/**
	 * O clima do SETOR para onde a posição aponta, sem olhar a distância.
	 *
	 * A serra do horizonte fica a quilômetros da ilha — muito além da praia e
	 * da água. `ClimateAt` responderia "praia" para todos os picos, e a
	 * geleira perderia o gelo por estar longe demais de si mesma.
	 *
	 * Aqui a direção é que manda: o horizonte ao norte é o horizonte DO setor
	 * que fica ao norte, esteja ele a seis mil unidades ou a um milhão.
	 */
	BATTLESQUARE_API EScenaryClimate SectorClimateAt(const FVector2D& PositionUnits);

	/** Está fora da terra, na água. */
	BATTLESQUARE_API bool IsOnLand(const FVector2D& PositionUnits);

	/** Nome curto para o painel de desenvolvimento. Nunca texto de jogador. */
	BATTLESQUARE_API const TCHAR* BiomeDebugName(EIslandBiome Biome);
}
