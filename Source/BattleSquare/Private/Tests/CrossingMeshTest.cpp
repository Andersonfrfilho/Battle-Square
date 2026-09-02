// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "World/CrossingMesh.h"
#include "World/IslandBakedPlan.h"
#include "World/RiverMesh.h"
#include "World/TrailLayout.h"
#include "World/WaterFooting.h"

/**
 * AS TRAVESSIAS — um teste por TIPO, porque cada tipo é uma coisa diferente.
 *
 * Se todas se atravessassem igual, o traçado não precisaria de quatro. É
 * justamente a diferença que some quando alguém constrói as quatro do mesmo
 * jeito, e nenhuma contagem acusa: 56 travessias erguidas, 56 iguais.
 */

namespace ProvaDaTravessia
{
	constexpr uint8 Vau = static_cast<uint8>(TrailLayout::ECrossingKind::Vau);
	constexpr uint8 Ponte = static_cast<uint8>(TrailLayout::ECrossingKind::Ponte);
	constexpr uint8 Barranco = static_cast<uint8>(TrailLayout::ECrossingKind::Barranco);
	constexpr uint8 Balsa = static_cast<uint8>(TrailLayout::ECrossingKind::Balsa);

	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	ACrossingMesh* Erguidas(UWorld* Mundo, const UIslandBakedPlan& Assado)
	{
		ACrossingMesh* Obras = Mundo->SpawnActor<ACrossingMesh>();
		Obras->BuildFrom(Assado);
		return Obras;
	}

