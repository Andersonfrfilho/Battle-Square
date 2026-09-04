// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A PENA DA PRISÃO (crime-e-recompensa, CR9 — decisões 27 e 28).
 *
 * 27: a prisão CONFISCA o pet, TRANCA por um tempo e MULTA; parte do dinheiro
 *     vai para o roubado, no valor do que ele perdeu.
 * 28: sem saldo para a multa, o tempo preso DOBRA — e a recompensa ao roubado
 *     existe sempre, porque quem paga é o GOVERNO quando o preso não paga.
 *
 * A invariante que amarra as duas: o roubado é RESSARCIDO SEMPRE, no valor que
 * perdeu, quer o preso tenha saldo quer não. Regra pura para o teste conseguir
 * afirmar essa garantia sem depender de saldo nenhum.
 */
namespace ArrestPenalty
{
	struct BATTLESQUARE_API FArrestInput
	{
		/** O saldo que o preso tem para pagar a multa. */
		int32 SuspectBalance = 0;

		/** O valor da multa. */
		int32 FineAmount = 0;

		/** O tempo-base de prisão, em turnos. */
		int32 BaseLockTurns = 0;

		/** O que o roubado perdeu — o valor que ele tem a receber de volta. */
		int32 VictimLoss = 0;
	};

	struct BATTLESQUARE_API FArrestResult
	{
		/** O pet é sempre confiscado na prisão. */
		bool bPetConfiscated = true;

		/** O tempo preso — dobrado quando não há saldo para a multa. */
		int32 LockTurns = 0;

		/** Quanto o preso efetivamente pagou de multa. */
		int32 FinePaid = 0;

		/** Quanto o roubado recebe — sempre igual ao que perdeu. */
		int32 ToVictim = 0;

		/** O governo cobriu a diferença porque o preso não pagou tudo? */
		bool bGovernmentCovered = false;
	};

	/** Aplica as decisões 27 e 28 sobre uma prisão. */
	BATTLESQUARE_API FArrestResult Apply(const FArrestInput& Input);
}
