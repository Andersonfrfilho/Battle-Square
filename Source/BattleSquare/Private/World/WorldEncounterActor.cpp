// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldEncounterActor.h"
#include "Components/StaticMeshComponent.h"

AWorldEncounterActor::AWorldEncounterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EncounterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EncounterMesh"));
	SetRootComponent(EncounterMesh);
	EncounterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FEncounterCandidate AWorldEncounterActor::MakeEncounterCandidate() const
{
	FEncounterCandidate Candidate;
	Candidate.WorldLocation = GetActorLocation();
	Candidate.EncounterRadiusUnits = EncounterRadiusUnits;
	Candidate.CatalogId = CatalogId;
	Candidate.bIsResolved = bIsResolved;
	return Candidate;
}
