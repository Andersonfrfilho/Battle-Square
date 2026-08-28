// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"

// T6 (tasks.md, Arenas Variadas, ARENA-08/DP-arena-04): nomeação e carga
// de layouts de arena. BattleSim NUNCA importa Json — este catálogo
// vive inteiramente em BattleSquare e produz só o array plano
// (TArray<uint8> de ECellProperty empacotado) que o núcleo já consome
// via FBattleState::CellLayout. Mesmo padrão de FTypeEffectivenessTable
// (escala-pets-skills).
class BATTLESQUARE_API FArenaLayoutCatalog
{
public:
	// Falha explícita (false) se arquivo ausente ou malformado — nunca
	// catálogo vazio silencioso. Mesmo padrão de
	// FPetDataLoader::LoadVerifiedPets / FTypeEffectivenessTable::LoadFromJson.
	static bool LoadFromJson(const FString& FilePath, FArenaLayoutCatalog& OutCatalog);

	// False se o nome não existir — nunca crasha, nunca inventa layout.
	bool GetLayoutByName(const FString& ArenaName, TArray<uint8>& OutLayout) const;

	/**
	 * Nomes ORDENADOS, para escolha determinística.
	 *
	 * A ordem de um TMap não é estável entre execuções: escolher por índice
	 * sobre ela faria a mesma semente abrir arenas diferentes, e o
	 * determinismo do núcleo não valeria de nada acima dele.
	 */
	TArray<FString> GetSortedLayoutNames() const;

private:
	TMap<FString, TArray<uint8>> Layouts;
};
