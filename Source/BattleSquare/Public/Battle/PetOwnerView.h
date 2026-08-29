// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PetOwnerView.generated.h"

class UStaticMeshComponent;

/**
 * O treinador — quem é DONO do pet que está lutando.
 *
 * O domínio já falava de dono (treino, especialidade, coleção) e a tela nunca
 * mostrou nenhum: quem olhava a batalha via dois bichos brigando sozinhos num
 * campo vazio. Um lado só existe quando tem alguém do lado.
 *
 * Feito das primitivas da engine, como os pets: a peça precisa existir na tela
 * hoje, não depois que alguém autorar um personagem.
 */
UCLASS()
class BATTLESQUARE_API APetOwnerView : public AActor
{
	GENERATED_BODY()

public:
	APetOwnerView();

	/** 0 = esquerda, 1 = direita — o mesmo eixo de FPetState::Side. */
	void SetSide(uint8 InSide);

	uint8 GetSide() const { return Side; }

	UStaticMeshComponent* GetLegsMesh() const { return LegsMesh; }
	UStaticMeshComponent* GetTorsoMesh() const { return TorsoMesh; }
	UStaticMeshComponent* GetHeadMesh() const { return HeadMesh; }
	UStaticMeshComponent* GetHatMesh() const { return HatMesh; }

	/** Toda peça visível, para o teste cobrar malha ATRIBUÍDA em cada uma. */
	TArray<UStaticMeshComponent*> GetBodyParts() const;

	/** Altura total do boneco, para quem precisa posicioná-lo sem chutar. */
	static float GetStandingHeight();

	UPROPERTY(EditDefaultsOnly, Category = "Dono")
	FLinearColor LocalSideColor = FLinearColor(0.15f, 0.45f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category = "Dono")
	FLinearColor OpponentSideColor = FLinearColor(0.95f, 0.25f, 0.20f);

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> OwnerRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> LegsMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TorsoMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HatMesh;

	uint8 Side = 0;
};
