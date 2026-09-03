// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * O RANKING do treinador — pura sobre o perfil, como a carteira e as
 * especialidades.
 *
 * A regra inteira cabe numa frase: vitória de Arena soma, e NADA subtrai. O
 * desconto por derrota é a metade irreversível (tirar ponto que alguém já
 * contou), e o dono do mundo decidiu que viajar é liberdade — o ranking é
 * placar, não cancela (decisão 58).
 */
class BATTLESQUARE_API FTrainerRankingRules
{
public:
	/** Quanto vale UMA vitória. Constante nomeada, nunca literal no call site. */
	static constexpr int32 PointsPerArenaVictory = 1;

	static void AwardArenaVictory(FTrainerProfile& Profile);
};
