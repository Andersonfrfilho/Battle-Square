// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"

/**
 * O que existe num ponto do mundo, para efeito de terreno de arena.
 *
 * É um vocabulário CURTO de propósito: o mundo tem dezenas de papéis de
 * cenário (`EScenaryRole`), e a arena tem cinco terrenos. Traduzir aqui, e não
 * lá, é o que impede o núcleo de aprender o que é um cogumelo.
 */
enum class EWorldFeatureKind : uint8
{
	/** Tronco, pedra, qualquer coisa com CORPO. Vira casa bloqueada. */
	Solid,
	/** Água funda: o rio, o lago, a faixa que fecha o mundo. */
	DeepWater,
	/** A beira: molha o pé e não dá para submergir. */
	Shore,
	/** Clareira de treino — a casa que ajuda. */
	TrainingGround
};

struct FWorldFeatureSample
{
	FVector Location = FVector::ZeroVector;
	EWorldFeatureKind Kind = EWorldFeatureKind::Solid;
};

struct FArenaFromWorldParams
{
	/** Onde o encontro aconteceu. O centro da grade cai aqui. */
	FVector EncounterLocation = FVector::ZeroVector;

	float CellSize = 200.0f;
	int32 Columns = 3;
	int32 Rows = 3;

	/** Umidade do lugar. Beira de água em clima úmido vira LAMA, não poça. */
	int32 HumidityPercent = 70;

	/**
	 * Está ALAGADO: a água transbordou o leito e a margem subiu.
	 *
	 * Separado da umidade porque são coisas de natureza diferente. Umidade
	 * MOLHA o que já era beira de água — é adjetivo do terreno que existe.
	 * Alagamento CRIA terreno: casa seca encostada na água vira poça, e uma
	 * arena sem gota nenhuma continua seca por mais que chova, porque não há
	 * leito de onde a água saia.
	 */
	bool bFlooded = false;

	TArray<FWorldFeatureSample> Features;
};

/**
 * A arena é o PEDAÇO DE MAPA onde o encontro aconteceu.
 *
 * Antes ela era sorteada de um catálogo: o jogador topava com um inimigo na
 * beira do lago e caía num "Campo Aberto" — o lugar onde ele estava não tinha
 * relação nenhuma com o lugar onde ele lutava. Terreno virava decoração
 * aleatória em vez de consequência de para onde ele andou.
 *
 * PURA de propósito, e é a razão de existir deste arquivo separado: quem toca
 * o mundo só COLETA amostras, e quem decide o tabuleiro se verifica sem
 * levantar `UWorld`, sem carregar malha e sem abrir o Editor. Foi a mesma
 * separação que tornou `FEncounterDetector` e `ScenaryClimate` testáveis.
 */
class BATTLESQUARE_API FArenaFromWorld
{
public:
	/**
	 * O tabuleiro daquele pedaço de mundo.
	 *
	 * Devolve vazio quando não há amostra nenhuma: arena sem informação de
	 * mundo tem de cair no catálogo de sempre, e não num campo todo neutro
	 * que pareceria uma escolha.
	 */
	static TArray<uint8> Build(const FArenaFromWorldParams& Params);

	/**
	 * Nenhuma casa INICIAL pode ficar bloqueada.
	 *
	 * A montagem rejeita layout que bloqueie onde um duelista nasce, e a
	 * batalha simplesmente não abre. Com catálogo, dava para tentar o próximo
	 * layout; com o mundo, não existe próximo — o lugar é aquele. Então a
	 * pedra que caiu numa casa inicial é REMOVIDA, e é a escolha certa: o
	 * jogador vê o mato abrir onde ele entrou, e não uma batalha que não
	 * começa.
	 */
	static void ClearStartingCells(TArray<uint8>& Layout, int32 Columns, int32 Rows);
};
