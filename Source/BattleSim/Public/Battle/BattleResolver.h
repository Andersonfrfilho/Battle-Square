// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

// Resultado de resolver um turno inteiro: o estado seguinte e o trace
// completo dos 3 slots. Ver design.md — é o par (estado, trace) que
// atravessa a fronteira do núcleo.
struct FBattleResolveResult
{
	FBattleState NextState;
	TArray<FBattleEvent> Trace;
};

// O coração do núcleo de simulação. FUNÇÃO ESTÁTICA E PURA — sem membro,
// sem singleton, sem estado interno (design.md, T9 em tasks.md). É o que
// torna o determinismo (BTL-16) testável em vez de presumido, e o que
// permite rodar milhares de combates headless para balanceamento.
class BATTLESIM_API FBattleResolver
{
public:
	static FBattleResolveResult ResolveTurn(
		const FBattleState& InState,
		const FTurnCommit& LeftCommit,
		const FTurnCommit& RightCommit);
};
