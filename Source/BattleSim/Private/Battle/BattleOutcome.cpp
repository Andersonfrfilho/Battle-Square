// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleOutcome.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

namespace
{
	bool SideHasLivingPet(const FBattleState& State, uint8 Side)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side && Pet.IsAlive())
			{
				return true;
			}
		}
		return false;
	}

	struct FHealthTotals
	{
		int32 Health = 0;
		int32 MaxHealth = 0;
	};

	FHealthTotals SumHealthForSide(const FBattleState& State, uint8 Side)
	{
		FHealthTotals Totals;
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side)
			{
				Totals.Health += FMath::Max(0, Pet.Health); // não deixa vida negativa inflar a soma
				Totals.MaxHealth += Pet.MaxHealth;
			}
		}
		return Totals;
	}

	// Compara Left% vs Right% por multiplicação cruzada — evita qualquer
	// divisão e, portanto, qualquer resto/arredondamento (AD-004: só
	// inteiros). LeftHealth/LeftMax vs RightHealth/RightMax é equivalente
	// a LeftHealth*RightMax vs RightHealth*LeftMax quando os dois MaxHealth
	// são positivos.
	int32 CompareHealthPercent(const FHealthTotals& Left, const FHealthTotals& Right)
	{
		const int64 LeftCross = static_cast<int64>(Left.Health) * static_cast<int64>(Right.MaxHealth);
		const int64 RightCross = static_cast<int64>(Right.Health) * static_cast<int64>(Left.MaxHealth);
		if (LeftCross > RightCross) return -1; // Left está melhor
		if (LeftCross < RightCross) return 1;  // Right está melhor
		return 0; // empate exato
	}

	void EmitBattleEnded(TArray<FBattleEvent>& OutTrace, uint8 WinningSide)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::BatalhaEncerrada;
		Event.SlotIndex = 0;
		Event.Phase = 0;
		Event.ActorId = BattleEventNoActor;
		Event.TargetId = BattleEventNoActor;
		Event.Value = static_cast<int32>(WinningSide); // 0, 1, ou 0xFF (empate)
		OutTrace.Add(Event);
	}
}

void BattleOutcome::EvaluateOutcome(FBattleState& State, TArray<FBattleEvent>& OutTrace)
{
	// Idempotente (BTL-14): já encerrada, não reavalia nem reemite.
	if (State.bBattleEnded)
	{
		return;
	}

	const bool bLeftAlive = SideHasLivingPet(State, /*Side=*/0);
	const bool bRightAlive = SideHasLivingPet(State, /*Side=*/1);

	if (!bLeftAlive && !bRightAlive)
	{
		State.bBattleEnded = true;
		State.WinningSide = 0xFF;
		EmitBattleEnded(OutTrace, State.WinningSide);
		return;
	}

	if (!bLeftAlive)
	{
		State.bBattleEnded = true;
		State.WinningSide = 1;
		EmitBattleEnded(OutTrace, State.WinningSide);
		return;
	}

	if (!bRightAlive)
	{
		State.bBattleEnded = true;
		State.WinningSide = 0;
		EmitBattleEnded(OutTrace, State.WinningSide);
		return;
	}

	// Os dois lados seguem vivos: só termina se o limite de turnos foi
	// atingido (DP-05). Caso contrário, um novo turno de commit começa —
	// e isso não é responsabilidade desta função sinalizar; é a AUSÊNCIA
	// de bBattleEnded que sinaliza "continue".
	if (State.TurnNumber < MaxTurns)
	{
		return;
	}

	const FHealthTotals LeftTotals = SumHealthForSide(State, /*Side=*/0);
	const FHealthTotals RightTotals = SumHealthForSide(State, /*Side=*/1);
	const int32 Comparison = CompareHealthPercent(LeftTotals, RightTotals);

	State.bBattleEnded = true;
	if (Comparison < 0)
	{
		State.WinningSide = 0;
	}
	else if (Comparison > 0)
	{
		State.WinningSide = 1;
	}
	else
	{
		State.WinningSide = 0xFF; // empate exato de percentual
	}
	EmitBattleEnded(OutTrace, State.WinningSide);
}
