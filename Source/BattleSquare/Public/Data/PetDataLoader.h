// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// T17 (tasks.md): lê o espelho local (produzido por apps/worker-pet-sync),
// reverifica a assinatura de CADA registro antes de aceitar — defesa em
// profundidade contra adulteração pós-sync (PETDB-10, design.md). Vive em
// BattleSquare, nunca em BattleSim (a fronteira do núcleo, AD-011/AD-012,
// continua intacta).
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
