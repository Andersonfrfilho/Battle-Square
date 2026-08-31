// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "World/VillageLayout.h"
#include "Village.generated.h"

class UStaticMeshComponent;

/**
 * A vila em pé.
 *
 * Só CONSTRÓI: onde cada prédio fica é decisão de `VillageLayout`, que é puro e
 * testado. Aqui mora a parte que precisa de mundo — malha, material, colisão —
 * e que por isso não se verifica headless.
 *
 * Duas coisas são obrigatórias, e as duas por defeito que este projeto já
 * pagou:
 *
 * 1. **Toda malha é ATRIBUÍDA no construtor.** Componente criado sem asset
 *    passa em teste de lógica e não existe na tela — aconteceu três vezes, e
 *    hoje há sonda para isso.
 * 2. **Todo prédio BLOQUEIA.** O usuário jogou e relatou atravessar montanha e
 *    blocos. Cenário sólido sem colisão é a mesma família de defeito, e uma
 *    vila que se atravessa não é um lugar.
 */
UCLASS()
class BATTLESQUARE_API AVillage : public AActor
{
	GENERATED_BODY()

public:
	AVillage();

	/** Ergue a vila. Idempotente: chamar de novo refaz do zero. */
	void BuildVillage();

	/**
	 * Que assentamento este é. Decide o traçado, e por isso vem ANTES de
	 * `BuildVillage` — mudar depois de erguido não move prédio nenhum.
	 */
	void SetSettlementKind(ESettlementKind NewKind) { Kind = NewKind; }

	ESettlementKind GetSettlementKind() const { return Kind; }

	/** Quantos prédios existem em cena. */
	int32 GetBuiltCount() const { return BuiltMeshes.Num(); }

	const TArray<TObjectPtr<UStaticMeshComponent>>& GetBuiltMeshes() const
	{
		return BuiltMeshes;
	}

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	ESettlementKind Kind = ESettlementKind::VilaInicial;

	UPROPERTY()
	TObjectPtr<USceneComponent> VillageRoot;

	/** Um componente por prédio, na ordem do traçado. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BuiltMeshes;

	/** O telhado de cada prédio — o que faz a vila se ler de longe. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BuiltRoofs;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BoxMesh;
};
