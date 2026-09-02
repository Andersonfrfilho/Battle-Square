// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Environment/IslandGeography.h"
#include "ProceduralMeshComponent.h"
#include "Tests/AutomationCommon.h"
#include "World/IslandBakedPlan.h"
#include "World/TerrainMesh.h"

/**
 * O relevo tem de EXISTIR NA TELA, não só na lógica.
 *
 * Ator que nasce com componente visual e sem asset atribuído passa em todo
 * teste de lógica e não aparece — três vezes neste projeto. Por isso o que se
 * cobra aqui é a ATRIBUIÇÃO, não a existência do componente.
 */

namespace ProvaDoRelevo
{
	/** Um mundo descartável só para o ator existir em algum lugar. */
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainMeshAssignsItsMaterialInTheConstructorTest,
	"BattleSquare.TerrainMesh.AssignsItsMaterialInTheConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainMeshAssignsItsMaterialInTheConstructorTest::RunTest(const FString& Parameters)
{
	// O DEFAULT da classe, de propósito: é o que prova que a atribuição está no
	// construtor, e não numa montagem que alguém pode esquecer de chamar.
	const ATerrainMesh* Padrao = GetDefault<ATerrainMesh>();
	if (!TestNotNull(TEXT("a classe do relevo tem default"), Padrao))
	{
		return false;
	}

	const UProceduralMeshComponent* Superficie = Padrao->GetSurface();
	if (!TestNotNull(TEXT("o relevo tem superficie"), Superficie))
	{
		return false;
	}

	// Malha procedural não tem malha para atribuir — o que ela pode nascer sem
	// é TINTA, e sem tinta o chão sobe com o xadrez cinza da engine.
	TestNotNull(TEXT("a superficie nasce com material atribuido"),
		Superficie->GetMaterial(0));

	// Chão sem colisão é desenho: o jogador atravessa a ilha caindo.
	TestTrue(TEXT("a superficie nasce com colisao"),
		Superficie->GetCollisionEnabled() != ECollisionEnabled::NoCollision);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainMeshHeightsMatchTheBakedPlanTest,
	"BattleSquare.TerrainMesh.HeightsMatchTheBakedPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainMeshHeightsMatchTheBakedPlanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoRelevo::MundoDeTeste();
	ATerrainMesh* Relevo = Mundo->SpawnActor<ATerrainMesh>();

	const int32 Vertices = Relevo->BuildFrom(*Assado);

	// Malha de zero vértice existe como ator e não é chão nenhum.
	TestEqual(TEXT("um vertice por casa da grade"), Vertices,
		Assado->HeightGridSide * Assado->HeightGridSide);
	TestEqual(TEXT("o lado da grade erguida"),
		Relevo->GetGridSide(), Assado->HeightGridSide);

	// O ponto mais alto e o mais baixo da malha têm de ser os do assado. Se a
	// construção perder a altura pelo caminho, a ilha sobe plana e nada acusa.
	float MaisAltoNaMalha = TNumericLimits<float>::Lowest();
	float MaisBaixoNaMalha = TNumericLimits<float>::Max();
	float MaisAltoNoAssado = TNumericLimits<float>::Lowest();
	float MaisBaixoNoAssado = TNumericLimits<float>::Max();

	for (int32 Linha = 0; Linha < Assado->HeightGridSide; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Assado->HeightGridSide; ++Coluna)
		{
			const float NaMalha = Relevo->BuiltHeightAtCell(Coluna, Linha);
			const float NoAssado = Assado->HeightAtCell(Coluna, Linha);

			MaisAltoNaMalha = FMath::Max(MaisAltoNaMalha, NaMalha);
			MaisBaixoNaMalha = FMath::Min(MaisBaixoNaMalha, NaMalha);
			MaisAltoNoAssado = FMath::Max(MaisAltoNoAssado, NoAssado);
			MaisBaixoNoAssado = FMath::Min(MaisBaixoNoAssado, NoAssado);
		}
	}

	TestEqual(TEXT("o ponto mais alto"), MaisAltoNaMalha, MaisAltoNoAssado, 0.01f);
	TestEqual(TEXT("o ponto mais baixo"), MaisBaixoNaMalha, MaisBaixoNoAssado, 0.01f);

	// A ilha não é plana: alto igual a baixo passaria nas duas linhas acima.
	TestTrue(TEXT("a ilha tem relevo de verdade"),
		MaisAltoNaMalha - MaisBaixoNaMalha > 1.0f);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainMeshCoversTheWholeIslandTest,
	"BattleSquare.TerrainMesh.CoversTheWholeIsland",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainMeshCoversTheWholeIslandTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoRelevo::MundoDeTeste();
	ATerrainMesh* Relevo = Mundo->SpawnActor<ATerrainMesh>();
	Relevo->BuildFrom(*Assado);

	// A casa da grade tem de ser mais FINA que a menor coisa que o mundo
	// precisa que ela enxergue. Grade grossa demais já se disfarçou de quatro
	// defeitos diferentes neste projeto — "não há pontes", "a trilha sobe de
	// frente", "não existe corredeira", "a cachoeira não tem poço".
	const float LadoCoberto = Relevo->GetCellSizeUnits() * (Relevo->GetGridSide() - 1);
	TestEqual(TEXT("a malha cobre a ilha de ponta a ponta"),
		LadoCoberto, 2.0f * Assado->LandRadiusUnits, 1.0f);

	TestTrue(TEXT("a casa da grade e menor que a largura do barranco"),
		Relevo->GetCellSizeUnits()
			< IslandGeography::BluffOuterRadiusUnits()
				- IslandGeography::BluffInnerRadiusUnits());

	Mundo->DestroyWorld(false);
	return true;
}
