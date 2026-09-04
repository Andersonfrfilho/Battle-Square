// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/BiomeTint.h"
#include "World/Village.h"
#include "Misc/AutomationTest.h"

/**
 * MB3 — a vila veste o bioma: o MESMO prédio muda de cor mensuravelmente entre
 * os 5+ biomas, e nenhum par de biomas empata; e dois prédios diferentes NO
 * MESMO bioma continuam distintos (o contrapeso — a identidade sobrevive).
 */

namespace TintTeste
{
	const EIslandBiome TodosOsBiomas[] = {
		EIslandBiome::Forest, EIslandBiome::Desert, EIslandBiome::Glacier,
		EIslandBiome::Volcano, EIslandBiome::Beach, EIslandBiome::Swamp,
	};

	bool CoresIguais(const FLinearColor& A, const FLinearColor& B)
	{
		return A.Equals(B, 0.01f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillageWearsBiomeTest,
	"BattleSquare.Environment.BiomaVila.AVilaVesteOBioma",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageWearsBiomeTest::RunTest(const FString&)
{
	// O MESMO prédio (Mercado) muda de cor entre biomas, e nenhum par empata.
	for (int32 I = 0; I < UE_ARRAY_COUNT(TintTeste::TodosOsBiomas); ++I)
	{
		for (int32 J = I + 1; J < UE_ARRAY_COUNT(TintTeste::TodosOsBiomas); ++J)
		{
			const FLinearColor CorI = AVillage::BuildingColorInBiome(
				EVillageBuilding::Mercado, TintTeste::TodosOsBiomas[I]);
			const FLinearColor CorJ = AVillage::BuildingColorInBiome(
				EVillageBuilding::Mercado, TintTeste::TodosOsBiomas[J]);
			TestFalse(TEXT("nenhum par de biomas pinta o Mercado igual"),
				TintTeste::CoresIguais(CorI, CorJ));
		}
	}

	// O CONTRAPESO: dois prédios diferentes NO MESMO bioma continuam distintos —
	// a paleta por bioma não pode apagar a distinção que CorDoPredio dava.
	for (const EIslandBiome Bioma : TintTeste::TodosOsBiomas)
	{
		const FLinearColor Mercado = AVillage::BuildingColorInBiome(EVillageBuilding::Mercado, Bioma);
		const FLinearColor Academia = AVillage::BuildingColorInBiome(EVillageBuilding::Academia, Bioma);
		const FLinearColor Centro = AVillage::BuildingColorInBiome(EVillageBuilding::CentroDeRecuperacao, Bioma);
		TestFalse(TEXT("Mercado e Academia distintos no mesmo bioma"),
			TintTeste::CoresIguais(Mercado, Academia));
		TestFalse(TEXT("Mercado e Centro distintos no mesmo bioma"),
			TintTeste::CoresIguais(Mercado, Centro));
	}

	return true;
}
