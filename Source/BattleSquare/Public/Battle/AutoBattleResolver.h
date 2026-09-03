// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleState.h"

/**
 * A BATALHA QUE SE RESOLVE SOZINHA — bot contra bot, no NÚCLEO de verdade.
 *
 * Existe pela defesa automática do posto (decisão 15, emendada): o líder que
 * saiu pode deixar a defesa "no automático", e o automático NÃO é um dado
 * inventado — é o `FBattleResolver` rodando a mesma simulação da tela, com a
 * mesma IA que já joga pelo oponente. O header do resolvedor prometia
 * "milhares de combates headless"; este é o primeiro cliente de produção.
 *
 * PURO e determinístico: mesmo estado e mesma semente, mesmo vencedor — a
 * defesa que aconteceu longe dos olhos tem de ser a MESMA que aconteceria de
 * novo, senão "automático" vira "sorte de quem reclama".
 */
namespace AutoBattleResolver
{
	/** 0 ou 1 = o lado que venceu; 0xFF = empate (ou o teto de turnos). */
	BATTLESQUARE_API uint8 ResolveBotVsBot(FBattleState State, uint64 BotSeed);
}
