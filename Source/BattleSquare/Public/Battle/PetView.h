// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Data/BattleDataTranslator.h"
#include "PetView.generated.h"

// T8 (tasks.md, PRES-11/12/13): lado lógico do pet na apresentação — reage
// a eventos do trace (via UBattleTracePlayer, T7), nunca calcula dano ou
// alcance. O que a visão sabe é só o que o evento já trouxe pronto —
// ver design.md, BTL-22.
UCLASS()
class BATTLESQUARE_API APetView : public AActor
{
	GENERATED_BODY()

public:
	APetView();

	// Posiciona o pet e define vida cheia a partir do estado inicial do
	// núcleo + info de apresentação (nome/tag, nunca entra em FPetState).
	// Não exposto ao Blueprint: FPetState/FPetPresentationInfo não são
	// BlueprintType (mesmo motivo de FTurnCommit em BuildCommit) — quem
	// consome isto é C++ (ABattleArena, T10).
	void SetInitialState(const FPetState& InitialState, const FPetPresentationInfo& Presentation);

	// Único ponto de reação a eventos do trace. Nunca recalcula: lê o que
	// o evento já trouxe pronto (Value, ToCell).
	void ApplyEvent(const FBattleEvent& Event);

	UFUNCTION(BlueprintPure)
	float GetHealthRatio() const { return HealthRatio; }

	UFUNCTION(BlueprintPure)
	bool IsDefeated() const { return bDefeated; }

	UFUNCTION(BlueprintPure)
	uint8 GetColumn() const { return Column; }

	UFUNCTION(BlueprintPure)
	uint8 GetRow() const { return Row; }

	UFUNCTION(BlueprintPure)
	uint8 GetPetId() const { return PetId; }

private:
	UPROPERTY()
	uint8 PetId = 0;

	UPROPERTY()
	uint8 Column = 0;

	UPROPERTY()
	uint8 Row = 0;

	UPROPERTY()
	int32 MaxHealth = 0;

	// Derivado de MaxHealth/Health do evento — nunca de um novo cálculo de
	// dano. 1.0 = vida cheia, 0.0 = sem vida.
	UPROPERTY()
	float HealthRatio = 1.0f;

	UPROPERTY()
	bool bDefeated = false;
};
