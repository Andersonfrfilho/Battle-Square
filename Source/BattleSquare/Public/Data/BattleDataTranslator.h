// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Battle/BattleState.h"
#include "Data/PetDataLoader.h"
#include "BattleDataTranslator.generated.h"

// T18 (tasks.md): traduz um pet carregado (FLoadedPetRecord — id uuid,
// type string) para o formato do núcleo (FPetState — só inteiros/enums,
// AD-004). "type" NUNCA entra em FPetState — vira FGameplayTag aqui,
// numa struct de apresentação que só existe em BattleSquare, exatamente
// como GameplayTags nunca entra em BattleSim (AD-012).

// Dados de pet que só interessam à apresentação — nome, tipo/tag. Nunca
// atravessa para o BattleSim.
USTRUCT()
struct FPetPresentationInfo
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 PetId = 0;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FGameplayTag TypeTag;
};

class BATTLESQUARE_API FBattleDataTranslator
{
public:
	// PetId é responsabilidade de quem monta a partida (DP-06 do design:
	// mapeamento uuid->PetId é local à batalha, sequencial, estável
	// durante toda ela) — esta função só traduz UM registro já com o
	// PetId, Side e posição inicial decididos pelo chamador.
	static void TranslatePet(
		const FLoadedPetRecord& Source,
		uint8 PetId,
		uint8 Side,
		uint8 Column,
		uint8 Row,
		FPetState& OutBattleState,
		FPetPresentationInfo& OutPresentation);
};
