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

	/**
	 * A casa BLOQUEADA não é parede lisa: é tronco caído ou pedra em pé, e o
	 * pet que anda contra ela faz uma de três coisas, nesta ordem.
	 *
	 * FORÇA derruba. Quem chega ao ataque efetivo do teto abaixo abre
	 * passagem para todo mundo — inclusive para o adversário. É por isso que
	 * derrubar não é obviamente bom: limpa o tabuleiro e entrega o terreno
	 * alto que o outro talvez fosse usar.
	 *
	 * VELOCIDADE sobe. Quem não derruba mas alcança a agilidade do teto
	 * escala e passa a lutar de cima (ElevatedAttackPercent). É a leitura
	 * literal do pedido: "se eles não tiver atributos ou poderes suficientes,
	 * subir neles".
	 *
	 * Quem não tem nem uma coisa nem outra esbarra, como antes.
	 *
	 * Os números saem do elenco semeado (pet-catalog.seed.ts): ataque de 40 a
	 * 75, velocidade de 35 a 80. 65 deixa derrubar com o quarto mais forte —
	 * raro o bastante para ser notícia; 55 deixa subir com pouco mais da
	 * metade — comum o bastante para ser tática, e não sorte de elenco.
	 * Ambos leem o valor EFETIVO, então um buff de ataque abre a passagem que
	 * o pet cru não abriria: o obstáculo responde à jogada, não só à ficha.
	 */
	constexpr int32 ObstacleBreakAttack = 65;
	constexpr int32 ObstacleClimbSpeed = 55;

	/**
	 * De cima para baixo bate mais forte — e só de cima para baixo: dois pets
	 * em obstáculos diferentes brigam de igual para igual, sem bônus para
	 * ninguém. Um multiplicador que valesse sempre que o atacante estivesse
	 * no alto premiaria subir mesmo contra quem já está lá.
	 *
	 * 130 é o degrau do meio entre o buff de casa (120) e a exposição no ar
	 * (150): mais que ocupar terreno bom, menos que pegar alguém sem chão.
	 */
	constexpr int32 ElevatedAttackPercent = 130;
}
