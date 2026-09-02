// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/IslandBakedPlan.h"
#include "AqueductMesh.generated.h"

class UProceduralMeshComponent;

/**
 * OS AQUEDUTOS — a obra que leva água à vila que não tem nenhuma perto.
 *
 * Ele DESCE do começo ao fim, e é a queda que faz a água andar. Um aqueduto
 * horizontal é um cano mágico: some a amarra que torna a obra interessante, que
 * é ela ter de contornar o terreno para nunca subir.
 */
UCLASS()
class BATTLESQUARE_API AAqueductMesh : public AActor
{
	GENERATED_BODY()

public:
	AAqueductMesh();

	int32 BuildFrom(const UIslandBakedPlan& Baked);

	UProceduralMeshComponent* GetChannel() const { return Channel; }

	int32 GetBuiltAqueductCount() const { return BuiltAqueductCount; }

	/** A altura da calha do aqueduto dado, no ponto dado. */
	float BuiltHeightAt(int32 Aqueduct, int32 Point) const;

	/** Quanto a calha passa acima do chão mais alto do percurso. */
	static float ClearanceUnits();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Channel;

	UPROPERTY()
	int32 BuiltAqueductCount = 0;

	UPROPERTY()
	TArray<float> BuiltHeights;

	UPROPERTY()
	TArray<int32> FirstPointOf;
};
