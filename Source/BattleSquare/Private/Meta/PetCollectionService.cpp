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

void FPetCollectionService::SaveCollection(const FString& SlotName, const TArray<FOwnedPetInstance>& Collection)
{
	UPetCollectionSaveGame* SaveGame = Cast<UPetCollectionSaveGame>(UGameplayStatics::CreateSaveGameObject(UPetCollectionSaveGame::StaticClass()));
	SaveGame->OwnedPets = Collection;

	// O PERFIL DO TREINADOR sobrevive. Esta função monta um save NOVO e grava
	// por cima; sem reler o que já estava lá, cada ganho de experiência
	// apagaria as especialidades — e o jogador perderia uma escolha que não
	// se refaz, sem nada indicando quando nem por quê.
	SaveGame->Trainer = LoadTrainerProfile(SlotName);

	UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, /*UserIndex=*/0);
}

FTrainerProfile FPetCollectionService::LoadTrainerProfile(const FString& SlotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex=*/0))
	{
		return {};
	}

	if (const UPetCollectionSaveGame* SaveGame = Cast<UPetCollectionSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, /*UserIndex=*/0)))
	{
		return SaveGame->Trainer;
	}

	return {};
}

void FPetCollectionService::SaveTrainerProfile(const FString& SlotName, const FTrainerProfile& Profile)
{
	UPetCollectionSaveGame* SaveGame = Cast<UPetCollectionSaveGame>(UGameplayStatics::CreateSaveGameObject(UPetCollectionSaveGame::StaticClass()));

	// Simétrico ao de cima, e pelo mesmo motivo: gravar o treinador não pode
	// apagar a coleção.
	SaveGame->OwnedPets = LoadCollection(SlotName);
	SaveGame->Trainer = Profile;

	UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, /*UserIndex=*/0);
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
