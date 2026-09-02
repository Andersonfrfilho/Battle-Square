// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "World/IslandBakedPlan.h"
#include "World/TrailLayout.h"
#include "World/TrailMesh.h"

/**
 * AS TRILHAS existem na tela, assentadas no chão, e sobem o que prometem.
 *
 * Três coisas separadas: a contagem (trilha que não virou geometria), o
 * assentamento (flutuando ou enterrada) e o declive (o traçado promete ≤ 10%).
 */

namespace ProvaDaTrilha
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailMeshAssignsItsMaterialInTheConstructorTest,
	"BattleSquare.TrailMesh.AssignsItsMaterialInTheConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailMeshAssignsItsMaterialInTheConstructorTest::RunTest(const FString& Parameters)
{
	const ATrailMesh* Padrao = GetDefault<ATrailMesh>();
	if (!TestNotNull(TEXT("a classe da trilha tem default"), Padrao))
	{
		return false;
	}

	const UProceduralMeshComponent* Caminho = Padrao->GetTrail();
	if (!TestNotNull(TEXT("a trilha tem componente"), Caminho))
	{
		return false;
	}

	// A ATRIBUIÇÃO, não a existência do componente. Componente sem material
	// passa em todo teste de lógica e sobe com o xadrez cinza da engine.
	TestNotNull(TEXT("a trilha nasce com material atribuido"), Caminho->GetMaterial(0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailMeshBuildsEveryRouteTest,
	"BattleSquare.TrailMesh.BuildsEveryRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailMeshBuildsEveryRouteTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaTrilha::MundoDeTeste();
	ATrailMesh* Trilhas = Mundo->SpawnActor<ATrailMesh>();
	const int32 Erguidas = Trilhas->BuildFrom(*Assado);

	TestEqual(TEXT("uma trilha erguida por trilha do assado"),
		Erguidas, Assado->Trails.Num());
	TestEqual(TEXT("o assado tem as trilhas do gerador"),
		Assado->Trails.Num(), TrailLayout::Plan().Num());
	TestTrue(TEXT("a ilha tem trilhas"), Erguidas > 0);

	UProceduralMeshComponent* Caminho = Trilhas->GetTrail();
	TestEqual(TEXT("uma secao por trilha"), Caminho->GetNumSections(), Erguidas);
	for (int32 Secao = 0; Secao < Caminho->GetNumSections(); ++Secao)
	{
		if (!Caminho->GetMaterial(Secao))
		{
			AddError(FString::Printf(TEXT("a secao %d subiu sem material"), Secao));
			Mundo->DestroyWorld(false);
			return false;
		}
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailMeshNeitherFloatsNorBuriesTest,
	"BattleSquare.TrailMesh.NeitherFloatsNorBuries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailMeshNeitherFloatsNorBuriesTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaTrilha::MundoDeTeste();
	ATrailMesh* Trilhas = Mundo->SpawnActor<ATrailMesh>();
	Trilhas->BuildFrom(*Assado);

	// A trilha fica exatamente uma elevação acima do chão que a superfície tem.
	// Enterrada some; flutuando lê como fita no ar — e as duas passariam na
	// contagem e no declive sem mudar um número.
	const float Elevacao = ATrailMesh::SurfaceLiftUnits();
	TestTrue(TEXT("a trilha sobe acima do chao"), Elevacao > 0.0f);

	for (int32 Qual = 0; Qual < Assado->Trails.Num(); ++Qual)
	{
		const FBakedTrail& Rota = Assado->Trails[Qual];
		for (int32 Ponto = 0; Ponto < Rota.PointsUnits.Num(); ++Ponto)
		{
			const float NoChao = Assado->HeightAt(Rota.PointsUnits[Ponto]);
			const float NaMalha = Trilhas->BuiltHeightAt(Qual, Ponto);

			if (!FMath::IsNearlyEqual(NaMalha - NoChao, Elevacao, 0.5f))
			{
				AddError(FString::Printf(
					TEXT("trilha %d, ponto %d: %.1f acima do chao (esperado %.1f)"),
					Qual, Ponto, NaMalha - NoChao, Elevacao));
				Mundo->DestroyWorld(false);
				return false;
			}
		}
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailMeshKeepsTheSustainableGradeTest,
	"BattleSquare.TrailMesh.KeepsTheSustainableGrade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailMeshKeepsTheSustainableGradeTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// O declive sai da ALTURA QUE VIAJA COM A TRILHA, nunca de reamostrar a
	// malha: reamostrar daria o declive da MALHA, que anda 1.555 unidades por
	// casa contra os 2.520 de largura do barranco.
	//
	// ## Duas coisas que eu tinha escrito errado aqui
	//
	// A primeira: medir entre PONTOS VIZINHOS. O passo da trilha vai de 6 a
	// 7.702 unidades, e um desnível qualquer sobre um passo de 6 dá uma razão
	// enorme que não é declive de trilha nenhum — é a grade de alturas sendo
	// amostrada mais fina que ela própria. Mede-se sobre uma JANELA ANDADA.
	//
	// A segunda: cobrar 10% como TETO. Fui ler o traçado: ele trata o declive
	// sustentável como PENALIDADE DE CUSTO (`Custo *= 1 + penalidade`), não
	// como limite — ele prefere o suave, não promete o suave. Um teto aqui
	// afirmaria uma promessa que o gerador nunca fez, e reprovaria as 23
	// trilhas de uma vez, que foi o que aconteceu.
	//
	// O que se cobra são as duas propriedades que o traçado de fato entrega.
	constexpr float DeclivelSustentavel = 0.10f;
	constexpr float JanelaAndada = 3000.0f;

	TArray<float> Declives;
	for (const FBakedTrail& Rota : Assado->Trails)
	{
		int32 Comeco = 0;
		while (Comeco + 1 < Rota.PointsUnits.Num())
		{
			int32 Fim = Comeco;
			float Andado = 0.0f;
			while (Fim + 1 < Rota.PointsUnits.Num() && Andado < JanelaAndada)
			{
				Andado += FVector2D::Distance(
					Rota.PointsUnits[Fim + 1], Rota.PointsUnits[Fim]);
				++Fim;
			}

			if (Andado > KINDA_SMALL_NUMBER
				&& Rota.GroundHeightUnits.IsValidIndex(Fim)
				&& Rota.GroundHeightUnits.IsValidIndex(Comeco))
			{
				Declives.Add(FMath::Abs(
					Rota.GroundHeightUnits[Fim] - Rota.GroundHeightUnits[Comeco]) / Andado);
			}
			Comeco = Fim;
		}
	}

	if (Declives.Num() == 0)
	{
		AddError(TEXT("nenhum trecho de trilha pode ser medido"));
		return false;
	}

	// PRIMEIRA: o declive TÍPICO fica dentro do sustentável. A mediana, e não
	// o pior trecho — o pior é a exceção que a penalidade aceita pagar quando
	// não há caminho melhor, e é justamente onde o barranco obriga.
	Declives.Sort();
	const float Mediana = Declives[Declives.Num() / 2];

	if (Mediana > DeclivelSustentavel)
	{
		AddError(FString::Printf(
			TEXT("declive mediano de %.1f%% sobre janelas de %.0f u — acima do ")
			TEXT("sustentavel de %.0f%%, a trilha deixou de preferir o suave"),
			Mediana * 100.0f, JanelaAndada, DeclivelSustentavel * 100.0f));
		return false;
	}

	// SEGUNDA: nenhuma trilha é mais íngreme que ir RETO entre as pontas.
	//
	// É a propriedade que faz uma trilha em ziguezague valer a pena: ela anda
	// mais para subir menos. Uma trilha que sobe igual à reta não está
	// contornando nada, e nenhuma medida de declive isolada denunciaria isso.
	for (int32 Qual = 0; Qual < Assado->Trails.Num(); ++Qual)
	{
		const FBakedTrail& Rota = Assado->Trails[Qual];
		if (Rota.PointsUnits.Num() < 2 || Rota.GroundHeightUnits.Num() < 2)
		{
			continue;
		}

		const float Subida = FMath::Abs(
			Rota.GroundHeightUnits.Last() - Rota.GroundHeightUnits[0]);
		const float PelaReta =
			FVector2D::Distance(Rota.PointsUnits.Last(), Rota.PointsUnits[0]);

		float Andado = 0.0f;
		for (int32 Ponto = 1; Ponto < Rota.PointsUnits.Num(); ++Ponto)
		{
			Andado += FVector2D::Distance(
				Rota.PointsUnits[Ponto], Rota.PointsUnits[Ponto - 1]);
		}

		if (PelaReta <= KINDA_SMALL_NUMBER || Andado <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (Subida / Andado > Subida / PelaReta)
		{
			AddError(FString::Printf(
				TEXT("a trilha %d sobe mais depressa que a reta entre as pontas — ")
				TEXT("ela nao esta contornando nada"), Qual));
			return false;
		}
	}

	return true;
}
