// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/IslandBakedPlan.h"
#include "RiverMesh.generated.h"

class UProceduralMeshComponent;

/**
 * A ÁGUA CORRENTE DA ILHA — os 137 cursos, assentados no relevo.
 *
 * Um ator para todos os cursos, e não um por curso. Cento e trinta e sete
 * atores custariam cento e trinta e sete transformações e a mesma tinta
 * repetida, para desenhar uma coisa só: a bacia. E a contagem, que é o que a
 * carta confere, ficaria espalhada por um `TActorIterator` em vez de sair de
 * um número que o ator sabe dizer.
 *
 * A largura vem PONTO A PONTO do assado, nunca recalculada aqui: o rio
 * engrossa da cabeceira para a foz, e refazer essa conta seria uma segunda
 * calha que concorda com a primeira até a primeira edição.
 */
UCLASS()
class BATTLESQUARE_API ARiverMesh : public AActor
{
	GENERATED_BODY()

public:
	ARiverMesh();

	/**
	 * Assenta os cursos do assado no relevo.
	 *
	 * Devolve QUANTOS cursos ergueu. A conta é o que a carta confere, e é
	 * também o que separa "a água subiu" de "o ator existe".
	 */
	int32 BuildFrom(const UIslandBakedPlan& Baked);

	UProceduralMeshComponent* GetWater() const { return Water; }

	/** Quantos cursos de fato viraram geometria. */
	int32 GetBuiltCourseCount() const { return BuiltCourseCount; }

	/**
	 * A meia-largura com que o curso dado foi ERGUIDO, no ponto dado.
	 *
	 * É o que a malha tem, não o que o plano diz — e é a diferença entre os
	 * dois que a conferência procura.
	 */
	float BuiltHalfWidthAt(int32 Course, int32 Sample) const;

	/**
	 * Quanto a lâmina d'água fica acima do chão.
	 *
	 * Não é enfeite: água exatamente na altura do terreno briga por
	 * profundidade com ele, e o resultado é uma faixa piscando — duas
	 * geometrias corretas, uma na frente da outra, com a bateria toda verde.
	 */
	static float SurfaceLiftUnits();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Water;

	UPROPERTY()
	int32 BuiltCourseCount = 0;

	/** As meias-larguras erguidas, curso a curso, na ordem das amostras. */
	UPROPERTY()
	TArray<float> BuiltHalfWidths;

	UPROPERTY()
	int32 SamplesPerCourse = 0;
};
