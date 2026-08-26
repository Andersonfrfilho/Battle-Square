// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterMatchAssembler.h"
#include "Net/BattleSquareGameMode.h"

namespace
{
	const FLoadedPetRecord* FindPetByCatalogId(const TArray<FLoadedPetRecord>& Pets, const FString& CatalogId)
	{
		return Pets.FindByPredicate([&CatalogId](const FLoadedPetRecord& Pet) { return Pet.Id == CatalogId; });
	}
}

bool FEncounterMatchAssembler::AssembleFromEncounter(const FEncounterMatchParams& Params,
	FBattleState& OutInitialState,
	TArray<FPetPresentationInfo>& OutPresentations)
{
	const FLoadedPetRecord* PlayerRecord = FindPetByCatalogId(Params.AvailablePets, Params.PlayerCatalogId);
	const FLoadedPetRecord* EncounterRecord = FindPetByCatalogId(Params.AvailablePets, Params.EncounterCatalogId);

	// CatalogId que não existe no espelho é erro de configuração do nível,
	// nunca um pet substituído em silêncio — mesma postura de ARENA-02.
	if (!PlayerRecord || !EncounterRecord)
	{
		return false;
	}

	FPetState PlayerPet;
	FPetPresentationInfo PlayerPresentation;
	FBattleDataTranslator::TranslatePet(*PlayerRecord, /*PetId=*/1, /*Side=*/0, /*Column=*/1, /*Row=*/1, PlayerPet, PlayerPresentation);

	FPetState EncounterPet;
	FPetPresentationInfo EncounterPresentation;
	FBattleDataTranslator::TranslatePet(*EncounterRecord, /*PetId=*/2, /*Side=*/1, /*Column=*/2, /*Row=*/1, EncounterPet, EncounterPresentation);

	ABattleSquareGameMode::ApplyOwnedPetProgressionBonus(Params.PetCollectionSlotName, PlayerPet, PlayerPresentation);
	ABattleSquareGameMode::ApplyOwnedPetProgressionBonus(Params.PetCollectionSlotName, EncounterPet, EncounterPresentation);

	OutInitialState = FBattleState();
	OutInitialState.Pets.Add(PlayerPet);
	OutInitialState.Pets.Add(EncounterPet);

	OutPresentations.Reset();
	OutPresentations.Add(PlayerPresentation);
	OutPresentations.Add(EncounterPresentation);
	return true;
}
