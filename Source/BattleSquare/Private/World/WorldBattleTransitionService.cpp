// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldBattleTransitionService.h"
#include "World/EncounterDetectionComponent.h"
#include "World/WorldEncounterActor.h"
#include "Battle/BattleArena.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

ABattleArena* UWorldBattleTransitionService::BeginTransition(AActor* WorldPawn,
	UEncounterDetectionComponent* Detection,
	AWorldEncounterActor* Encounter,
	TSubclassOf<ABattleArena> ArenaClass)
{
	if (bIsTransitionActive || !WorldPawn || !Detection || !Encounter || !ArenaClass)
	{
		return nullptr;
	}

	UWorld* World = WorldPawn->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	CapturedPawnTransform = WorldPawn->GetActorTransform();

	const FVector ArenaLocation(WorldBattleTransition::ArenaWorldOffsetUnits,
		WorldBattleTransition::ArenaWorldOffsetUnits,
		WorldBattleTransition::ArenaWorldOffsetUnits);
	ABattleArena* Arena = World->SpawnActor<ABattleArena>(ArenaClass, ArenaLocation, FRotator::ZeroRotator);
	if (!Arena)
	{
		return nullptr;
	}

	TransitioningPawn = WorldPawn;
	TransitioningDetection = Detection;
	TransitioningEncounter = Encounter;
	ActiveArena = Arena;
	bIsTransitionActive = true;

	Detection->SetDetectionEnabled(false);
	Arena->OnBattleFinished.AddUObject(this, &UWorldBattleTransitionService::HandleBattleFinished);

	return Arena;
}

void UWorldBattleTransitionService::HandleBattleFinished()
{
	if (!bIsTransitionActive)
	{
		return;
	}

	// A arena só morre aqui, e OnBattleFinished já roda depois de captura e
	// XP — destruí-la antes perderia os dois (DP-enc-04).
	if (ActiveArena)
	{
		ActiveArena->Destroy();
		ActiveArena = nullptr;
	}

	if (TransitioningPawn)
	{
		TransitioningPawn->SetActorTransform(CapturedPawnTransform);
	}

	// A ordem daqui para baixo é a regra, não detalhe: marcar resolvido ANTES
	// de religar é o que faz o jogador voltar parado em cima do pet derrotado
	// sem cair num segundo encontro (P1, critério 4).
	if (TransitioningEncounter)
	{
		TransitioningEncounter->MarkResolved();
	}

	if (TransitioningDetection)
	{
		TransitioningDetection->SetDetectionEnabled(true);
	}

	TransitioningPawn = nullptr;
	TransitioningDetection = nullptr;
	TransitioningEncounter = nullptr;
	bIsTransitionActive = false;
}
