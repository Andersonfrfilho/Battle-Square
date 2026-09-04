// Copyright 2026 Anderson. All Rights Reserved.

#include "World/BorderGate.h"

bool BorderGate::AllowsPassage(ESettlementKind Kind, bool bRegionRankingWon)
{
	// Só o Posto de Fronteira é amarrado ao ranking. Toda outra passagem é
	// livre — travar vila–vila faria a ilha uma prisão, que a spec não quer.
	if (Kind != ESettlementKind::PostoDeFronteira)
	{
		return true;
	}
	return bRegionRankingWon;
}
