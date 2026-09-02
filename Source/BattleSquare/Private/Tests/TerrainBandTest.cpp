// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "World/IslandBakedPlan.h"
#include "World/TerrainMesh.h"

/**
 * A COR CONTA O TERRENO — e a prova é a ATRIBUIÇÃO, não a existência.
 *
 * Uma seção de malha sem material sobe com o xadrez cinza da engine: ela
 * existe, tem geometria, passa em todo teste de forma, e não conta nada ao
 * jogador. É o mesmo invisível dos pets, dos inimigos e do próprio jogador,
 * agora repartido em pedaços.
 */

namespace ProvaDaFaixa
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainBandEverySectionIsPaintedTest,
	"BattleSquare.TerrainBand.EverySectionIsPainted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainBandEverySectionIsPaintedTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaFaixa::MundoDeTeste();
	ATerrainMesh* Relevo = Mundo->SpawnActor<ATerrainMesh>();
	Relevo->BuildFrom(*Assado);

	UProceduralMeshComponent* Superficie = Relevo->GetSurface();

	// Nenhuma seção sem tinta. Zero seções também reprova: uma ilha sem
	// nenhuma faixa é uma ilha que ninguém vê.
	const int32 Secoes = Superficie->GetNumSections();
	TestTrue(TEXT("o relevo subiu em secoes"), Secoes > 0);

	for (int32 Secao = 0; Secao < Secoes; ++Secao)
	{
		TestNotNull(*FString::Printf(TEXT("a secao %d tem material atribuido"), Secao),
			Superficie->GetMaterial(Secao));
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainBandTheIslandShowsMoreThanOneTerrainTest,
	"BattleSquare.TerrainBand.TheIslandShowsMoreThanOneTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainBandTheIslandShowsMoreThanOneTerrainTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaFaixa::MundoDeTeste();
	ATerrainMesh* Relevo = Mundo->SpawnActor<ATerrainMesh>();
	Relevo->BuildFrom(*Assado);

	// Se a regra de faixa devolvesse sempre a mesma coisa, todo o teste acima
	// passaria: uma seção, pintada, sem defeito nenhum — e a ilha inteira de
	// uma cor só. O que se cobra aqui é que a cor SEPARE terrenos.
	int32 FaixasPresentes = 0;
	FString Quais;
	for (int32 Faixa = 0; Faixa < static_cast<int32>(ETerrainBand::Count); ++Faixa)
	{
		if (Relevo->GetSectionOfBand(static_cast<ETerrainBand>(Faixa)) != INDEX_NONE)
		{
			++FaixasPresentes;
			Quais += FString::Printf(TEXT(" %s"),
				UIslandBakedPlan::BandDebugName(static_cast<ETerrainBand>(Faixa)));
		}
	}

	if (FaixasPresentes < 2)
	{
		AddError(FString::Printf(
			TEXT("a ilha subiu com %d faixa(s):%s — a cor nao esta separando terreno"),
			FaixasPresentes, *Quais));
		Mundo->DestroyWorld(false);
		return false;
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainBandBeachHugsTheCoastlineTest,
	"BattleSquare.TerrainBand.BeachHugsTheCoastline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainBandBeachHugsTheCoastlineTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A praia mede da COSTA para dentro. Medida do centro para fora, ela cairia
	// no mar numa reentrância — e a ilha deixou de ser um círculo faz tempo.
	//
	// Na forma existencial: basta haver rumo em que a beira é praia. Exigir de
	// TODO rumo afirmaria que nenhuma reentrância cai dentro do vulcão ou de
	// outra faixa que legitimamente vem antes.
	int32 RumosComPraia = 0;
	constexpr int32 Rumos = 36;

	for (int32 Rumo = 0; Rumo < Rumos; ++Rumo)
	{
		const float Angulo = (2.0f * PI * Rumo) / Rumos;
		const FVector2D Direcao(FMath::Cos(Angulo), FMath::Sin(Angulo));
		const float NaCosta = Assado->CoastRadiusAt(Angulo);

		if (Assado->BandAt(Direcao * (NaCosta - 1.0f)) == ETerrainBand::Praia)
		{
			++RumosComPraia;
		}
	}

	TestTrue(TEXT("a beira da ilha e praia na maioria dos rumos"),
		RumosComPraia > Rumos / 2);

	// E longe da costa NÃO é praia, senão a faixa seria a ilha inteira.
	TestNotEqual(TEXT("o centro da ilha nao e praia"),
		static_cast<int32>(Assado->BandAt(FVector2D::ZeroVector)),
		static_cast<int32>(ETerrainBand::Praia));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainBandScorchedRockWinsOverEverythingTest,
	"BattleSquare.TerrainBand.ScorchedRockWinsOverEverything",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainBandScorchedRockWinsOverEverythingTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A ordem das faixas é decisão, não acaso: o vulcão queima o que já estava
	// ali. No centro da mancha queimada não pode sair outra coisa.
	TestEqual(TEXT("o centro do vulcao e rocha queimada"),
		static_cast<int32>(Assado->BandAt(Assado->Parameters.VolcanoCenterUnits)),
		static_cast<int32>(ETerrainBand::RochaQueimada));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainBandNoBandIsUnreachableTest,
	"BattleSquare.TerrainBand.NoBandIsUnreachable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainBandNoBandIsUnreachableTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// TODA FAIXA DECLARADA TEM DE APARECER em alguma casa da ilha.
	//
	// Faixa que o código declara e o mundo nunca produz é cor que o jogador
	// jamais vê — e nada acusa, porque as outras aparecem e o mapa parece
	// completo. Foi assim que o CUME ficou de fora: as 36 casas acima do
	// limiar estavam TODAS dentro da mancha queimada, que é avaliada antes, e
	// a prova de variedade pedia só "mais de uma faixa" — passou com quatro.
	//
	// Este teste é a versão forte daquela: nenhuma faixa pode ser inalcançável
	// por construção.
	TArray<int32> CasasDaFaixa;
	CasasDaFaixa.SetNumZeroed(static_cast<int32>(ETerrainBand::Count));

	const int32 Lado = Assado->HeightGridSide;
	for (int32 Linha = 0; Linha < Lado; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
		{
			const float X = ((static_cast<float>(Coluna) / (Lado - 1)) * 2.0f - 1.0f)
				* Assado->LandRadiusUnits;
			const float Y = ((static_cast<float>(Linha) / (Lado - 1)) * 2.0f - 1.0f)
				* Assado->LandRadiusUnits;

			++CasasDaFaixa[static_cast<int32>(Assado->BandAt(FVector2D(X, Y)))];
		}
	}

	FString Ausentes;
	for (int32 Faixa = 0; Faixa < static_cast<int32>(ETerrainBand::Count); ++Faixa)
	{
		if (CasasDaFaixa[Faixa] == 0)
		{
			Ausentes += FString::Printf(TEXT(" %s"),
				UIslandBakedPlan::BandDebugName(static_cast<ETerrainBand>(Faixa)));
		}
	}

	if (!Ausentes.IsEmpty())
	{
		AddError(FString::Printf(
			TEXT("faixa(s) que o codigo declara e a ilha nunca produz:%s — ")
			TEXT("cor que o jogador jamais ve"), *Ausentes));
		return false;
	}

	return true;
}
