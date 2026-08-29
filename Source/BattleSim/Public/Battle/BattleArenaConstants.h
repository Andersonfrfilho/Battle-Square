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

	/**
	 * Esquiva por reflexo (DP-atr-07): chance em porcentagem = Reflexes / 4,
	 * com TETO. O teto é a amarra que impede o número de decidir mais que a
	 * decisão — sem ele, atributo alto viraria imunidade e o commit às cegas
	 * já traz incerteza suficiente.
	 */
	constexpr int32 ReflexDodgeDivisor = 4;
	constexpr int32 ReflexDodgeMaxPercent = 25;

	/**
	 * Variação do dano, em porcentagem para cada lado. A agressividade
	 * ESTREITA a faixa; nunca aumenta o dano médio.
	 *
	 * O agressivo bate no que promete, o cauteloso bate irregular — e a troca
	 * é essa, não "mais forte". Um bônus de dano disfarçado de constância
	 * faria a personalidade ser só uma segunda musculatura.
	 */
	constexpr int32 DamageVarianceBasePercent = 20;
	constexpr int32 DamageVarianceFloorPercent = 5;
	constexpr int32 AggressionPerVariancePoint = 2;
}
