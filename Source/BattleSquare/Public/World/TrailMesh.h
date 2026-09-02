// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/IslandBakedPlan.h"
#include "TrailMesh.generated.h"

class UProceduralMeshComponent;

/**
 * AS TRILHAS DA ILHA — as 23 rotas, assentadas no relevo.
 *
 * Um ator para todas, pelo mesmo motivo da água: elas são uma coisa só, a rede
 * de caminhos, e a contagem que a carta confere tem de sair de um número que o
 * ator sabe dizer — não de varrer o mundo atrás de atores.
 *
 * ## A altura da trilha e a altura da malha não são a mesma pergunta
 *
 * O assado guarda a altura de cada ponto de trilha, vinda de `GroundHeightAt`.
 * Ela serve para MEDIR o declive, e tem de ser essa: reamostrar a malha daria o
 * declive da MALHA, que anda 1.555 unidades por casa contra os 2.520 do
 * barranco.
 *
 * Mas para DESENHAR, o que vale é a altura que a superfície de fato tem —
 * `HeightAt`, interpolada na grade. Assentar a trilha na altura do plano a
 * deixaria flutuando ou enterrada exatamente onde a grade arredonda, e nenhuma
 * medição de declive notaria.
 */
UCLASS()
class BATTLESQUARE_API ATrailMesh : public AActor
{
	GENERATED_BODY()

public:
	ATrailMesh();

	/** Assenta as trilhas do assado. Devolve quantas ergueu. */
	int32 BuildFrom(const UIslandBakedPlan& Baked);

	UProceduralMeshComponent* GetTrail() const { return Trail; }

	int32 GetBuiltTrailCount() const { return BuiltTrailCount; }

	/**
	 * A altura com que o ponto dado da trilha dada foi ERGUIDO.
	 *
	 * É o que a malha tem. A conferência de "flutuando ou enterrado" compara
	 * isto com o chão, e comparar plano com plano não provaria nada.
	 */
	float BuiltHeightAt(int32 Trail, int32 Point) const;

	/** Quanto a trilha fica acima do chão, para não brigar por profundidade. */
	static float SurfaceLiftUnits();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Trail;

	UPROPERTY()
	int32 BuiltTrailCount = 0;

	/** As alturas erguidas, achatadas: uma faixa por trilha, na ordem dos pontos. */
	UPROPERTY()
	TArray<float> BuiltHeights;

	UPROPERTY()
	TArray<int32> FirstPointOfTrail;
};
