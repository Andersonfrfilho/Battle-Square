// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PetCollectionSaveGame.generated.h"

// T1 (tasks.md, Coleção e Captura, DP-colecao-01/03): coleção local do
// jogador. Save independente de conta (M7 não existe ainda) — cada
// instalação guarda a própria, sem sincronização.
USTRUCT()
struct FOwnedPetInstance
{
	GENERATED_BODY()

	// Identidade de captura: o id do REGISTRO DE CATÁLOGO
	// (FLoadedPetRecord::Id), nunca o Type — dois pets do mesmo tipo com
	// ids diferentes são capturas independentes (spec.md, edge case).
	UPROPERTY()
	FString CatalogId;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Type;

	// Gancho para Níveis, Experiência e Evolução (próxima feature) — só
	// existe aqui e é incrementado no caso P2 (vitória redundante);
	// nenhuma lógica de nível/evolução nesta feature.
	UPROPERTY()
	int32 Experience = 0;
};

UCLASS()
class BATTLESQUARE_API UPetCollectionSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FOwnedPetInstance> OwnedPets;
};
