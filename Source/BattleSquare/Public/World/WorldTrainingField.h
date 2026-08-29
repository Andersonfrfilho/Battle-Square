// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldTrainingField.generated.h"

class UStaticMeshComponent;

/**
 * Um lugar no mundo que treina UM atributo enquanto o jogador está nele.
 *
 * É o que dá DESTINO ao mundo aberto. Até aqui ele só servia para encontrar
 * inimigos, e caminhar até um lugar por um motivo é o que separa mapa de
 * corredor.
 *
 * O tempo que conta é o de PERMANÊNCIA COM O JOGO ABERTO, não o do relógio do
 * sistema — ver o comentário de ABattleSquareGameMode::TickTrainingFields.
 */
UCLASS()
class BATTLESQUARE_API AWorldTrainingField : public AActor
{
	GENERATED_BODY()

public:
	AWorldTrainingField();

	/**
	 * Qual atributo este campo treina, no MESMO vocabulário do requisito de
	 * golpe ("musculature", "flight", …).
	 *
	 * Uma segunda lista de nomes produziria um campo que treina algo que
	 * nenhum golpe exige — treino que nunca destrava nada, e sem nada
	 * apontando a causa.
	 */
	UPROPERTY(EditAnywhere, Category = "Treino")
	FString TrainedAttribute = TEXT("musculature");

	/** Raio em que a permanência conta. */
	UPROPERTY(EditAnywhere, Category = "Treino")
	float FieldRadiusUnits = 400.0f;

	UStaticMeshComponent* GetFieldMesh() const { return FieldMesh; }

	/** Cor do campo, escolhida pelo atributo — o mesmo atributo, a mesma cor. */
	static FLinearColor ColorForAttribute(const FString& Attribute);

	bool IsInside(const FVector& WorldLocation) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FieldMesh;
};
