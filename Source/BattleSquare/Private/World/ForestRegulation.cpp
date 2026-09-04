// Copyright 2026 Anderson. All Rights Reserved.

#include "World/ForestRegulation.h"

bool ForestRegulation::MayCutFreely(bool bIsPlantedGrove, bool bRegulationAllows)
{
	// Bosque plantado do assentamento: livre. Mata selvagem: so com permissao da
	// regulacao (Guarda Florestal) — nunca livre por omissao.
	if (bIsPlantedGrove)
	{
		return true;
	}
	return bRegulationAllows;
}
