// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldFinding.h"

#include "Battle/DeterministicSpread.h"

bool WorldFinding::HasFinding(EGroundUse Use)
{
	// Só os destinos fora da trilha (decisão 57). O achado paga por SAIR do
	// caminho — trilha, vila e fazenda nao guardam nada.
	switch (Use)
	{
	case EGroundUse::Ruina:
	case EGroundUse::ClareiraFechada:
	case EGroundUse::MercadoNegro:
		return true;
	default:
		return false;
	}
}

int32 WorldFinding::AmountAt(uint32 PlaceSeed, const FFindingConfig& Config)
{
	const int32 Min = Config.MinAmount;
	const int32 Max = Config.MaxAmount;
	if (Min >= Max)
	{
		// Config invertida ou degenerada: devolve o minimo, nunca um intervalo
		// doido. (Min == Max tambem cai aqui e devolve o valor fixo.)
		return Min;
	}
	// Deterministico da semente do LUGAR (regra 5): o mesmo canto sempre o mesmo
	// achado. BattleSpread::Below da [0, N).
	return Min + BattleSpread::Below(PlaceSeed, 0, Max - Min + 1);
}
