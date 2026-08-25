// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BattleRandom.generated.h"

// PRNG de estado explícito. Nada de FMath::Rand ou FRandomStream global —
// ver AD-004/AD-016(BTL-18): a simulação usa exclusivamente este gerador,
// porque o estado precisa viajar dentro de FBattleState para ser
// serializado, replicado e reproduzido em replay.
//
// PCG32 (O'Neill, 2014): estado de 64 bits, saída de 32 bits, período 2^64.
USTRUCT()
struct BATTLESIM_API FBattleRandom
{
	GENERATED_BODY()

	UPROPERTY()
	uint64 State = 0;

	UPROPERTY()
	uint64 Increment = 1442695040888963407ULL;

	// Avança o estado e devolve o próximo uint32 da sequência.
	uint32 NextUInt32();

	// Inteiro em [Min, Max], inclusivo nos dois extremos, sem viés de
	// módulo (usa rejeição, não '%').
	int32 NextRange(int32 Min, int32 Max);
};
