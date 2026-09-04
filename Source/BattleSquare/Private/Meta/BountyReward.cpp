// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/BountyReward.h"

BountyReward::FRewardResult BountyReward::Resolve(const FRewardInput& Entrada)
{
	FRewardResult Resultado;

	// O CONTRAPESO: devolver o PRÓPRIO pet não paga recompensa. A vítima se
	// resolvendo não resolveu o crime de outro — não há resolvedor a premiar.
	if (Entrada.ReturnerAccountId == Entrada.VictimAccountId)
	{
		return Resultado; // bRewardPaid = false, tudo zero.
	}

	const int32 Valor = FMath::Max(0, Entrada.RewardAmount);
	if (Valor == 0)
	{
		return Resultado;
	}

	Resultado.bRewardPaid = true;
	Resultado.ToReturner = Valor;

	// PAGA PELO LADRÃO: debita o que ele tem. Sem saldo cheio, o governo cobre
	// o rombo (decisão 28) — a recompensa sempre existe, mas nunca do nada em
	// silêncio: o que veio do governo fica explícito.
	const int32 DoLadrao = FMath::Clamp(Entrada.ThiefBalance, 0, Valor);
	Resultado.FromThief = DoLadrao;
	Resultado.FromGovernment = Valor - DoLadrao;
	return Resultado;
}
