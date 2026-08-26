// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DebugRouteMoverComponent.generated.h"

// T1 (tasks.md, Streaming de Mundo): move o Owner por uma rota fixa e
// determinística de waypoints — usado pelo pawn de debug (T4) para provar
// streaming de World Partition de forma reproduzível. Nunca depende de
// tempo real (FPlatformTime/GetWorld()) além do DeltaTime injetado pelo
// próprio TickComponent — mesma disciplina de tempo sempre injetado já
// usada em UBattleTurnCoordinator (design.md, Combate Online).
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BATTLESQUARE_API UDebugRouteMoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDebugRouteMoverComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rota")
	TArray<FVector> Waypoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rota")
	float SpeedUnitsPerSecond = 200.0f;

	// Distância abaixo da qual o waypoint atual é considerado alcançado e
	// o componente avança para o próximo — nunca zero, para não travar
	// em ponto flutuante nunca chegando exatamente ao alvo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rota")
	float WaypointReachedToleranceUnits = 10.0f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Índice do waypoint atual em Waypoints; igual a Waypoints.Num() quando
	// a rota terminou (parado no último ponto, sem loop).
	int32 GetCurrentWaypointIndex() const { return CurrentWaypointIndex; }

	bool HasFinishedRoute() const { return Waypoints.Num() == 0 || CurrentWaypointIndex >= Waypoints.Num(); }

private:
	int32 CurrentWaypointIndex = 0;
};
