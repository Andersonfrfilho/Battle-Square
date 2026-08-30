// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PetDataLoader.h"
#include "Data/BattleDataTranslator.h"
#include "Battle/BattleState.h"
#include "World/ArenaFromWorld.h"

struct FEncounterMatchParams
{
	TArray<FLoadedPetRecord> AvailablePets;
	FString PlayerCatalogId;
	FString EncounterCatalogId;
	FString PetCollectionSlotName;

	/**
	 * Semente do gerador da partida. Zero = a engine escolhe uma por relógio.
	 *
	 * Sem isto toda batalha começava com State=0, e como o PCG32 é
	 * determinístico o PRIMEIRO TURNO do oponente era sempre idêntico
	 * (Esquivar, Aguardar, Esquivar — nenhuma delas move). Num jogo de commit
	 * às cegas, oponente previsível anula a mecânica.
	 *
	 * Determinismo NÃO se perde (AD-004): a semente vive dentro de
	 * FBattleState, então a mesma semente reproduz a mesma partida. O que
	 * muda é que partidas diferentes começam em pontos diferentes.
	 * Teste que precisa de resultado fixo passa a semente explicitamente.
	 */
	uint64 RandomSeed = 0;

	/**
	 * O PEDAÇO DE MAPA onde o encontro aconteceu.
	 *
	 * Vazio é legítimo e significa "batalha sem mundo" — teste, tela de
	 * batalha aberta direto, partida montada à mão. Aí a arena cai no
	 * catálogo de sempre, que continua existindo por isso.
	 */
	FVector EncounterLocation = FVector::ZeroVector;
	TArray<FWorldFeatureSample> WorldFeatures;
};

/**
 * Monta o estado inicial de uma partida nascida de um encontro no mundo.
 * É montagem, não combate: escolhe QUEM entra, nunca calcula o que acontece
 * (DP-enc-01, e a mesma categoria de TranslateMatchup/ApplyOwnedPetProgressionBonus
 * que L-021/L-022 já isentaram da sonda de recálculo).
 */
class BATTLESQUARE_API FEncounterMatchAssembler
{
public:
	static bool AssembleFromEncounter(const FEncounterMatchParams& Params,
		FBattleState& OutInitialState,
		TArray<FPetPresentationInfo>& OutPresentations);
};
