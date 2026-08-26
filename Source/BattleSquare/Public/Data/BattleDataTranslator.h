// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Battle/BattleState.h"
#include "Data/PetDataLoader.h"
#include "Balance/TypeEffectivenessTable.h"
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

	// T3 (escala-pets-skills, design.md — DP-escala-01): traduz os DOIS
	// lados de uma partida ao mesmo tempo, pré-multiplicando o Attack de
	// cada lado pela efetividade do tipo dele contra o tipo do oponente.
	// Chama a mesma lógica de TranslatePet internamente para os campos
	// que não mudam (Defense/Speed/MaxHealth/Health nunca são alterados
	// por tipo — só Attack). O núcleo (BattleSim) nunca sabe que tipo
	// existe: o Attack que ele recebe já chega efetivo.
	static void TranslateMatchup(
		const FLoadedPetRecord& LeftSource,
		const FLoadedPetRecord& RightSource,
		const FTypeEffectivenessTable& EffectivenessTable,
		uint8 LeftPetId,
		uint8 RightPetId,
		FPetState& OutLeftState,
		FPetPresentationInfo& OutLeftPresentation,
		FPetState& OutRightState,
		FPetPresentationInfo& OutRightPresentation);
};

