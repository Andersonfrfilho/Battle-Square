// Copyright 2026 Anderson. All Rights Reserved.

#include "World/DebugRouteMoverComponent.h"

UDebugRouteMoverComponent::UDebugRouteMoverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDebugRouteMoverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (HasFinishedRoute())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector TargetLocation = Waypoints[CurrentWaypointIndex];
	const FVector ToTarget = TargetLocation - CurrentLocation;
	const float DistanceToTarget = ToTarget.Size();

	if (DistanceToTarget <= WaypointReachedToleranceUnits)
	{
		++CurrentWaypointIndex;
		return;
	}

	const FVector Direction = ToTarget / DistanceToTarget;
	const float StepDistance = FMath::Min(SpeedUnitsPerSecond * DeltaTime, DistanceToTarget);
	Owner->SetActorLocation(CurrentLocation + Direction * StepDistance);

	// O passo pode chegar exatamente (ou quase) ao alvo dentro do mesmo
	// tick — sem isto, um passo que cobre a distância inteira só seria
	// detectado como "chegou" no PRÓXIMO tick, atrasando a rota em um
	// tick por waypoint de forma cumulativa e não intuitiva.
	if (StepDistance >= DistanceToTarget - WaypointReachedToleranceUnits)
	{
		++CurrentWaypointIndex;
	}
}
