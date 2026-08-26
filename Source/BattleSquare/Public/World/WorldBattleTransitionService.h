// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "WorldBattleTransitionService.generated.h"

class AActor;
class ABattleArena;
class AWorldEncounterActor;
class UEncounterDetectionComponent;

namespace WorldBattleTransition
{
	/**
	 * Deslocamento onde a arena nasce, no MESMO UWorld do mundo aberto
	 * (DP-enc-03). Grande o bastante para cair fora de qualquer célula de
	 * World Partition da área de referência de 8000x8000uu — trocar de nível
	 * traria de volta a tela de loading que M5 gastou uma feature para tirar.
	 */
	inline constexpr double ArenaWorldOffsetUnits = 1000000.0;
}

/**
 * Leva o jogador do mundo para a batalha e de volta, sem trocar de nível.
 * Não calcula nada de combate: captura e XP são de quem já os faz (DP-enc-04).
 */
UCLASS()
class BATTLESQUARE_API UWorldBattleTransitionService : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Guarda o transform do pawn, desliga a detecção e spawna a arena no
	 * deslocamento. Devolve a arena, para quem chamou montar a partida — a
	 * montagem em si não é responsabilidade desta classe.
	 */
	ABattleArena* BeginTransition(AActor* WorldPawn,
		UEncounterDetectionComponent* Detection,
		AWorldEncounterActor* Encounter,
		TSubclassOf<ABattleArena> ArenaClass);

	bool IsTransitionActive() const { return bIsTransitionActive; }

	const FTransform& GetCapturedPawnTransform() const { return CapturedPawnTransform; }

	ABattleArena* GetActiveArena() const { return ActiveArena; }

private:
	void HandleBattleFinished();

	UPROPERTY()
	TObjectPtr<AActor> TransitioningPawn;

	UPROPERTY()
	TObjectPtr<UEncounterDetectionComponent> TransitioningDetection;

	UPROPERTY()
	TObjectPtr<AWorldEncounterActor> TransitioningEncounter;

	UPROPERTY()
	TObjectPtr<ABattleArena> ActiveArena;

	FTransform CapturedPawnTransform = FTransform::Identity;

	bool bIsTransitionActive = false;
};
