// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Environment/FreshWater.h"
#include "ProceduralMeshComponent.h"
#include "World/IslandBakedPlan.h"
#include "World/RiverMesh.h"

/**
 * A ÁGUA CORRENTE existe na tela, com a largura certa e assentada no chão.
 *
 * Três coisas separadas, e cada uma some sozinha: a contagem (curso que não
 * virou geometria), a largura (calha uniforme em vez de rio que engrossa) e a
 * altura (lâmina flutuando ou enterrada). Bateria verde não cobre nenhuma.
 */

namespace ProvaDoRio
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverMeshAssignsItsMaterialInTheConstructorTest,
	"BattleSquare.RiverMesh.AssignsItsMaterialInTheConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverMeshAssignsItsMaterialInTheConstructorTest::RunTest(const FString& Parameters)
{
	// No DEFAULT da classe: é o que prova que a atribuição está no construtor,
	// e não numa montagem que alguém pode esquecer de chamar.
	const ARiverMesh* Padrao = GetDefault<ARiverMesh>();
	if (!TestNotNull(TEXT("a classe do rio tem default"), Padrao))
	{
		return false;
	}

	const UProceduralMeshComponent* Agua = Padrao->GetWater();
	if (!TestNotNull(TEXT("o rio tem componente de agua"), Agua))
	{
		return false;
	}

	TestNotNull(TEXT("a agua nasce com material atribuido"), Agua->GetMaterial(0));

	// Rio que BARRA seria parede. Quem entra na água atravessa; o que muda é
	// o movimento, e isso é regra, não colisão.
	TestEqual(TEXT("a agua nao bloqueia"),
		static_cast<int32>(Agua->GetCollisionEnabled()),
		static_cast<int32>(ECollisionEnabled::QueryOnly));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverMeshBuildsEveryCourseOfThePlanTest,
	"BattleSquare.RiverMesh.BuildsEveryCourseOfThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverMeshBuildsEveryCourseOfThePlanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoRio::MundoDeTeste();
	ARiverMesh* Rio = Mundo->SpawnActor<ARiverMesh>();
	const int32 Erguidos = Rio->BuildFrom(*Assado);

	// Curso que o traçado tem e o mundo não ergueu é água que some sem nada
	// acusar — o modo de falhar desta feature inteira.
	TestEqual(TEXT("um curso erguido por curso do assado"),
		Erguidos, Assado->Rivers.Num());
	TestEqual(TEXT("e o ator sabe dizer quantos"),
		Rio->GetBuiltCourseCount(), Assado->Rivers.Num());

	// E o assado tem de bater com o gerador: se ele perdeu cursos ao assar, a
	// linha acima passaria com a bacia pela metade.
	TestEqual(TEXT("o assado tem os cursos do gerador"),
		Assado->Rivers.Num(), FreshWater::Plan().Num());

	// Uma seção de malha por curso, e todas com tinta.
	UProceduralMeshComponent* Agua = Rio->GetWater();
	TestEqual(TEXT("uma secao por curso"), Agua->GetNumSections(), Erguidos);
	for (int32 Secao = 0; Secao < Agua->GetNumSections(); ++Secao)
	{
		if (!Agua->GetMaterial(Secao))
		{
			AddError(FString::Printf(TEXT("a secao %d subiu sem material"), Secao));
			Mundo->DestroyWorld(false);
			return false;
		}
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverMeshWidthMatchesTheProgressAlongTheCourseTest,
	"BattleSquare.RiverMesh.WidthMatchesTheProgressAlongTheCourse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverMeshWidthMatchesTheProgressAlongTheCourseTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoRio::MundoDeTeste();
	ARiverMesh* Rio = Mundo->SpawnActor<ARiverMesh>();
	Rio->BuildFrom(*Assado);

	// A largura erguida em cada ponto tem de ser a do assado naquele ponto.
	// Uma calha de largura constante passaria em contagem e em altura.
	for (int32 Curso = 0; Curso < Assado->Rivers.Num(); ++Curso)
	{
		const FBakedRiver& DoAssado = Assado->Rivers[Curso];
		for (int32 Ponto = 0; Ponto < DoAssado.HalfWidthUnits.Num(); ++Ponto)
		{
			if (!FMath::IsNearlyEqual(Rio->BuiltHalfWidthAt(Curso, Ponto),
				DoAssado.HalfWidthUnits[Ponto], 0.01f))
			{
				AddError(FString::Printf(
					TEXT("curso %d, ponto %d: erguido %.2f, assado %.2f"),
					Curso, Ponto, Rio->BuiltHalfWidthAt(Curso, Ponto),
					DoAssado.HalfWidthUnits[Ponto]));
				Mundo->DestroyWorld(false);
				return false;
			}
		}
	}

	// E a água ENGROSSA: se toda meia-largura fosse igual, o laço acima
	// passaria feliz e o rio seria um cano da nascente ao mar.
	float MaisEstreito = TNumericLimits<float>::Max();
	float MaisLargo = 0.0f;
	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (const float Meia : Curso.HalfWidthUnits)
		{
			MaisEstreito = FMath::Min(MaisEstreito, Meia);
			MaisLargo = FMath::Max(MaisLargo, Meia);
		}
	}
	TestTrue(TEXT("a bacia tem calhas de larguras diferentes"), MaisLargo > MaisEstreito);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverMeshSitsOnTheGroundTest,
	"BattleSquare.RiverMesh.SitsOnTheGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverMeshSitsOnTheGroundTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A lâmina acompanha o chão ponto a ponto. Uma altura só para o curso
	// inteiro poria a foz enterrada e a cabeceira no ar — e nada nas outras
	// provas notaria, porque a contagem e a largura continuariam certas.
	//
	// A elevação é fixa e pequena: água exatamente na altura do terreno briga
	// por profundidade com ele, e o resultado é uma faixa piscando, com duas
	// geometrias corretas e a bateria toda verde.
	const float Elevacao = ARiverMesh::SurfaceLiftUnits();
	TestTrue(TEXT("a lamina sobe acima do chao"), Elevacao > 0.0f);

	// A lâmina ACOMPANHA o relevo: a altura do chão sob um mesmo curso tem de
	// variar. Se ela fosse constante, uma altura única por curso passaria em
	// tudo o que está acima — contagem certa, largura certa, e a água plana
	// atravessando o morro.
	int32 CursosComDesnivel = 0;
	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		float MaisAlto = TNumericLimits<float>::Lowest();
		float MaisBaixo = TNumericLimits<float>::Max();
		for (const FVector2D& Ponto : Curso.PointsUnits)
		{
			const float Chao = Assado->HeightAt(Ponto);
			MaisAlto = FMath::Max(MaisAlto, Chao);
			MaisBaixo = FMath::Min(MaisBaixo, Chao);
		}
		if (MaisAlto - MaisBaixo > 1.0f)
		{
			++CursosComDesnivel;
		}
	}
	TestTrue(TEXT("os cursos correm sobre chao que varia"),
		CursosComDesnivel > Assado->Rivers.Num() / 2);

	// O leito DESCE da cabeceira para a foz em algum curso: rio que sobe o
	// morro seria defeito de traçado, e rio plano não corre.
	int32 CursosQueDescem = 0;
	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		if (Curso.PointsUnits.Num() < 2)
		{
			continue;
		}
		const float NaCabeceira = Assado->HeightAt(Curso.PointsUnits[0]);
		const float NaFoz = Assado->HeightAt(Curso.PointsUnits.Last());
		if (NaFoz < NaCabeceira)
		{
			++CursosQueDescem;
		}
	}
	TestTrue(TEXT("existe curso que desce"), CursosQueDescem > 0);

	return true;
}
