// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "Battle/BattleState.h"
#include "Battle/BattleRandom.h"
#include "Battle/BattleActionSelection.h"

// T5 (tasks.md): oponente placeholder — só existe para tornar a feature
// demonstrável antes de M2 (rede/jogador real). "Burra" de propósito:
// escolhe ação válida ao acaso, sem estratégia. Descartável quando M2
// chegar.
class BATTLESQUARE_API FDumbOpponentAI
{
public:
	// Usa EXCLUSIVAMENTE o FBattleRandom recebido — nunca FMath::Rand
	// (AD-004). Nunca gera Mover para fora da grade, checando a partir
	// da posição do pet NO INÍCIO DO TURNO (não simula os 3 slots em
	// sequência — seria over-engineering para uma IA que só existe para
	// não travar a demonstração; o resolvedor real já trata bloqueio de
	// movimento na resolução de verdade).
	static FTurnCommit GenerateRandomValidCommit(const FBattleState& State, uint8 Side, FBattleRandom& Random);
};
