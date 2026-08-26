// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// T1 (tasks.md, Combate Online — DP-online-04): parâmetros de balanceamento
// de rede, nomeados — mesmo padrão de MaxTurns no núcleo (BattleOutcome.h).
// Chute fundamentado, não medição; só jogo real ajusta. Por isso ficam
// isolados aqui, num lugar só.
namespace BattleNetConstants
{
	// Tempo que um jogador tem para commitar antes do servidor preencher
	// o lado dele com 3x Aguardar (design.md, DP-online-04).
	constexpr int32 CommitTimeoutSeconds = 45;

	// Tempo sem reconexão antes do servidor declarar vitória por abandono
	// ao jogador presente (design.md, DP-online-03/NET-11).
	constexpr int32 AbandonTimeoutSeconds = 120;
}
