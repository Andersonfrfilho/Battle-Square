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
// Adiantada no ESCOPO GLOBAL — `struct F...*` no parâmetro dentro do
// namespace declararia um `AutoBattleResolver::FTrainerProfile` novo,
// incompleto para sempre. Terceira vez que este erro aparece hoje (enum sem
// base, a tabela de efetividade, e agora este): a declaração adiantada dentro
// de namespace é armadilha de C++, não descuido de quem escreveu.
struct FTrainerProfile;

namespace AutoBattleResolver
{
	/**
	 * 0 ou 1 = o lado que venceu; 0xFF = empate (ou o teto de turnos).
	 *
	 * `Side0Profile` é o perfil de quem a I.A. SIMULA no lado 0 (decisão 15-d:
	 * "a I.A. batalha por você baseando nos seus movimentos"): os tipos de
	 * ação saem da TENDÊNCIA dele, e a ordem sai das SEQUÊNCIAS — quem abre
	 * defendendo e fecha atacando é simulado assim. Nulo, o lado 0 joga
	 * uniforme — o bot de
	 * sempre. O lado 1 (o desafiante) é sempre o bot uniforme: NPC não tem
	 * histórico para simular, e fingir um seria estilo inventado.
	 */
	BATTLESQUARE_API uint8 ResolveBotVsBot(FBattleState State, uint64 BotSeed,
		const FTrainerProfile* Side0Profile = nullptr);
}
