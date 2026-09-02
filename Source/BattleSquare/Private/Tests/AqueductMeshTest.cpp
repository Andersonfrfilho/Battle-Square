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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAqueductMeshDescendsAndTunnelsThroughTest,
	"BattleSquare.AqueductMesh.DescendsAndTunnelsThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAqueductMeshDescendsAndTunnelsThroughTest::RunTest(const FString& Parameters)
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

	// ESTE TESTE SUBSTITUI `DescendsAndNeverEntersTheHill`, e a substituição é
	// o ponto da tarefa. Aquele afirmava que a calha NUNCA entra no morro — e
	// a regra mudou: ela pode, e onde entra há TÚNEL. Deixar o antigo verde ao
	// lado do novo faria a bateria provar duas regras que se contradizem, e
	// uma delas estaria mentindo em silêncio.
	for (int32 Qual = 0; Qual < Assado->Aqueducts.Num(); ++Qual)
	{
		const FBakedAqueduct& Obra = Assado->Aqueducts[Qual];
		if (Obra.PointsUnits.Num() < 2)
		{
			continue;
		}

		// DESCE, e isso não mudou: água não sobe o morro sozinha, e é a única
		// coisa que o túnel não pode comprar.
		for (int32 Ponto = 1; Ponto < Obra.PointsUnits.Num(); ++Ponto)
		{
			if (Obras->BuiltHeightAt(Qual, Ponto)
				> Obras->BuiltHeightAt(Qual, Ponto - 1) + KINDA_SMALL_NUMBER)
			{
				AddError(FString::Printf(
					TEXT("o aqueduto %d SOBE no ponto %d"), Qual, Ponto));
				Mundo->DestroyWorld(false);
				return false;
			}
		}

		// A QUEDA É A DECLARADA. Sem isto, a obra inventaria altura para
		// escapar do morro — que é exatamente o que o envelope antigo fazia, e
		// por isso ele nunca precisava de túnel.
		const float Caiu = Obras->BuiltHeightAt(Qual, 0)
			- Obras->BuiltHeightAt(Qual, Obra.PointsUnits.Num() - 1);
		TestEqual(*FString::Printf(TEXT("o aqueduto %d cai o que declarou"), Qual),
			Caiu, Obra.DropUnits, 1.0f);

		// ONDE ESTÁ ABAIXO DO CHÃO, É TÚNEL — a marcação e a geometria têm de
		// concordar. Um ponto enterrado e não marcado é rocha maciça cortada
		// pela água, que é o que a obra não pode ser.
		for (int32 Ponto = 0; Ponto < Obra.PointsUnits.Num(); ++Ponto)
		{
			const bool bAbaixo = Obras->BuiltHeightAt(Qual, Ponto)
				< Assado->HeightAt(Obra.PointsUnits[Ponto]);

			if (bAbaixo != Obras->IsTunnelAt(Qual, Ponto))
			{
				AddError(FString::Printf(
					TEXT("aqueduto %d, ponto %d: abaixo do chao=%d, marcado como ")
					TEXT("tunel=%d — a marcacao nao bate com a geometria"),
					Qual, Ponto, bAbaixo ? 1 : 0,
					Obras->IsTunnelAt(Qual, Ponto) ? 1 : 0));
				Mundo->DestroyWorld(false);
				return false;
			}
		}

		// A SAÍDA entrega água à vila, e boca enterrada não entrega nada.
		TestFalse(*FString::Printf(TEXT("a saida do aqueduto %d nao e tunel"), Qual),
			Obras->IsTunnelAt(Qual, Obra.PointsUnits.Num() - 1));
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAqueductMeshIsNotAllTunnelTest,
	"BattleSquare.AqueductMesh.IsNotAllTunnel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAqueductMeshIsNotAllTunnelTest::RunTest(const FString& Parameters)
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

	// O CONTRAPESO, e sem ele "pode entrar no morro" viraria "é um cano
	// enterrado": uma regra que pusesse tudo debaixo da terra passaria no
	// teste acima, com a marcação batendo perfeitamente com a geometria, e
	// nenhum aqueduto apareceria na tela.
	int32 PontosAoTodo = 0;
	for (const FBakedAqueduct& Obra : Assado->Aqueducts)
	{
		PontosAoTodo += Obra.PointsUnits.Num();
	}

	if (PontosAoTodo == 0)
	{
		AddError(TEXT("nao ha aqueduto para conferir"));
		return false;
	}

	TestTrue(TEXT("existe trecho fora do morro"),
		Obras->GetTunnelPointCount() < PontosAoTodo);

	Mundo->DestroyWorld(false);
	return true;
}
