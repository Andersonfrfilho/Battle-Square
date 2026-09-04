// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A RECOMPENSA É PAGA PELO CRIMINOSO, NUNCA DO NADA (crime-e-recompensa, CR5).
 *
 * A recompensa anti-conluio: quem RESOLVE o crime de outro — devolve o pet
 * roubado ao dono — é pago às custas de QUEM ROUBOU, não de uma fonte infinita.
 * Sem isso, dois jogadores combinam "eu roubo, você devolve, os dois lucram".
 *
 * Duas amarras, e as duas testadas:
 *  - o débito sai da carteira do LADRÃO (compõe com a carteira, CI1); sem saldo,
 *    o governo cobre o rombo (decisão 28: a recompensa sempre existe), nunca se
 *    paga do nada em silêncio;
 *  - devolver o PRÓPRIO pet não paga recompensa (o contrapeso): a vítima se
 *    resolvendo não resolveu o crime de ninguém.
 */
namespace BountyReward
{
	struct BATTLESQUARE_API FRewardInput
	{
		/** Quem devolveu o pet e reivindica a recompensa. */
		FString ReturnerAccountId;

		/** A vítima do roubo — a quem o pet pertence. */
		FString VictimAccountId;

		/** O ladrão — de quem sai o débito. */
		FString ThiefAccountId;

		/** O saldo do ladrão, de onde a recompensa é debitada. */
		int32 ThiefBalance = 0;

		/** O valor da recompensa. */
		int32 RewardAmount = 0;
	};

	struct BATTLESQUARE_API FRewardResult
	{
		/** A recompensa é paga a alguém? Falso quando a vítima se resolve. */
		bool bRewardPaid = false;

		/** Quanto saiu do bolso do ladrão. */
		int32 FromThief = 0;

		/** Quanto o governo cobriu do rombo (decisão 28). */
		int32 FromGovernment = 0;

		/** Quanto o resolvedor recebe — a recompensa cheia quando paga. */
		int32 ToReturner = 0;
	};

	/** Resolve o pagamento da recompensa por devolver um pet roubado. */
	BATTLESQUARE_API FRewardResult Resolve(const FRewardInput& Input);
}
