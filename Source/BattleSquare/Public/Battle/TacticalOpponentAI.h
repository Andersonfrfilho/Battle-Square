// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "Battle/BattleState.h"
#include "Battle/BattleRandom.h"
#include "Battle/BattleActionSelection.h"

/**
 * Oponente com intenção: aproxima quando está longe, ataca quando alcança,
 * se protege quando está ferido.
 *
 * Por que substituir o sorteio uniforme: a narração ensina coisas como
 * "atacar quem está evasivo é desperdício" — e contra uma moeda não há o que
 * aprender, porque padrão não existe. Legibilidade sem oponente que decide
 * não tem troco.
 *
 * Por que FDumbOpponentAI CONTINUA existindo: ele é instrumento de medição do
 * BattleBalanceSimulator. Jogo aleatório é justamente o que isola efetividade
 * de tipo da tática de quem joga; trocá-lo por este faria os números de
 * balanceamento medirem outra coisa sem avisar.
 *
 * Continua valendo AD-004: só o FBattleRandom recebido, nunca FMath::Rand,
 * nunca relógio. Mesma semente, mesmo commit.
 */
class BATTLESQUARE_API FTacticalOpponentAI
{
public:
	static FTurnCommit GenerateCommit(const FBattleState& State, uint8 Side, FBattleRandom& Random);
};
