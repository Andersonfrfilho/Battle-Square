// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetCollectionService.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/**
	 * Carrega o save INTEIRO, deixa quem chamou mexer num pedaço, e grava.
	 *
	 * Existe porque a alternativa não escala: cada gravação relia as OUTRAS
	 * metades para não apagá-las, e com dois campos isso já era duas
	 * releituras cruzadas (L-048, que nasceu de gravar XP e perder as
	 * especialidades). Com três seria seis, e o defeito volta na primeira que
	 * alguém esquecer.
	 *
	 * Aqui não há o que esquecer: o que não foi tocado veio do disco e volta
	 * para ele.
	 */
	template <typename FMutacao>
	void MutarSave(const FString& SlotName, FMutacao&& Mutacao)
	{
		UPetCollectionSaveGame* SaveGame = nullptr;

		if (UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex=*/0))
		{
			SaveGame = Cast<UPetCollectionSaveGame>(
				UGameplayStatics::LoadGameFromSlot(SlotName, /*UserIndex=*/0));
		}

		if (!SaveGame)
		{
			SaveGame = Cast<UPetCollectionSaveGame>(UGameplayStatics::CreateSaveGameObject(
				UPetCollectionSaveGame::StaticClass()));
		}

		Mutacao(*SaveGame);
		UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, /*UserIndex=*/0);
	}
}

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
	// Só a COLEÇÃO é tocada; o resto do save volta como veio do disco. Antes
	// esta função montava um save novo e relia o treinador à mão para não
	// apagá-lo — funcionava, e não sobrevivia ao terceiro campo (L-048).
	MutarSave(SlotName, [&Collection](UPetCollectionSaveGame& Save)
	{
		Save.OwnedPets = Collection;
	});
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
	MutarSave(SlotName, [&Profile](UPetCollectionSaveGame& Save)
	{
		Save.Trainer = Profile;
	});
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

FWorldDiscovery FPetCollectionService::LoadDiscovery(const FString& SlotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex=*/0))
	{
		return {};
	}

	if (const UPetCollectionSaveGame* SaveGame = Cast<UPetCollectionSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, /*UserIndex=*/0)))
	{
		return SaveGame->Discovery;
	}

	return {};
}

void FPetCollectionService::SaveDiscovery(const FString& SlotName, const FWorldDiscovery& Discovery)
{
	MutarSave(SlotName, [&Discovery](UPetCollectionSaveGame& Save)
	{
		Save.Discovery = Discovery;
	});
}
