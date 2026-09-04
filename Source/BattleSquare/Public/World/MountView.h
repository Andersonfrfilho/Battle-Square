// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MountView.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/**
 * O PET MONTADO NA TELA (montaria-e-trilhas, MT5).
 *
 * Este projeto teve TRÊS atores nascerem sem malha atribuída e passarem em toda
 * bateria (o pet, o inimigo do mundo, o próprio jogador). Este ator nasce já
 * citando essa lição: a malha e a cor são atribuídas NO CONSTRUTOR, e o teste
 * cobra a ATRIBUIÇÃO — não só que o ator existe.
 *
 * É corpo procedural, como todo o resto do mundo (cilindro), com cor própria
 * para o montado se distinguir do personagem que o monta.
 */
UCLASS()
class BATTLESQUARE_API AMountView : public AActor
{
	GENERATED_BODY()

public:
	AMountView();

	/** O corpo do montado, para o teste cobrar malha atribuída. */
	UStaticMeshComponent* GetBody() const { return Body; }

	/** Tinge o montado com uma cor (padrão já vem no construtor). */
	void SetMountColor(const FLinearColor& Color);

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> MountRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;
};
