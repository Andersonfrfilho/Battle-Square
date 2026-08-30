// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldBoundaryWater.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

/**
 * A faixa de água que fecha o mundo.
 *
 * O limite precisa ser LEGÍVEL de longe. Uma parede invisível para o jogador
 * de repente e não explica; uma montanha explica mas vira obstáculo que se
 * contorna procurando a passagem que não existe. Água diz "o mundo acaba aqui"
 * de qualquer distância, e ninguém procura passagem no meio de um lago.
 *
 * A BARREIRA é invisível e a RAZÃO é visível — nessa ordem. O anel de colisão
 * fica na margem, então o jogador chega à beira, vê a água, e para. Ele nunca
 * bate numa parede no meio do nada.
 *
 * Isto também resolve por construção o que o guarda de queda conserta depois
 * do fato: quem não alcança a borda não cai dela.
 */
UCLASS(config = Game)
class BATTLESQUARE_API AWorldBoundaryWater : public AActor
{
	GENERATED_BODY()

public:
	AWorldBoundaryWater();

	/**
	 * Onde a terra acaba. A colisão fica aqui; a água começa um pouco antes,
	 * para haver margem molhada em vez de um corte seco.
	 */
	UPROPERTY(EditDefaultsOnly, config, Category = "Mundo")
	float ShoreRadiusUnits = 6000.0f;

	/** Até onde a água vai. Precisa passar do horizonte visível da câmera. */
	UPROPERTY(EditDefaultsOnly, config, Category = "Mundo")
	float WaterRadiusUnits = 26000.0f;

	/** Quanto a água assenta abaixo da terra. */
	UPROPERTY(EditDefaultsOnly, config, Category = "Mundo")
	float WaterDepthUnits = 40.0f;

	/**
	 * De quantos lados é o anel de colisão.
	 *
	 * A engine não traz cilindro OCO, então a barreira é um polígono de caixas.
	 * Vinte e quatro lados dão um erro de menos de 1% do raio — imperceptível
	 * andando, e barato: são 24 caixas, não 24 mil.
	 */
	UPROPERTY(EditDefaultsOnly, config, Category = "Mundo")
	int32 ShoreSegments = 24;

	UStaticMeshComponent* GetWaterMesh() const { return WaterMesh; }
	const TArray<TObjectPtr<UBoxComponent>>& GetShoreWalls() const { return ShoreWalls; }

	/** Monta a água e o anel. Chamado por quem sabe onde fica o centro. */
	void BuildBoundary();

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WaterMesh;

	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> ShoreWalls;
};
