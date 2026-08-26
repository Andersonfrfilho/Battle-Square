// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleState.h"

// T4–T5 (tasks.md, Escala de Pets e Skills): ferramenta de medição —
// roda N combates completos entre duas composições fixas, agrega
// vitórias/turnos/dano. Não decide balanceamento, só mede (spec.md,
// Out of Scope: "balanceamento automático").
struct BATTLESQUARE_API FBattleBalanceResult
{
	int32 LeftWins = 0;
	int32 RightWins = 0;
	int32 Draws = 0;
	double AverageTurns = 0.0;
	double AverageDamagePerTurn = 0.0;
};

class BATTLESQUARE_API FBattleBalanceSimulator
{
public:
	// Cada simulação usa FBattleRandom semeado com BaseSeed + Index —
	// determinístico e reproduzível (AD-004: nunca FMath::Rand). Os dois
	// lados são jogados por FDumbOpponentAI (não há jogador humano numa
	// simulação de balanceamento). Roda até FBattleState::bBattleEnded
	// ou o limite de turnos do núcleo (BattleOutcome::MaxTurns).
	static FBattleBalanceResult RunBatchSimulation(
		const FPetState& LeftTemplate,
		const FPetState& RightTemplate,
		int32 NumSimulations,
		uint64 BaseSeed);
};
