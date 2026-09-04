// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/ArrestPenalty.h"

ArrestPenalty::FArrestResult ArrestPenalty::Apply(const FArrestInput& Input)
{
	FArrestResult Resultado;
	Resultado.bPetConfiscated = true; // 27: confisca sempre.

	const bool bPodePagar = Input.SuspectBalance >= Input.FineAmount;

	if (bPodePagar)
	{
		// Paga a multa e cumpre o tempo-base.
		Resultado.FinePaid = Input.FineAmount;
		Resultado.LockTurns = Input.BaseLockTurns;
	}
	else
	{
		// 28: sem saldo para a multa, DOBRA o tempo. Não paga nada — o governo
		// cobre o ressarcimento.
		Resultado.FinePaid = 0;
		Resultado.LockTurns = Input.BaseLockTurns * 2;
	}

	// 27+28: o roubado recebe SEMPRE o que perdeu. Se a multa paga não cobre,
	// o governo cobre a diferença — a recompensa nunca falta.
	Resultado.ToVictim = Input.VictimLoss;
	Resultado.bGovernmentCovered = Resultado.FinePaid < Input.VictimLoss;

	return Resultado;
}
