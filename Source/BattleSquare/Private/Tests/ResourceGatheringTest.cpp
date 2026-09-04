// Copyright 2026 Anderson. All Rights Reserved.

#include "World/ResourceCatalog.h"
#include "World/ResourceGathering.h"
#include "World/ForestRegulation.h"
#include "Misc/AutomationTest.h"

/**
 * MV7 — a colheita: a ferramenta decide o que se colhe (68-b), o pet aumenta o
 * rendimento sem ser requisito (68-c), e a mata selvagem nunca e corte livre por
 * omissao (68-d). Os contrapesos no centro.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResourceGatheringTest,
	"BattleSquare.World.Recursos.FerramentaEPetDecidem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResourceGatheringTest::RunTest(const FString&)
{
	using namespace ResourceGathering;

	// A FERRAMENTA DECIDE (68-b): madeira exige machado. Sem ele, rende ZERO —
	// o recurso aparece mas nao vem.
	FGatherContext MaoVazia; MaoVazia.Tool = EGatherTool::Nenhuma;
	TestEqual(TEXT("madeira sem machado nao rende"),
		Yield(EWorldResource::Madeira, MaoVazia), 0);

	FGatherContext ComMachado; ComMachado.Tool = EGatherTool::Machado;
	TestTrue(TEXT("madeira com machado rende"),
		Yield(EWorldResource::Madeira, ComMachado) > 0);

	// Ferramenta ERRADA tambem nao rende: picareta nao corta arvore.
	FGatherContext ComPicareta; ComPicareta.Tool = EGatherTool::Picareta;
	TestEqual(TEXT("madeira com picareta nao rende"),
		Yield(EWorldResource::Madeira, ComPicareta), 0);

	// A MAO BASTA para o facil: flor rende de mao vazia.
	TestTrue(TEXT("flor rende de mao vazia"),
		Yield(EWorldResource::Flor, MaoVazia) > 0);

	// O PET AUMENTA (68-c), nunca e requisito: com pet rende MAIS que sem, mas
	// sem pet ainda rende.
	FGatherContext SoMao = MaoVazia; SoMao.bPetHelps = false;
	FGatherContext MaoEPet = MaoVazia; MaoEPet.bPetHelps = true;
	const int32 SemPet = Yield(EWorldResource::Flor, SoMao);
	const int32 ComPet = Yield(EWorldResource::Flor, MaoEPet);
	TestTrue(TEXT("sem pet ainda colhe"), SemPet > 0);
	TestTrue(TEXT("com pet colhe MAIS"), ComPet > SemPet);

	// Picareta serve para pedra, minerio e cristal (fonte unica da exigencia).
	TestEqual(TEXT("pedra exige picareta"),
		static_cast<int32>(ResourceCatalog::RequiredTool(EWorldResource::Pedra)),
		static_cast<int32>(EGatherTool::Picareta));
	TestTrue(TEXT("pedra com picareta rende"),
		Yield(EWorldResource::Pedra, ComPicareta) > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestRegulationTest,
	"BattleSquare.World.Recursos.BosqueLivreMataRegulada",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRegulationTest::RunTest(const FString&)
{
	using ForestRegulation::MayCutFreely;

	// BOSQUE PLANTADO (do assentamento): corte livre, com ou sem permissao.
	TestTrue(TEXT("bosque plantado e corte livre"),
		MayCutFreely(/*bPlantedGrove=*/true, /*bRegulationAllows=*/false));

	// MATA SELVAGEM: so com permissao da regulacao (Guarda Florestal).
	TestTrue(TEXT("mata selvagem com permissao pode cortar"),
		MayCutFreely(false, true));

	// O CONTRAPESO: mata selvagem NUNCA e corte livre por omissao.
	TestFalse(TEXT("mata selvagem sem permissao NAO e corte livre"),
		MayCutFreely(false, false));

	return true;
}
