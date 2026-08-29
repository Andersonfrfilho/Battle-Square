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

	/**
	 * Força bruta. Sobe com DANO CAUSADO, não com ataque desferido — atacar o
	 * vazio não fortalece ninguém (DP-atr-03).
	 */
	UPROPERTY()
	int32 Musculature = 0;

	/**
	 * Temperamento, num EIXO: negativo é cauteloso, positivo é agressivo.
	 *
	 * Não sobe, INCLINA. É o que permite golpe que exige um lado em vez de um
	 * mínimo — e um pet não pode ter os dois, o que faz personalidade virar
	 * decisão em vez de mais um número para maximizar.
	 */
	UPROPERTY()
	int32 Personality = 0;

	/**
	 * Proficiência POR SKILL, na ordem camuflagem, voo, subsolo.
	 *
	 * Separadas de propósito: voar muito destrava golpe aéreo, e não golpe de
	 * camuflagem — é o que torna o caminho do jogador legível no pet dele.
	 *
	 * Sobe com uso EFETIVO: a postura assumida só conta se o pet não tomou
	 * dano naquele slot. Contar toda postura recompensaria camuflar contra um
	 * bot parado.
	 */
	UPROPERTY()
	int32 SkillProficiency[3] = { 0, 0, 0 };
};

UCLASS()
class BATTLESQUARE_API UPetCollectionSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FOwnedPetInstance> OwnedPets;
};
