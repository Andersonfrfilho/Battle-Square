// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/BattleBalanceSimulator.h"
#include "Battle/BattleResolver.h"
#include "Battle/DumbOpponentAI.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleOutcome.h"

namespace
{
	// Teto de segurança — o núcleo já tem seu próprio MaxTurns
	// (BattleOutcome.h) que sempre encerra a batalha antes disso; este
	// valor só existe para nunca travar a ferramenta se algo externo
	// mudar essa garantia.
	constexpr int32 SafetyTurnCap = 10000;

	int32 SumDamageInTrace(const TArray<FBattleEvent>& Trace)
	{
		int32 TotalDamage = 0;
		for (const FBattleEvent& Event : Trace)
		{
			if (Event.Type == EBattleEventType::DanoAplicado)
			{
				TotalDamage += Event.Value;
			}
		}
		return TotalDamage;
	}
}

FBattleBalanceResult FBattleBalanceSimulator::RunBatchSimulation(
	const FPetState& LeftTemplate,
	const FPetState& RightTemplate,
	int32 NumSimulations,
	uint64 BaseSeed)
{
	FBattleBalanceResult Result;

	int64 TotalTurns = 0;
	int64 TotalDamage = 0;
	int64 TotalTurnsResolved = 0;

	for (int32 SimIndex = 0; SimIndex < NumSimulations; ++SimIndex)
	{
		FBattleState State;
		State.Pets.Add(LeftTemplate);
		State.Pets.Add(RightTemplate);
		State.Random.State = BaseSeed + static_cast<uint64>(SimIndex);

		int32 TurnGuard = 0;
		while (!State.bBattleEnded && TurnGuard < SafetyTurnCap)
		{
			++TurnGuard;

			const FTurnCommit LeftCommit = FDumbOpponentAI::GenerateRandomValidCommit(State, /*Side=*/0, State.Random);
			const FTurnCommit RightCommit = FDumbOpponentAI::GenerateRandomValidCommit(State, /*Side=*/1, State.Random);

			FBattleResolveResult TurnResult = FBattleResolver::ResolveTurn(State,
				FBattleResolver::DuelCommits(State, LeftCommit, RightCommit));
			BattleOutcome::EvaluateOutcome(TurnResult.NextState, TurnResult.Trace);
			State = TurnResult.NextState;

			TotalDamage += SumDamageInTrace(TurnResult.Trace);
			++TotalTurnsResolved;
		}

		TotalTurns += TurnGuard;

		if (State.bBattleEnded)
		{
			if (State.WinningSide == 0) { ++Result.LeftWins; }
			else if (State.WinningSide == 1) { ++Result.RightWins; }
			else { ++Result.Draws; }
		}
		else
		{
			// Teto de segurança estourado sem a batalha terminar — trata
			// como empate para não distorcer o agregado, mas isto nunca
			// deveria acontecer com MaxTurns do núcleo intacto.
			++Result.Draws;
		}
	}

	if (NumSimulations > 0)
	{
		Result.AverageTurns = static_cast<double>(TotalTurns) / static_cast<double>(NumSimulations);
	}
	if (TotalTurnsResolved > 0)
	{
		Result.AverageDamagePerTurn = static_cast<double>(TotalDamage) / static_cast<double>(TotalTurnsResolved);
	}

	return Result;
}
