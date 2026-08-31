// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldDiscovery.h"
#include "Environment/IslandGeography.h"

namespace
{
	/**
	 * Vinte e cinco regiões do centro até a beira.
	 *
	 * Escolhido para dar EXATAMENTE as 800 unidades de hoje com o raio de
	 * 20.000 — a mesma disciplina dos anéis das peças da ilha.
	 */
	constexpr float RegioesAteABeira = 25.0f;
}

float FWorldDiscovery::RegionSizeUnits()
{
	return FMath::Max(1.0f, IslandGeography::LandRadiusUnits() / RegioesAteABeira);
}

int32 FWorldDiscovery::MarkSeenFrom(const FVector2D& WorldXY)
{
	const int32 Coluna = RegionColumnOf(WorldXY.X);
	const int32 Linha = RegionRowOf(WorldXY.Y);

	int32 Novas = 0;
	for (int32 DeltaColuna = -SightRadiusInRegions; DeltaColuna <= SightRadiusInRegions; ++DeltaColuna)
	{
		for (int32 DeltaLinha = -SightRadiusInRegions; DeltaLinha <= SightRadiusInRegions; ++DeltaLinha)
		{
			bool bJaEstava = false;
			VisitedRegions.Add(RegionKey(Coluna + DeltaColuna, Linha + DeltaLinha), &bJaEstava);
			if (!bJaEstava)
			{
				++Novas;
			}
		}
	}

	return Novas;
}

bool FWorldDiscovery::IsDiscovered(const FVector2D& WorldXY) const
{
	return VisitedRegions.Contains(
		RegionKey(RegionColumnOf(WorldXY.X), RegionRowOf(WorldXY.Y)));
}
