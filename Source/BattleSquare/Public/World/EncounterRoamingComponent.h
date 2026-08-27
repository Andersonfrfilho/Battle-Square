// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/RandomStream.h"
#include "EncounterRoamingComponent.generated.h"

/**
 * Faz um encontro ANDAR pelo mundo, em vez de esperar parado.
 *
 * Por que é componente e não código dentro de AWorldEncounterActor: aquele ator
 * é DADO (catálogo, raio, resolvido ou não), e a regra de disparo vive em
 * FEncounterDetector. Movimento é comportamento — misturá-lo ao dado faria o
 * ator carregar duas responsabilidades e ficar impossível de posicionar parado
 * quando isso for o desejado.
 *
 * Usa FRandomStream com semente PRÓPRIA, não FMath::Rand: dois encontros criados
 * no mesmo quadro precisam andar diferente, e um roteiro de verificação precisa
 * poder repetir o mesmo passeio.
 */
UCLASS(ClassGroup = (Battle), meta = (BlueprintSpawnableComponent))
class BATTLESQUARE_API UEncounterRoamingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEncounterRoamingComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Semente e centro do passeio. Chamado por quem cria o encontro. */
	void ConfigureRoaming(const FVector& InHomeLocation, int32 Seed);

	/** Alvo atual do passeio, para teste e para depuração. */
	const FVector& GetRoamTarget() const { return RoamTarget; }

	/** Quão longe de casa ele pode se afastar. */
	UPROPERTY(EditAnywhere, Category = "Passeio")
	float RoamRadiusUnits = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Passeio")
	float RoamSpeedUnitsPerSecond = 180.0f;

	/** Pausa ao chegar, para não parecer um trilho. */
	UPROPERTY(EditAnywhere, Category = "Passeio")
	float PauseSecondsOnArrival = 1.5f;

private:
	void PickNewTarget();

	FVector HomeLocation = FVector::ZeroVector;
	FVector RoamTarget = FVector::ZeroVector;
	float PauseRemaining = 0.0f;
	FRandomStream Random;
};
