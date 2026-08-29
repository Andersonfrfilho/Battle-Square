// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

// T2–T3 (tasks.md, Coleção e Captura): captura e persistência da
// coleção local. CaptureIfNewInMemory é lógica pura (T2, headless-
// testável sem tocar UGameplayStatics/disco); CaptureIfNew/LoadCollection
// são o caminho real (T3), mesmo padrão de Limite de Ferramenta do
// resto do projeto — headless primeiro, I/O real por último.
class BATTLESQUARE_API FPetCollectionService
{
public:
	// T2: retorna true se uma instância NOVA foi adicionada a SaveGame
	// (captura real); false se o CatalogId já estava presente — nunca
	// duplica. Muta SaveGame diretamente; quem chama decide se persiste.
	static bool CaptureIfNewInMemory(UPetCollectionSaveGame* SaveGame, const FOwnedPetInstance& Instance);

	// T3: caminho real — carrega o slot (ou começa vazio se ausente),
	// aplica CaptureIfNewInMemory, salva de volta. Save corrompido/
	// ilegível é tratado como coleção vazia, nunca crash (mesmo padrão
	// de FPetDataLoader::LoadVerifiedPets).
	static bool CaptureIfNew(const FString& SlotName, const FOwnedPetInstance& Instance);

	// T3: só leitura — coleção vazia se o slot nunca foi usado ou está
	// corrompido.
	static TArray<FOwnedPetInstance> LoadCollection(const FString& SlotName);

	// T4 (niveis-experiencia-evolucao): persiste uma coleção já
	// modificada por inteiro (ex.: XP concedido a uma instância
	// existente) — diferente de CaptureIfNew, que só adiciona.
	static void SaveCollection(const FString& SlotName, const TArray<FOwnedPetInstance>& Collection);

	/**
	 * O perfil do treinador, no MESMO slot da coleção.
	 *
	 * As duas gravações preservam a outra metade: montar o save do zero e
	 * gravar por cima é o que faria salvar experiência apagar as
	 * especialidades — uma escolha que não se refaz, sumindo sem aviso.
	 */
	static FTrainerProfile LoadTrainerProfile(const FString& SlotName);

	static void SaveTrainerProfile(const FString& SlotName, const FTrainerProfile& Profile);
};
