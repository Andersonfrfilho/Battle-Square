// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PetDataLoader.h"
#include "Data/BattleDataTranslator.h"
#include "Battle/BattleState.h"

struct FEncounterMatchParams
{
	TArray<FLoadedPetRecord> AvailablePets;
	FString PlayerCatalogId;
	FString EncounterCatalogId;
	FString PetCollectionSlotName;
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
