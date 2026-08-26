// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetCollectionService.h"
#include "Kismet/GameplayStatics.h"

bool FPetCollectionService::CaptureIfNewInMemory(UPetCollectionSaveGame* SaveGame, const FOwnedPetInstance& Instance)
{
	if (!SaveGame)
	{
		return false;
	}

	for (const FOwnedPetInstance& Owned : SaveGame->OwnedPets)
	{
		if (Owned.CatalogId == Instance.CatalogId)
		{
			return false;
		}
	}

	SaveGame->OwnedPets.Add(Instance);
	return true;
}

bool FPetCollectionService::CaptureIfNew(const FString& SlotName, const FOwnedPetInstance& Instance)
{
	UPetCollectionSaveGame* SaveGame = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex=*/0))
	{
		SaveGame = Cast<UPetCollectionSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, /*UserIndex=*/0));
	}

	// Ausente ou corrompido (Cast falhou) — começa vazio, nunca crash.
	if (!SaveGame)
	{
		SaveGame = Cast<UPetCollectionSaveGame>(UGameplayStatics::CreateSaveGameObject(UPetCollectionSaveGame::StaticClass()));
	}

	const bool bCaptured = CaptureIfNewInMemory(SaveGame, Instance);
	if (bCaptured)
	{
		UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, /*UserIndex=*/0);
	}

	return bCaptured;
}

TArray<FOwnedPetInstance> FPetCollectionService::LoadCollection(const FString& SlotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex=*/0))
	{
		return {};
	}

	if (const UPetCollectionSaveGame* SaveGame = Cast<UPetCollectionSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, /*UserIndex=*/0)))
	{
		return SaveGame->OwnedPets;
	}

	return {};
}
