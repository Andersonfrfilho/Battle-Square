// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEvent;

// Vitória, derrota, empate — resolvido DEPOIS de FBattleResolver::ResolveTurn
// (T9), não dentro dele: T9 só resolve os 3 slots, esta função decide se a
// batalha terminou (design.md, BTL-14/BTL-15). Separação deliberada: T9
// não sabe de condição de vitória, e nunca vai precisar saber.
namespace BattleOutcome
{
	// DP-05 (spec.md): limite de turnos até desempate por percentual de vida.
	inline constexpr int32 MaxTurns = 10;

	// Avalia o estado após um turno resolvido. Marca State.bBattleEnded
	// quando aplicável e emite exatamente um evento BatalhaEncerrada.
	// Idempotente: se bBattleEnded já era true, não faz nada (BTL-14:
	// "emite BatalhaEncerrada uma única vez").
	void EvaluateOutcome(FBattleState& State, TArray<FBattleEvent>& OutTrace);
}
