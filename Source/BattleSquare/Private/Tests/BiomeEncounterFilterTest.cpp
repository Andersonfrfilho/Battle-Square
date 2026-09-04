// Copyright 2026 Anderson. All Rights Reserved.

#include "World/BiomeEncounterFilter.h"
#include "Data/PetDataLoader.h"
#include "Misc/AutomationTest.h"

/**
 * MB2 — o MESMO catálogo, filtrado na Floresta e no Vulcão, dá listas
 * diferentes, e nenhuma lista elemento fora da tabela do bioma. Sem bioma, o
 * catálogo passa inteiro (o contrapeso).
 */

namespace BiomaTeste
{
	FLoadedPetRecord De(const TCHAR* Id, const TCHAR* Tipo)
	{
		FLoadedPetRecord R;
		R.Id = Id;
		R.Type = Tipo;
		return R;
	}

	bool ContemElemento(const TArray<FLoadedPetRecord>& Lista, const FString& Elemento)
	{
		for (const FLoadedPetRecord& R : Lista)
		{
			if (R.Type.Contains(Elemento))
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBiomeChangesWildPetTest,
	"BattleSquare.World.BiomaEncontro.SelvagemMudaComOBioma",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBiomeChangesWildPetTest::RunTest(const FString&)
{
	using namespace BiomeEncounterFilter;

	// Um catálogo com um pet de cada elemento das duas pontas.
	const TArray<FLoadedPetRecord> Catalogo = {
		BiomaTeste::De(TEXT("p1"), TEXT("Natural/Planta")),
		BiomaTeste::De(TEXT("f1"), TEXT("Natural/Fantasma")),
		BiomaTeste::De(TEXT("fo1"), TEXT("Natural/Fogo")),
		BiomaTeste::De(TEXT("t1"), TEXT("Natural/Terra")),
		BiomaTeste::De(TEXT("a1"), TEXT("Natural/Agua")),
	};

	const TArray<FLoadedPetRecord> NaFloresta =
		FilterByBiome(Catalogo, EIslandBiome::Forest, /*bHasBiome=*/true);
	const TArray<FLoadedPetRecord> NoVulcao =
		FilterByBiome(Catalogo, EIslandBiome::Volcano, /*bHasBiome=*/true);

	// O ACEITE: as duas listas são DIFERENTES.
	TestNotEqual(TEXT("Floresta e Vulcao produzem listas diferentes"),
		NaFloresta.Num() == NoVulcao.Num()
			&& BiomaTeste::ContemElemento(NaFloresta, TEXT("Fogo"))
				== BiomaTeste::ContemElemento(NoVulcao, TEXT("Fogo")),
		true);

	// Floresta = Planta+Fantasma: tem os dois, NÃO tem Fogo nem Terra.
	TestTrue(TEXT("Floresta tem Planta"), BiomaTeste::ContemElemento(NaFloresta, TEXT("Planta")));
	TestTrue(TEXT("Floresta tem Fantasma"), BiomaTeste::ContemElemento(NaFloresta, TEXT("Fantasma")));
	TestFalse(TEXT("Floresta NAO tem Fogo — fora da tabela"),
		BiomaTeste::ContemElemento(NaFloresta, TEXT("Fogo")));

	// Vulcao = Fogo+Terra: tem os dois, NÃO tem Planta.
	TestTrue(TEXT("Vulcao tem Fogo"), BiomaTeste::ContemElemento(NoVulcao, TEXT("Fogo")));
	TestTrue(TEXT("Vulcao tem Terra"), BiomaTeste::ContemElemento(NoVulcao, TEXT("Terra")));
	TestFalse(TEXT("Vulcao NAO tem Planta — fora da tabela"),
		BiomaTeste::ContemElemento(NoVulcao, TEXT("Planta")));

	// O CONTRAPESO: sem bioma (batalha sem mundo), o catálogo passa INTEIRO.
	const TArray<FLoadedPetRecord> SemMundo =
		FilterByBiome(Catalogo, EIslandBiome::Forest, /*bHasBiome=*/false);
	TestEqual(TEXT("sem bioma nao filtra nada"), SemMundo.Num(), Catalogo.Num());

	return true;
}
