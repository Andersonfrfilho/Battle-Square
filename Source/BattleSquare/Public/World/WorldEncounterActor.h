// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/EncounterDetector.h"
#include "WorldEncounterActor.generated.h"

class UStaticMeshComponent;

/**
 * Pet posicionado no mundo. É dado, não comportamento: sem Tick, sem overlap,
 * sem IA — a regra de disparo vive em FEncounterDetector (DP-enc-01/02).
 */
UCLASS()
class BATTLESQUARE_API AWorldEncounterActor : public AActor
{
	GENERATED_BODY()

public:
	AWorldEncounterActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encontro")
	FName CatalogId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encontro")
	float EncounterRadiusUnits = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encontro")
	bool bIsResolved = false;

	UPROPERTY(VisibleAnywhere, Category = "Encontro")
	TObjectPtr<UStaticMeshComponent> EncounterMesh;

	FEncounterCandidate MakeEncounterCandidate() const;

	void MarkResolved() { bIsResolved = true; }
};
