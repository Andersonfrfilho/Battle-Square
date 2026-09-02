// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Environment/FreshWater.h"
#include "World/IslandBakedPlan.h"
#include "World/RiverMesh.h"

/**
 * CÓRREGO E FONTE — o que liga a bacia, e a água do lugar plano.
 *
 * Sem os córregos a ilha tem cursos paralelos e nada entre eles: água correndo
 * lado a lado sem nunca se encontrar não é bacia, é listras. E a contagem dos
 * rios continuaria batendo com a carta o tempo todo.
 */

namespace ProvaDoCorrego
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrookAndSpringBuildEveryOneOfThemTest,
	"BattleSquare.BrookAndSpring.BuildEveryOneOfThem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBrookAndSpringBuildEveryOneOfThemTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoCorrego::MundoDeTeste();
	ARiverMesh* Agua = Mundo->SpawnActor<ARiverMesh>();
	Agua->BuildFrom(*Assado);

	TestEqual(TEXT("um corrego erguido por corrego do assado"),
		Agua->GetBuiltBrookCount(), Assado->Brooks.Num());
	TestEqual(TEXT("uma fonte erguida por fonte do assado"),
		Agua->GetBuiltSpringCount(), Assado->Springs.Num());

	// E o assado tem de bater com o gerador: se ele perdesse córregos ao
	// assar, as duas linhas acima passariam com a ilha em listras.
	TestEqual(TEXT("o assado tem os corregos do gerador"),
		Assado->Brooks.Num(), FreshWater::PlanBrooks().Num());
	TestEqual(TEXT("o assado tem as fontes do gerador"),
		Assado->Springs.Num(), FreshWater::PlanSprings().Num());

	// Zero em qualquer um dos dois passaria em tudo acima.
	TestTrue(TEXT("a ilha tem corregos"), Agua->GetBuiltBrookCount() > 0);
	TestTrue(TEXT("a ilha tem fontes"), Agua->GetBuiltSpringCount() > 0);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrookAndSpringTheBrookIsNarrowerThanTheRiverTest,
	"BattleSquare.BrookAndSpring.TheBrookIsNarrowerThanTheRiver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBrookAndSpringTheBrookIsNarrowerThanTheRiverTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// O córrego SE ATRAVESSA A PÉ — é isso que o separa do rio, que precisa de
	// ponte. Desenhado com a calha de um rio, ele vira mais um rio, e a
	// diferença que lhe dá sentido desaparece sem nenhuma contagem mudar.
	//
	// O defeito que este teste pega já existiu: o assado guardava só a LINHA do
	// córrego, e a largura dele se perdia ao assar.
	for (int32 Qual = 0; Qual < Assado->Brooks.Num(); ++Qual)
	{
		if (Assado->Brooks[Qual].HalfWidthUnits <= 0.0f)
		{
			AddError(FString::Printf(
				TEXT("o corrego %d foi assado sem largura — ele sairia invisivel"), Qual));
			return false;
		}

		TestTrue(*FString::Printf(TEXT("o corrego %d e mais estreito que o rio"), Qual),
			Assado->Brooks[Qual].HalfWidthUnits < FreshWater::RiverHalfWidthUnits());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrookAndSpringSpringsHavePoolsTest,
	"BattleSquare.BrookAndSpring.SpringsHavePools",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBrookAndSpringSpringsHavePoolsTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// Fonte de poço zero é fonte que não se vê. E ela tem de estar em TERRA:
	// uma fonte no mar seria água doce boiando no salgado.
	for (int32 Qual = 0; Qual < Assado->Springs.Num(); ++Qual)
	{
		const FBakedSpring& Fonte = Assado->Springs[Qual];

		TestTrue(*FString::Printf(TEXT("a fonte %d tem poco"), Qual),
			Fonte.PoolHalfWidthUnits > 0.0f);

		const float NaCosta = Assado->CoastRadiusAt(
			FMath::Atan2(Fonte.CenterUnits.Y, Fonte.CenterUnits.X));
		TestTrue(*FString::Printf(TEXT("a fonte %d esta em terra"), Qual),
			Fonte.CenterUnits.Size() < NaCosta);
	}

	return true;
}
