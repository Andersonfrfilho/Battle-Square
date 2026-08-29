// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldBattleTransitionService.h"
#include "World/EncounterDetectionComponent.h"
#include "World/WorldEncounterActor.h"
#include "Battle/BattleArena.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

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

	// A arena veste o lugar de onde o jogador veio. Ela nasce a um milhão de
	// unidades dali (DP-enc-03): luz, céu e névoa são globais e já a alcançam,
	// mas o chão não — e sem isto a batalha parecia acontecer noutro mundo.
	// Recusa é aceitável: fica a paleta autorada.
	Arena->AdoptAmbienceFromWorldLocation(CapturedPawnTransform.GetLocation());

	Detection->SetDetectionEnabled(false);
	Arena->OnBattleFinished.AddUObject(this, &UWorldBattleTransitionService::HandleBattleFinished);
	SetViewTarget(Arena);

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
		// A visão volta ANTES de a detecção religar: o jogador precisa estar
		// de volta ao mundo quando o próximo encontro puder disparar.
		SetViewTarget(TransitioningPawn);
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

void UWorldBattleTransitionService::SetViewTarget(AActor* NewViewTarget)
{
	const APawn* Pawn = Cast<APawn>(TransitioningPawn);
	if (!Pawn || !NewViewTarget)
	{
		return;
	}

	// Sem controller (teste headless, ou pawn ainda não possuído) não há o que
	// trocar — e isso não é erro, é um mundo sem jogador olhando.
	if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
	{
		PlayerController->SetViewTargetWithBlend(NewViewTarget, WorldBattleTransition::ViewBlendSeconds);
	}
}
