// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "World/AqueductLayout.h"
#include "World/AqueductMesh.h"
#include "World/IslandBakedPlan.h"

/**
 * OS AQUEDUTOS — e o que os separa de um cano mágico.
 *
 * A obra tem de DESCER do começo ao fim, e nunca entrar no morro. Um aqueduto
 * horizontal ou enterrado passa em contagem, em material e em colisão, e some
 * exatamente a amarra que o torna interessante.
 */

namespace ProvaDoAqueduto
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAqueductMeshAssignsItsMaterialInTheConstructorTest,
	"BattleSquare.AqueductMesh.AssignsItsMaterialInTheConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAqueductMeshAssignsItsMaterialInTheConstructorTest::RunTest(const FString& Parameters)
{
	const AAqueductMesh* Padrao = GetDefault<AAqueductMesh>();
	if (!TestNotNull(TEXT("a classe do aqueduto tem default"), Padrao))
	{
		return false;
	}

	const UProceduralMeshComponent* Calha = Padrao->GetChannel();
	if (!TestNotNull(TEXT("o aqueduto tem componente"), Calha))
	{
		return false;
	}

	TestNotNull(TEXT("a calha nasce com material atribuido"), Calha->GetMaterial(0));

	// Obra de pedra sustenta quem anda em cima.
	TestEqual(TEXT("a calha bloqueia"),
		static_cast<int32>(Calha->GetCollisionEnabled()),
		static_cast<int32>(ECollisionEnabled::QueryAndPhysics));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAqueductMeshBuildsEveryOneTest,
	"BattleSquare.AqueductMesh.BuildsEveryOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAqueductMeshBuildsEveryOneTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoAqueduto::MundoDeTeste();
	AAqueductMesh* Obras = Mundo->SpawnActor<AAqueductMesh>();
	const int32 Erguidos = Obras->BuildFrom(*Assado);

	TestEqual(TEXT("um aqueduto erguido por aqueduto do assado"),
		Erguidos, Assado->Aqueducts.Num());
	TestEqual(TEXT("o assado tem os aquedutos do gerador"),
		Assado->Aqueducts.Num(), AqueductLayout::Plan().Num());
	TestTrue(TEXT("a ilha tem aquedutos"), Erguidos > 0);

	// A QUEDA viaja com a obra. O assado guardava só a linha, e sem a queda o
	// aqueduto sairia horizontal — o mesmo defeito que a largura perdida do
	// córrego, no mesmo lugar.
	for (int32 Qual = 0; Qual < Assado->Aqueducts.Num(); ++Qual)
	{
		TestTrue(*FString::Printf(TEXT("o aqueduto %d foi assado com queda"), Qual),
			Assado->Aqueducts[Qual].DropUnits > 0.0f);
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAqueductMeshDescendsAndNeverEntersTheHillTest,
	"BattleSquare.AqueductMesh.DescendsAndNeverEntersTheHill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAqueductMeshDescendsAndNeverEntersTheHillTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoAqueduto::MundoDeTeste();
	AAqueductMesh* Obras = Mundo->SpawnActor<AAqueductMesh>();
	Obras->BuildFrom(*Assado);

	for (int32 Qual = 0; Qual < Assado->Aqueducts.Num(); ++Qual)
	{
		const FBakedAqueduct& Obra = Assado->Aqueducts[Qual];
		if (Obra.PointsUnits.Num() < 2)
		{
			continue;
		}

		// DESCE, ponto a ponto e nunca sobe. Água não sobe o morro sozinha, e
		// um aqueduto que sobe em algum trecho é um cano mágico.
		for (int32 Ponto = 1; Ponto < Obra.PointsUnits.Num(); ++Ponto)
		{
			const float Antes = Obras->BuiltHeightAt(Qual, Ponto - 1);
			const float Agora = Obras->BuiltHeightAt(Qual, Ponto);

			if (Agora > Antes + KINDA_SMALL_NUMBER)
			{
				AddError(FString::Printf(
					TEXT("o aqueduto %d SOBE no ponto %d (%.1f para %.1f)"),
					Qual, Ponto, Antes, Agora));
				Mundo->DestroyWorld(false);
				return false;
			}
		}

		// E DESCE DE VERDADE: começo igual ao fim passaria no laço acima com a
		// calha perfeitamente horizontal.
		const float NoComeco = Obras->BuiltHeightAt(Qual, 0);
		const float NoFim = Obras->BuiltHeightAt(Qual, Obra.PointsUnits.Num() - 1);
		TestTrue(*FString::Printf(TEXT("o aqueduto %d desce de verdade"), Qual),
			NoComeco - NoFim > 0.0f);

		// NUNCA ENTRA NO MORRO: enterrado, ele some, e a contagem continua 2.
		for (int32 Ponto = 0; Ponto < Obra.PointsUnits.Num(); ++Ponto)
		{
			const float NoChao = Assado->HeightAt(Obra.PointsUnits[Ponto]);
			if (Obras->BuiltHeightAt(Qual, Ponto) <= NoChao)
			{
				AddError(FString::Printf(
					TEXT("o aqueduto %d esta enterrado no ponto %d (calha %.1f, chao %.1f)"),
					Qual, Ponto, Obras->BuiltHeightAt(Qual, Ponto), NoChao));
				Mundo->DestroyWorld(false);
				return false;
			}
		}
	}

	Mundo->DestroyWorld(false);
	return true;
}
