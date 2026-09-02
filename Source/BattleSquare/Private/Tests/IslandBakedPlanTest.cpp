#include "Misc/AutomationTest.h"

#include "Environment/FreshWater.h"
#include "Environment/IslandGeography.h"
#include "World/AqueductLayout.h"
#include "World/IslandBakedPlan.h"
#include "World/LandUseLayout.h"
#include "World/TrailLayout.h"

/**
 * O assado é o RETRATO do traçado — e retrato que não bate com o original é
 * pior que retrato nenhum, porque o mundo inteiro é construído em cima dele.
 *
 * Estes testes comparam campo a campo o que `BakeInto` produz com o que os
 * planos devolvem. Não é tautologia: `BakeInto` escolhe QUAIS campos assar e em
 * que resolução, e é justamente aí que uma peça some sem ninguém notar.
 */

namespace ProvaDoAssado
{
	/**
	 * Assa uma vez e reaproveita.
	 *
	 * Montar o mundo inteiro leva minutos medidos; assar uma vez por teste
	 * multiplicaria isso pelo número de testes desta bateria.
	 */
	const UIslandBakedPlan& Assado()
	{
		static UIslandBakedPlan* Guardado = nullptr;
		if (!Guardado)
		{
			Guardado = NewObject<UIslandBakedPlan>();
			Guardado->AddToRoot();
			IslandBakedPlan::BakeInto(*Guardado);
		}
		return *Guardado;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanRiversMatchThePlanTest,
	"BattleSquare.IslandBakedPlan.RiversMatchThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanRiversMatchThePlanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan& Assado = ProvaDoAssado::Assado();
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();

	TestEqual(TEXT("o assado tem um curso para cada curso do plano"),
		Assado.Rivers.Num(), Cursos.Num());

	for (int32 Qual = 0; Qual < Cursos.Num(); ++Qual)
	{
		const FBakedRiver& Rio = Assado.Rivers[Qual];
		const FreshWater::FRiverCourse& Curso = Cursos[Qual];

		TestEqual(*FString::Printf(TEXT("curso %d: pontos"), Qual),
			Rio.PointsUnits.Num(), IslandBakedPlan::RiverSampleCount());
		TestEqual(*FString::Printf(TEXT("curso %d: uma largura por ponto"), Qual),
			Rio.HalfWidthUnits.Num(), Rio.PointsUnits.Num());
		TestEqual(*FString::Printf(TEXT("curso %d: uma corredeira por ponto"), Qual),
			Rio.bIsRapids.Num(), Rio.PointsUnits.Num());

		TestEqual(*FString::Printf(TEXT("curso %d: ordem de Strahler"), Qual),
			Rio.Order, Curso.Order);
		TestEqual(*FString::Printf(TEXT("curso %d: vai ao mar"), Qual),
			Rio.bFlowsToTheSea, Curso.FlowsToTheSea());
		TestEqual(*FString::Printf(TEXT("curso %d: tem lago"), Qual),
			Rio.bHasLake, Curso.HasLake());
		TestEqual(*FString::Printf(TEXT("curso %d: tem queda"), Qual),
			Rio.bHasFall, Curso.HasFall());

		for (int32 Amostra = 0; Amostra < Rio.PointsUnits.Num(); ++Amostra)
		{
			const float Onde = IslandBakedPlan::ProgressAtSample(Amostra);
			const FVector2D DoPlano = FreshWater::PointAtProgress(Curso, Onde);

			if (!Rio.PointsUnits[Amostra].Equals(DoPlano, 0.01f))
			{
				AddError(FString::Printf(
					TEXT("curso %d, amostra %d: assado (%.2f,%.2f) != plano (%.2f,%.2f)"),
					Qual, Amostra, Rio.PointsUnits[Amostra].X, Rio.PointsUnits[Amostra].Y,
					DoPlano.X, DoPlano.Y));
				return false;
			}

			TestEqual(*FString::Printf(TEXT("curso %d, amostra %d: largura"), Qual, Amostra),
				Rio.HalfWidthUnits[Amostra],
				FreshWater::HalfWidthAtProgress(Curso, Onde), 0.01f);
			TestEqual(*FString::Printf(TEXT("curso %d, amostra %d: corredeira"), Qual, Amostra),
				Rio.bIsRapids[Amostra], FreshWater::IsRapidsAtProgress(Curso, Onde));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanTrailsCarryTheirGroundHeightTest,
	"BattleSquare.IslandBakedPlan.TrailsCarryTheirGroundHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanTrailsCarryTheirGroundHeightTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan& Assado = ProvaDoAssado::Assado();
	const TArray<FTrailRoute>& Trilhas = TrailLayout::Plan();

	TestEqual(TEXT("uma trilha assada por trilha do plano"),
		Assado.Trails.Num(), Trilhas.Num());

	for (int32 Qual = 0; Qual < Trilhas.Num(); ++Qual)
	{
		const FBakedTrail& Assada = Assado.Trails[Qual];

		TestEqual(*FString::Printf(TEXT("trilha %d: pontos"), Qual),
			Assada.PointsUnits.Num(), Trilhas[Qual].PointsUnits.Num());
		TestEqual(*FString::Printf(TEXT("trilha %d: uma altura por ponto"), Qual),
			Assada.GroundHeightUnits.Num(), Assada.PointsUnits.Num());
		TestEqual(*FString::Printf(TEXT("trilha %d: reta de emergencia"), Qual),
			Assada.bFellBackToStraightLine, Trilhas[Qual].bFellBackToStraightLine);

		for (int32 Ponto = 0; Ponto < Assada.PointsUnits.Num(); ++Ponto)
		{
			if (!Assada.PointsUnits[Ponto].Equals(Trilhas[Qual].PointsUnits[Ponto], 0.01f))
			{
				AddError(FString::Printf(TEXT("trilha %d, ponto %d: assado != plano"),
					Qual, Ponto));
				return false;
			}

			TestEqual(*FString::Printf(TEXT("trilha %d, ponto %d: altura"), Qual, Ponto),
				Assada.GroundHeightUnits[Ponto],
				IslandGeography::GroundHeightAt(Assada.PointsUnits[Ponto]), 0.01f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanCountsMatchThePlansTest,
	"BattleSquare.IslandBakedPlan.CountsMatchThePlans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanCountsMatchThePlansTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan& Assado = ProvaDoAssado::Assado();

	TestEqual(TEXT("corregos"), Assado.Brooks.Num(), FreshWater::PlanBrooks().Num());
	TestEqual(TEXT("fontes"), Assado.Springs.Num(), FreshWater::PlanSprings().Num());
	TestEqual(TEXT("travessias"), Assado.Crossings.Num(), TrailLayout::Crossings().Num());
	TestEqual(TEXT("manchas de solo"), Assado.GroundUses.Num(), LandUseLayout::Plan().Num());
	TestEqual(TEXT("galerias"),
		Assado.UnderwaterLinks.Num(), FreshWater::PlanUnderwaterLinks().Num());
	TestEqual(TEXT("aquedutos"), Assado.Aqueducts.Num(), AqueductLayout::Plan().Num());

	// A grade de alturas é quadrada e completa: casa faltando é buraco no chão.
	TestEqual(TEXT("a grade de alturas esta completa"), Assado.GroundHeightUnits.Num(),
		Assado.HeightGridSide * Assado.HeightGridSide);
	TestEqual(TEXT("o raio de terra"),
		Assado.LandRadiusUnits, IslandGeography::LandRadiusUnits(), 0.01f);

	// Nenhuma caverna sem planta: caverna de zero casas é entrada para lugar nenhum.
	for (int32 Qual = 0; Qual < Assado.Caves.Num(); ++Qual)
	{
		TestTrue(*FString::Printf(TEXT("caverna %d tem casas"), Qual),
			Assado.Caves[Qual].Columns > 0 && Assado.Caves[Qual].Rows > 0);
		TestEqual(*FString::Printf(TEXT("caverna %d: uma parede por casa"), Qual),
			Assado.Caves[Qual].Walls.Num(),
			Assado.Caves[Qual].Columns * Assado.Caves[Qual].Rows);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanGroundUsesMatchThePlanTest,
	"BattleSquare.IslandBakedPlan.GroundUsesMatchThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanGroundUsesMatchThePlanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan& Assado = ProvaDoAssado::Assado();
	const TArray<FGroundUsePatch> Manchas = LandUseLayout::Plan();

	for (int32 Qual = 0; Qual < Manchas.Num() && Qual < Assado.GroundUses.Num(); ++Qual)
	{
		const FBakedGroundUse& Assada = Assado.GroundUses[Qual];

		TestEqual(*FString::Printf(TEXT("mancha %d: uso"), Qual),
			static_cast<int32>(Assada.Use), static_cast<int32>(Manchas[Qual].Use));
		TestEqual(*FString::Printf(TEXT("mancha %d: deus"), Qual),
			static_cast<int32>(Assada.Deity), static_cast<int32>(Manchas[Qual].Deity));
		TestEqual(*FString::Printf(TEXT("mancha %d: meia extensao"), Qual),
			Assada.HalfExtentUnits, Manchas[Qual].HalfExtentUnits, 0.01f);
		TestEqual(*FString::Printf(TEXT("mancha %d: da agua"), Qual),
			Assada.bYieldsWater, Manchas[Qual].bYieldsWater);
		TestTrue(*FString::Printf(TEXT("mancha %d: centro"), Qual),
			Assada.CenterUnits.Equals(Manchas[Qual].CenterUnits, 0.01f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanCrossingsKeepTheirKindTest,
	"BattleSquare.IslandBakedPlan.CrossingsKeepTheirKind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanCrossingsKeepTheirKindTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan& Assado = ProvaDoAssado::Assado();
	const TArray<TrailLayout::FCrossing> Travessias = TrailLayout::Crossings();

	// O tipo viaja como número porque `ECrossingKind` mora num namespace, onde
	// `UENUM` não existe. Se a conversão escorregar, vau vira ponte em silêncio.
	for (int32 Qual = 0; Qual < Travessias.Num() && Qual < Assado.Crossings.Num(); ++Qual)
	{
		TestEqual(*FString::Printf(TEXT("travessia %d: tipo"), Qual),
			static_cast<int32>(Assado.Crossings[Qual].Kind),
			static_cast<int32>(Travessias[Qual].Kind));
		TestEqual(*FString::Printf(TEXT("travessia %d: fundura"), Qual),
			Assado.Crossings[Qual].DepthUnits, Travessias[Qual].DepthUnits, 0.01f);
		TestTrue(*FString::Printf(TEXT("travessia %d: centro"), Qual),
			Assado.Crossings[Qual].CenterUnits.Equals(Travessias[Qual].CenterUnits, 0.01f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanHeightGridMatchesGroundHeightTest,
	"BattleSquare.IslandBakedPlan.HeightGridMatchesGroundHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanHeightGridMatchesGroundHeightTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan& Assado = ProvaDoAssado::Assado();
	const int32 Lado = Assado.HeightGridSide;

	// Amostra esparsa: a grade tem 32.400 casas, e conferir todas custaria mais
	// que assar. O passo primo evita cair sempre nas mesmas colunas.
	for (int32 Linha = 0; Linha < Lado; Linha += 7)
	{
		for (int32 Coluna = 0; Coluna < Lado; Coluna += 11)
		{
			const float X = ((static_cast<float>(Coluna) / (Lado - 1)) * 2.0f - 1.0f)
				* Assado.LandRadiusUnits;
			const float Y = ((static_cast<float>(Linha) / (Lado - 1)) * 2.0f - 1.0f)
				* Assado.LandRadiusUnits;

			TestEqual(*FString::Printf(TEXT("altura em (%d,%d)"), Coluna, Linha),
				Assado.HeightAtCell(Coluna, Linha),
				IslandGeography::GroundHeightAt(FVector2D(X, Y)), 0.01f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanLoadsFastTest,
	"BattleSquare.IslandBakedPlan.LoadsFast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanLoadsFastTest::RunTest(const FString& Parameters)
{
	// A medição só vale na PRIMEIRA carga do processo: a engine guarda o objeto,
	// e a segunda devolveria o ponteiro sem ler disco nenhum. Este é o único
	// teste que carrega o assado, e por isso ele é quem mede.
	const double Comecou = FPlatformTime::Seconds();
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	const double Milissegundos = (FPlatformTime::Seconds() - Comecou) * 1000.0;

	if (!Assado)
	{
		AddError(FString::Printf(
			TEXT("o assado nao existe em %s — rode ./Tools/bake_island.sh"),
			IslandBakedPlan::AssetPath()));
		return false;
	}

	// 100 ms é o orçamento da spec. O ponto não é velocidade em si: é que o
	// assado exista para não se pagar os 136 segundos medidos de recalcular o
	// mundo inteiro na entrada.
	if (Milissegundos > 100.0)
	{
		AddError(FString::Printf(TEXT("o assado levou %.1f ms para carregar (teto: 100 ms)"),
			Milissegundos));
		return false;
	}

	// Carregar depressa e vir vazio passaria no relógio e derrubaria o mundo.
	TestTrue(TEXT("o assado carregado tem relevo"), Assado->GroundHeightUnits.Num() > 0);
	TestTrue(TEXT("o assado carregado tem rios"), Assado->Rivers.Num() > 0);
	TestTrue(TEXT("o assado carregado tem trilhas"), Assado->Trails.Num() > 0);

	return true;
}
