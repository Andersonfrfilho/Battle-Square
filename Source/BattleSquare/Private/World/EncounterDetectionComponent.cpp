// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterDetectionComponent.h"
#include "World/EncounterDetector.h"
#include "World/WorldEncounterActor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UEncounterDetectionComponent::UEncounterDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEncounterDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsDetectionEnabled)
	{
		return;
	}

	EvaluateAndTrigger(GatherEncounterActorsFromWorld());
}

TArray<AWorldEncounterActor*> UEncounterDetectionComponent::GatherEncounterActorsFromWorld() const
{
	TArray<AWorldEncounterActor*> EncounterActors;

	UWorld* World = GetWorld();
	if (!World)
	{
		return EncounterActors;
	}

	for (TActorIterator<AWorldEncounterActor> Iterator(World); Iterator; ++Iterator)
	{
		EncounterActors.Add(*Iterator);
	}

	return EncounterActors;
}

AWorldEncounterActor* UEncounterDetectionComponent::EvaluateAndTrigger(const TArray<AWorldEncounterActor*>& EncounterActors)
{
	if (!bIsDetectionEnabled)
	{
		return nullptr;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Os nulos saem antes de virarem candidatos: um FEncounterCandidate default
	// tem raio 0 na origem, e um pawn na origem cairia "dentro" dele.
	TArray<AWorldEncounterActor*> ValidEncounterActors;
	ValidEncounterActors.Reserve(EncounterActors.Num());
	for (AWorldEncounterActor* EncounterActor : EncounterActors)
	{
		if (EncounterActor)
		{
			ValidEncounterActors.Add(EncounterActor);
		}
	}

	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = Owner->GetActorLocation();
	Params.Candidates.Reserve(ValidEncounterActors.Num());
	for (const AWorldEncounterActor* EncounterActor : ValidEncounterActors)
	{
		Params.Candidates.Add(EncounterActor->MakeEncounterCandidate());
	}

	const int32 TriggeredIndex = FEncounterDetector::FindTriggeredEncounter(Params);
	if (TriggeredIndex == INDEX_NONE)
	{
		return nullptr;
	}

	// Desligar ANTES de anunciar: o ouvinte é a transição, que só religa no fim
	// da batalha. Anunciar primeiro deixaria uma janela para um segundo disparo.
	bIsDetectionEnabled = false;

	AWorldEncounterActor* TriggeredActor = ValidEncounterActors[TriggeredIndex];
	OnEncounterTriggered.Broadcast(TriggeredActor);
	return TriggeredActor;
}