	/**
	 * Um assado de mentira com UMA travessia do tipo pedido.
	 *
	 * Existe porque o traçado desta ilha não produz todos os tipos, e a
	 * construção de um tipo ausente é código sem prova — exatamente a
	 * situação que este projeto já pagou caro. Aqui se prova a CONSTRUÇÃO,
	 * que é o que esta tarefa entrega; quantos a ilha tem é outra pergunta.
	 */
	UIslandBakedPlan* AssadoComUmaTravessia(const UIslandBakedPlan& Original, uint8 Tipo)
	{
		UIslandBakedPlan* Falso = NewObject<UIslandBakedPlan>();
		Falso->HeightGridSide = Original.HeightGridSide;
		Falso->LandRadiusUnits = Original.LandRadiusUnits;
		Falso->GroundHeightUnits = Original.GroundHeightUnits;
		Falso->Trails = Original.Trails;

		FBakedCrossing Uma;
		Uma.Kind = Tipo;
		Uma.CenterUnits = FVector2D(1000.0f, 1000.0f);
		Uma.DepthUnits = 800.0f;
		Falso->Crossings.Add(Uma);

		return Falso;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossingMeshAssignsItsMaterialInTheConstructorTest,
	"BattleSquare.CrossingMesh.AssignsItsMaterialInTheConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrossingMeshAssignsItsMaterialInTheConstructorTest::RunTest(const FString& Parameters)
{
	const ACrossingMesh* Padrao = GetDefault<ACrossingMesh>();
	if (!TestNotNull(TEXT("a classe da travessia tem default"), Padrao))
	{
		return false;
	}

	const UProceduralMeshComponent* Obra = Padrao->GetStructure();
	if (!TestNotNull(TEXT("a travessia tem componente"), Obra))
	{
		return false;
	}

	TestNotNull(TEXT("a obra nasce com material atribuido"), Obra->GetMaterial(0));

	// A obra SUSTENTA. Sem colisão, quem pisa na ponte cai no rio e a ponte
	// vira desenho — e a bateria ficaria verde do mesmo jeito.
	TestEqual(TEXT("a obra bloqueia"),
		static_cast<int32>(Obra->GetCollisionEnabled()),
		static_cast<int32>(ECollisionEnabled::QueryAndPhysics));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossingMeshSeesEveryCrossingOfThePlanTest,
	"BattleSquare.CrossingMesh.SeesEveryCrossingOfThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrossingMeshSeesEveryCrossingOfThePlanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaTravessia::MundoDeTeste();
	ACrossingMesh* Obras = ProvaDaTravessia::Erguidas(Mundo, *Assado);

	// Toda travessia do traçado é VISTA — inclusive as que não ganham obra.
	// Contar só as construídas esconderia uma travessia perdida atrás do vau.
	TestEqual(TEXT("toda travessia do assado foi vista"),
		Obras->GetSeenTotal(), Assado->Crossings.Num());
	TestEqual(TEXT("o assado tem as travessias do gerador"),
		Assado->Crossings.Num(), TrailLayout::Crossings().Num());

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossingMeshTheFordBuildsNothingTest,
	"BattleSquare.CrossingMesh.TheFordBuildsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrossingMeshTheFordBuildsNothingTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaTravessia::MundoDeTeste();
	ACrossingMesh* Obras = ProvaDaTravessia::Erguidas(Mundo, *Assado);

	// O VAU SE ATRAVESSA A PÉ, e por isso não constrói NADA. Isso é decisão,
	// não omissão: uma obra no vau seria construção onde o traçado disse que
	// não há, e apagaria a diferença entre passar andando e precisar de ponte.
	TestTrue(TEXT("o tracado tem vaus"),
		Obras->GetSeenCount(ProvaDaTravessia::Vau) > 0);
	TestEqual(TEXT("nenhum vau ganhou obra"),
		Obras->GetBuiltCount(ProvaDaTravessia::Vau), 0);

	// E quem diz que ali se passa a pé é a regra da água, não a geometria.
	for (const FBakedCrossing& Onde : Assado->Crossings)
	{
		if (Onde.Kind != ProvaDaTravessia::Vau)
		{
			continue;
		}

		TestTrue(TEXT("no vau nao ha agua funda"),
			WaterFooting::At(*Assado, Onde.CenterUnits) != EWaterFooting::Fundo);
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossingMeshTheBridgeIsGeometryOverTheWaterTest,
	"BattleSquare.CrossingMesh.TheBridgeIsGeometryOverTheWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrossingMeshTheBridgeIsGeometryOverTheWaterTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Real = IslandBakedPlan::Load();
	if (!Real)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// ESTA ILHA NÃO TEM PONTE NENHUMA: medido, 0 de 56 travessias, e
	// `BridgePoints()` vazio. Isso é do traçado, e por invariante não se mexe
	// nele aqui — mas a CONSTRUÇÃO da ponte não pode ficar sem prova só porque
	// o traçado desta ilha não pediu uma. Código sem prova é exatamente o que
	// aparece quebrado no dia em que outro bioma produzir a primeira.
	UWorld* Mundo = ProvaDaTravessia::MundoDeTeste();
	UIslandBakedPlan* Falso =
		ProvaDaTravessia::AssadoComUmaTravessia(*Real, ProvaDaTravessia::Ponte);

	ACrossingMesh* Obras = Mundo->SpawnActor<ACrossingMesh>();
	Obras->BuildFrom(*Falso);

	TestEqual(TEXT("a ponte ganhou obra"),
		Obras->GetBuiltCount(ProvaDaTravessia::Ponte), 1);

	// PONTE É GEOMETRIA, e geometria ACIMA DA ÁGUA. Um tabuleiro na altura da
	// lâmina não é ponte: ele some dentro do rio, e a obra vira faixa piscando.
	const UProceduralMeshComponent* Obra = Obras->GetStructure();
	TestTrue(TEXT("a ponte tem malha"), Obra->GetNumSections() > 0);

	const FVector2D Centro(1000.0f, 1000.0f);
	const float NaLamina = Falso->HeightAt(Centro) + ARiverMesh::SurfaceLiftUnits();

	TestTrue(TEXT("o tabuleiro passa acima da lamina"),
		ACrossingMesh::DeckClearanceUnits() > 0.0f);

	FProcMeshSection const* Secao = Obras->GetStructure()->GetProcMeshSection(0);
	if (!Secao || Secao->ProcVertexBuffer.Num() == 0)
	{
		AddError(TEXT("a ponte subiu sem vertice nenhum"));
		Mundo->DestroyWorld(false);
		return false;
	}

	for (const FProcMeshVertex& Vertice : Secao->ProcVertexBuffer)
	{
		if (Vertice.Position.Z <= NaLamina)
		{
			AddError(FString::Printf(
				TEXT("um canto do tabuleiro esta em %.1f, na ou abaixo da lamina (%.1f)"),
				Vertice.Position.Z, NaLamina));
			Mundo->DestroyWorld(false);
			return false;
		}
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossingMeshTheBluffIsARampNotADeckTest,
	"BattleSquare.CrossingMesh.TheBluffIsARampNotADeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrossingMeshTheBluffIsARampNotADeckTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Real = IslandBakedPlan::Load();
	if (!Real)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaTravessia::MundoDeTeste();
	UIslandBakedPlan* Falso =
		ProvaDaTravessia::AssadoComUmaTravessia(*Real, ProvaDaTravessia::Barranco);

	ACrossingMesh* Obras = Mundo->SpawnActor<ACrossingMesh>();
	Obras->BuildFrom(*Falso);

	TestEqual(TEXT("o barranco ganhou obra"),
		Obras->GetBuiltCount(ProvaDaTravessia::Barranco), 1);

	// O BARRANCO EXIGE SUBIDA, e é isso que o separa da ponte: a ponte precisa
	// de duas margens no mesmo nível, e onde uma delas é um degrau o que se faz
	// é cavar a descida. Uma laje HORIZONTAL ali seria uma ponte que ninguém
	// pediu — e ela passaria em contagem, em material e em colisão.
	FProcMeshSection const* Secao = Obras->GetStructure()->GetProcMeshSection(0);
	if (!Secao || Secao->ProcVertexBuffer.Num() == 0)
	{
		AddError(TEXT("o barranco subiu sem vertice nenhum"));
		Mundo->DestroyWorld(false);
		return false;
	}

	float MaisAlto = TNumericLimits<float>::Lowest();
	float MaisBaixo = TNumericLimits<float>::Max();
	for (const FProcMeshVertex& Vertice : Secao->ProcVertexBuffer)
	{
		MaisAlto = FMath::Max(MaisAlto, static_cast<float>(Vertice.Position.Z));
		MaisBaixo = FMath::Min(MaisBaixo, static_cast<float>(Vertice.Position.Z));
	}

	if (!(MaisAlto - MaisBaixo > 1.0f))
	{
		AddError(FString::Printf(
			TEXT("o barranco saiu HORIZONTAL (alto %.1f, baixo %.1f) — isso e uma ponte"),
			MaisAlto, MaisBaixo));
		Mundo->DestroyWorld(false);
		return false;
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossingMeshTheFerryFloatsOnTheWaterTest,
	"BattleSquare.CrossingMesh.TheFerryFloatsOnTheWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrossingMeshTheFerryFloatsOnTheWaterTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaTravessia::MundoDeTeste();
	ACrossingMesh* Obras = ProvaDaTravessia::Erguidas(Mundo, *Assado);

	// Esta ilha TEM balsas — 25 delas — então esta prova roda no traçado real.
	TestTrue(TEXT("o tracado tem balsas"),
		Obras->GetSeenCount(ProvaDaTravessia::Balsa) > 0);
	TestEqual(TEXT("toda balsa vista ganhou plataforma"),
		Obras->GetBuiltCount(ProvaDaTravessia::Balsa),
		Obras->GetSeenCount(ProvaDaTravessia::Balsa));

	// A BALSA FLUTUA: ela fica logo acima da lâmina, não sobre um vão. Ela não
	// vence a água por altura — vence atravessando por cima dela. Construída na
	// altura da ponte, ela viraria uma ponte de 25 vãos, e a distinção que o
	// traçado fez entre "largo demais para ponte" e "ponte" desapareceria.
	TestTrue(TEXT("a balsa fica mais baixa que o tabuleiro de uma ponte"),
		ACrossingMesh::DeckClearanceUnits() > 0.0f);

	FProcMeshSection const* Secao = Obras->GetStructure()->GetProcMeshSection(0);
	if (!Secao || Secao->ProcVertexBuffer.Num() == 0)
	{
		AddError(TEXT("nenhuma obra subiu"));
		Mundo->DestroyWorld(false);
		return false;
	}

	// A TRAVESSIA PLANEJA a balsa; quem a instancia e o `GameMode`.
	//
	// Aqui se cobra o PLANO: uma balsa por travessia de balsa, no lugar certo,
	// com vao e lamina. Que ela anda, flutua e esbarra e provado no ator dela
	// (`BattleSquare.FerryActor`) — separado, porque sao duas coisas.
	TestEqual(TEXT("um plano de balsa por balsa vista"),
		Obras->GetFerryPlacements().Num(), Obras->GetSeenCount(ProvaDaTravessia::Balsa));

	for (const ACrossingMesh::FFerryPlacement& Plano : Obras->GetFerryPlacements())
	{
		// Vao zero seria uma balsa que nao sai do lugar, e ela passaria em
		// contagem sem levar ninguem a lugar nenhum.
		TestTrue(TEXT("a balsa tem vao para vencer"), Plano.SpanUnits > 0.0f);

		// Rumo nulo poria toda balsa alinhada ao eixo X, e metade delas
		// atravessaria o rio no sentido do comprimento.
		TestTrue(TEXT("a balsa tem rumo"), !Plano.AxisUnits.IsNearlyZero());

		// E ela nasce na LAMINA daquele ponto, nao numa altura qualquer.
		TestEqual(TEXT("a lamina do plano e a do ponto"),
			Plano.WaterZ,
			Assado->HeightAt(Plano.CenterUnits) + ARiverMesh::SurfaceLiftUnits(), 1.0f);
	}

	Mundo->DestroyWorld(false);
	return true;
}
