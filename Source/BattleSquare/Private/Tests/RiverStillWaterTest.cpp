// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "World/IslandBakedPlan.h"
#include "World/RiverMesh.h"

/**
 * LAGO, QUEDA E POÇO — a água PARADA, que some à parte da água que corre.
 *
 * Um lago que não vira disco deixa o curso inteiro certo e o remanso
 * invisível: a contagem da calha continua batendo com a carta, a largura
 * continua certa, e o mapa promete um lago que o mundo não tem.
 */

namespace ProvaDoRemanso
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	ARiverMesh* AguaErguida(UWorld* Mundo, const UIslandBakedPlan& Assado)
	{
		ARiverMesh* Rio = Mundo->SpawnActor<ARiverMesh>();
		Rio->BuildFrom(Assado);
		return Rio;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverStillWaterBuildsEveryLakeAndPoolTest,
	"BattleSquare.RiverStillWater.BuildsEveryLakeAndPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverStillWaterBuildsEveryLakeAndPoolTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoRemanso::MundoDeTeste();
	ARiverMesh* Rio = ProvaDoRemanso::AguaErguida(Mundo, *Assado);

	// Conta no assado o que o mundo deveria ter erguido. O número não é
	// escrito à mão: ele sai do traçado, então continua certo quando a ilha
	// mudar de tamanho ou de bioma.
	int32 LagosNoAssado = 0;
	int32 PocosNoAssado = 0;
	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		if (Curso.bHasLake)
		{
			++LagosNoAssado;
		}
		if (Curso.bHasFall && Curso.PlungePoolHalfWidthUnits > 0.0f)
		{
			++PocosNoAssado;
		}
	}

	TestEqual(TEXT("um lago erguido por lago do tracado"),
		Rio->GetBuiltLakeCount(), LagosNoAssado);
	TestEqual(TEXT("um poco erguido por queda com poco"),
		Rio->GetBuiltPlungePoolCount(), PocosNoAssado);

	// E existe água parada nesta ilha: zero em ambos passaria nas duas linhas
	// acima com a bacia inteira sem um remanso.
	TestTrue(TEXT("a ilha tem agua parada"),
		Rio->GetBuiltLakeCount() + Rio->GetBuiltPlungePoolCount() > 0);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverStillWaterNoLakeWithoutALakeTest,
	"BattleSquare.RiverStillWater.NoLakeWithoutALake",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverStillWaterNoLakeWithoutALakeTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoRemanso::MundoDeTeste();
	ARiverMesh* Rio = ProvaDoRemanso::AguaErguida(Mundo, *Assado);

	// Nem todo curso alaga nem despenca: um fio de cabeceira que desce o tempo
	// todo não tem remanso. Forçar um lago em cada curso inventaria acidente
	// onde o terreno não tem, e é o que aconteceria se o construtor lesse a
	// posição sem perguntar o booleano antes.
	TestTrue(TEXT("nem todo curso tem lago"),
		Rio->GetBuiltLakeCount() < Assado->Rivers.Num());
	TestTrue(TEXT("nem todo curso tem poco de queda"),
		Rio->GetBuiltPlungePoolCount() < Assado->Rivers.Num());

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverStillWaterTheFallSitsOnAStepTest,
	"BattleSquare.RiverStillWater.TheFallSitsOnAStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverStillWaterTheFallSitsOnAStepTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A QUEDA FICA NO DEGRAU: onde ela está, o chão tem de descer mais depressa
	// que na média do curso. Uma cachoeira em terreno plano é uma promessa que
	// o relevo não cumpre — e nenhuma contagem notaria.
	//
	// Na forma EXISTENCIAL, sobre a MAIORIA: exigir de toda queda afirmaria que
	// nenhuma cai numa dobra que a grade de alturas não resolve, e a grade tem
	// 1.555 unidades por casa.
	int32 QuedasEmDegrau = 0;
	int32 QuedasConferidas = 0;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		if (!Curso.bHasFall || Curso.PointsUnits.Num() < 4)
		{
			continue;
		}

		// Acha o ponto do curso mais perto da queda, e mede o desnível ali
		// contra o desnível médio entre pontos vizinhos do mesmo curso.
		int32 NaQueda = 0;
		float MaisPerto = TNumericLimits<float>::Max();
		for (int32 Ponto = 0; Ponto < Curso.PointsUnits.Num(); ++Ponto)
		{
			const float Distancia =
				FVector2D::Distance(Curso.PointsUnits[Ponto], Curso.FallCenterUnits);
			if (Distancia < MaisPerto)
			{
				MaisPerto = Distancia;
				NaQueda = Ponto;
			}
		}

		if (NaQueda == 0 || NaQueda + 1 >= Curso.PointsUnits.Num())
		{
			continue;
		}

		float SomaDosDesniveis = 0.0f;
		for (int32 Ponto = 1; Ponto < Curso.PointsUnits.Num(); ++Ponto)
		{
			SomaDosDesniveis += FMath::Abs(
				Assado->HeightAt(Curso.PointsUnits[Ponto])
					- Assado->HeightAt(Curso.PointsUnits[Ponto - 1]));
		}
		const float DesnivelMedio = SomaDosDesniveis / (Curso.PointsUnits.Num() - 1);

		const float NoDegrau = FMath::Abs(
			Assado->HeightAt(Curso.PointsUnits[NaQueda + 1])
				- Assado->HeightAt(Curso.PointsUnits[NaQueda - 1]));

		++QuedasConferidas;
		if (NoDegrau >= DesnivelMedio)
		{
			++QuedasEmDegrau;
		}
	}

	if (QuedasConferidas == 0)
	{
		AddError(TEXT("nenhuma queda pode ser conferida — o tracado nao tem queda ")
			TEXT("nenhuma, ou o assado perdeu a posicao delas"));
		return false;
	}

	if (QuedasEmDegrau * 2 <= QuedasConferidas)
	{
		AddError(FString::Printf(
			TEXT("so %d de %d quedas caem num degrau mais forte que a media do curso"),
			QuedasEmDegrau, QuedasConferidas));
		return false;
	}

	return true;
}
