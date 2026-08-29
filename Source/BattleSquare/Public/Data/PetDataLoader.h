// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// T17 (tasks.md): lê o espelho local (produzido por apps/worker-pet-sync),
// reverifica a assinatura de CADA registro antes de aceitar — defesa em
// profundidade contra adulteração pós-sync (PETDB-10, design.md). Vive em
// BattleSquare, nunca em BattleSim (a fronteira do núcleo, AD-011/AD-012,
// continua intacta).
/** Um golpe do pet: nome e poder, na ordem do slot (golpes-por-pet). */
struct FLoadedPetMove
{
	FString Name;
	int32 Power = 0;

	/** "none" | "water" | "damage" — o que o golpe deixa na casa que acertou. */
	FString TerrainEffect = TEXT("none");
};

struct FLoadedPetRecord
{
	FString Id;
	FString Name;
	FString Type;
	int32 Attack = 0;
	int32 Defense = 0;
	int32 Speed = 0;
	int32 MaxHealth = 0;
	FString UpdatedAt;

	/**
	 * Até quatro golpes, na ordem do slot.
	 *
	 * Chegam pelo espelho como JSON canônico, e são parte do payload ASSINADO
	 * (DP-golpe-03): golpe fora da assinatura seria o caminho óbvio para
	 * adulterar dano. Pet cadastrado antes dos golpes existirem chega com a
	 * lista vazia, e isso é assinatura VÁLIDA — não ausência de dado.
	 */
	TArray<FLoadedPetMove> Moves;

	/** O JSON exatamente como veio, para remontar o payload assinado. */
	FString MovesCanonicalJson;
};

class BATTLESQUARE_API FPetDataLoader
{
public:
	// PETDB-06: espelho vazio ou ausente é erro explícito, não lista
	// vazia silenciosa — quem chama decide se isso impede montar batalha.
	static bool LoadVerifiedPets(
		const FString& EncryptedMirrorPath,
		TConstArrayView<uint8> EncryptionKey32Bytes,
		const FString& Ed25519PublicKeyPem,
		TArray<FLoadedPetRecord>& OutPets,
		int32& OutRejectedCount);
};
