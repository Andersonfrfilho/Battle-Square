// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterMatchAssembler.h"
#include "Misc/Paths.h"
#include "Balance/TypeEffectivenessTable.h"
#include "Net/BattleSquareGameMode.h"
#include "Misc/DateTime.h"

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
	FPetState EncounterPet;
	FPetPresentationInfo EncounterPresentation;

	// TranslateMatchup, e não TranslatePet: só ele aplica a efetividade de
	// tipo. Com TranslatePet, Fogo contra Planta batia igual a Fogo contra
	// Água — a tabela existia, era testada, e nunca chegava ao jogo.
	FTypeEffectivenessTable Efetividade;
	if (!FTypeEffectivenessTable::LoadFromJson(
		FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("TypeEffectiveness.json")), Efetividade))
	{
		// Tabela ausente degrada para NEUTRO — GetPercent devolve 100 para
		// par desconhecido, e uma tabela vazia é exatamente isso. Nunca
		// impede a batalha de acontecer.
		UE_LOG(LogTemp, Warning,
			TEXT("EncounterMatchAssembler: tabela de efetividade nao carregou — combate neutro"));
	}

	FBattleDataTranslator::TranslateMatchup(*PlayerRecord, *EncounterRecord, Efetividade,
		/*LeftPetId=*/1, /*RightPetId=*/2,
		PlayerPet, PlayerPresentation, EncounterPet, EncounterPresentation);

	ABattleSquareGameMode::ApplyOwnedPetProgressionBonus(Params.PetCollectionSlotName, PlayerPet, PlayerPresentation);
	ABattleSquareGameMode::ApplyOwnedPetProgressionBonus(Params.PetCollectionSlotName, EncounterPet, EncounterPresentation);

	OutInitialState = FBattleState();
	// A semente é decisão de MONTAGEM, não do núcleo: o BattleSim recebe um
	// estado já semeado e nunca consulta relógio nenhum.
	OutInitialState.Random.State = Params.RandomSeed != 0
		? Params.RandomSeed
		: static_cast<uint64>(FDateTime::Now().GetTicks());
	OutInitialState.Pets.Add(PlayerPet);
	OutInitialState.Pets.Add(EncounterPet);
	OutInitialState.PlaceDuelistsAtStartingCells();

	OutPresentations.Reset();
	OutPresentations.Add(PlayerPresentation);
	OutPresentations.Add(EncounterPresentation);
	return true;
}
