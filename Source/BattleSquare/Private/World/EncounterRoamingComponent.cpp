// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterRoamingComponent.h"

UEncounterRoamingComponent::UEncounterRoamingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEncounterRoamingComponent::ConfigureRoaming(const FVector& InHomeLocation, int32 Seed)
{
	HomeLocation = InHomeLocation;
	Random.Initialize(Seed);
	PickNewTarget();
}

void UEncounterRoamingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Sem configuração explícita, a casa é onde ele nasceu — assim um encontro
	// colocado à mão no nível também passeia, em vez de ficar parado sem
	// ninguém entender por quê.
	if (HomeLocation.IsZero() && GetOwner())
	{
		ConfigureRoaming(GetOwner()->GetActorLocation(),
			GetUniqueID() & 0x7FFFFFFF);
	}
}

void UEncounterRoamingComponent::PickNewTarget()
{
	// Ponto aleatório no disco ao redor de casa. A raiz do sorteio evita que
	// os alvos se concentrem no centro, que faria o passeio parecer nervoso.
	const float Angulo = Random.FRandRange(0.0f, 2.0f * PI);
	const float Distancia = RoamRadiusUnits * FMath::Sqrt(Random.FRand());

	RoamTarget = HomeLocation
		+ FVector(FMath::Cos(Angulo) * Distancia, FMath::Sin(Angulo) * Distancia, 0.0f);
}

void UEncounterRoamingComponent::TickComponent(float DeltaSeconds, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (PauseRemaining > 0.0f)
	{
		PauseRemaining -= DeltaSeconds;
		return;
	}

	const FVector Atual = Owner->GetActorLocation();
	FVector ParaOAlvo = RoamTarget - Atual;
	ParaOAlvo.Z = 0.0f;

	const float Distancia = ParaOAlvo.Size();
	const float Passo = RoamSpeedUnitsPerSecond * DeltaSeconds;

	if (Distancia <= Passo)
	{
		Owner->SetActorLocation(FVector(RoamTarget.X, RoamTarget.Y, Atual.Z));
		PauseRemaining = PauseSecondsOnArrival;
		PickNewTarget();
		return;
	}

	const FVector Direcao = ParaOAlvo / Distancia;
	Owner->SetActorLocation(Atual + Direcao * Passo);

	// Vira para onde anda: um pet que desliza de lado não parece andar.
	Owner->SetActorRotation(Direcao.Rotation());
}
