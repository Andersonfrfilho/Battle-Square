// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EncounterDetectionComponent.generated.h"

class AWorldEncounterActor;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncounterTriggered, AWorldEncounterActor*);

/**
 * Cola entre o mundo e FEncounterDetector (DP-enc-02). A coleta dos atores do
 * mundo e a regra de disparo são métodos separados de propósito: a regra é
 * testável sem depender de quem está no UWorld.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BATTLESQUARE_API UEncounterDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEncounterDetectionComponent();

	FOnEncounterTriggered OnEncounterTriggered;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Avalia a lista dada contra a posição do Owner e dispara OnEncounterTriggered
	 * no máximo uma vez: o disparo desliga a detecção, e só a transição a religa
	 * (DP-enc-03). Devolve o ator disparado, ou nullptr.
	 */
	AWorldEncounterActor* EvaluateAndTrigger(const TArray<AWorldEncounterActor*>& EncounterActors);

	TArray<AWorldEncounterActor*> GatherEncounterActorsFromWorld() const;

	void SetDetectionEnabled(bool bEnabled) { bIsDetectionEnabled = bEnabled; }

	bool IsDetectionEnabled() const { return bIsDetectionEnabled; }

private:
	UPROPERTY(EditAnywhere, Category = "Encontro")
	bool bIsDetectionEnabled = true;
};
