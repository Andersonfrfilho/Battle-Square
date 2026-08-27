// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldEncounterActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AWorldEncounterActor::AWorldEncounterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EncounterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EncounterMesh"));
	SetRootComponent(EncounterMesh);
	EncounterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// MALHA ATRIBUÍDA AQUI, e não deixada para um Blueprint que ninguém criou.
	//
	// Sem isto o ator existe, anda, dispara batalha — e é INVISÍVEL. É o mesmo
	// defeito de APetView, que custou horas: a lógica passa em todos os testes
	// justamente porque nenhum deles olha a tela.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CorpoMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CorpoMesh.Succeeded())
	{
		EncounterMesh->SetStaticMesh(CorpoMesh.Object);

		// A esfera da engine tem origem no CENTRO: sem levantar pelo raio o
		// pet nasce meio enterrado no chão, que já aconteceu na arena.
		EncounterMesh->SetRelativeScale3D(FVector(BodyScale));
		EncounterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SphereRadiusUnits * BodyScale));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CorpoMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CorpoMaterial.Succeeded())
	{
		EncounterMesh->SetMaterial(0, CorpoMaterial.Object);
	}
}

void AWorldEncounterActor::BeginPlay()
{
	Super::BeginPlay();

	// Cor viva e distinta do cenário: um pet da cor do chão é tão invisível
	// quanto um sem malha.
	if (UMaterialInstanceDynamic* Material = EncounterMesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.35f, 0.05f));
	}
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
