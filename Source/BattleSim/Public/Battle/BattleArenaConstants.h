// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Arenas Variadas (design.md, DP-arena-03): parâmetros de balanceamento
// de propriedade de casa, nomeados — mesmo padrão de MinDamage/
// AttackDamageMultiplierPercent (BattlePhaseCombat.cpp).
namespace BattleArenaConstants
{
	// Dano fixo aplicado a quem permanece numa casa de dano ao fim de
	// cada slot (design.md, DP-arena-02).
	constexpr int32 CellDamageAmount = 5;

	// Percentual de fortalecimento de quem ocupa uma casa de buff —
	// aplicado ao Attack de quem ataca a partir dela, e ao Defense de
	// quem é alvo estando nela (design.md — buff é contextual, não dois
	// bônus separados).
	constexpr int32 CellBuffPercent = 120;
}
