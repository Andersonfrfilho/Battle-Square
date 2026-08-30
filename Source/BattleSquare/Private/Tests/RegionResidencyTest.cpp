// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/RegionResidency.h"

#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"
#include "World/WorldDiscovery.h"

namespace
{
	// Nomes próprios (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.

	/** Os nove pedaços à volta de um ponto, já filtrados por terra. */
	TSet<FIntPoint> VizinhancaResidenteDe(const FVector2D& Onde)
	{
		const RegionResidency::FResidencyChange Plano =
			RegionResidency::PlanChange(TSet<FIntPoint>(), Onde);

		TSet<FIntPoint> Vivos;
		for (const FIntPoint& Pedaco : Plano.ToBuild)
		{
			Vivos.Add(Pedaco);
		}
		return Vivos;
	}

	/** Um ponto qualquer bem no meio do pedaço, para não cair em costura. */
	FVector2D MeioDoPedacoResidente(const FIntPoint& Pedaco)
	{
		return RegionResidency::ChunkCenterUnits(Pedaco);
	}
}

/**
 * O pedaço é um MÚLTIPLO da região de descoberta, não um lado próprio.
 *
 * Duas malhas desalinhadas fariam "região" querer dizer duas coisas: uma para
 * a memória de onde se esteve, outra para o que está montado. A primeira vez
 * que alguém comparasse as duas, o erro seria de um lugar que nunca existiu.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencyAlignsWithDiscoveryGridTest,
	"BattleSquare.RegionResidency.AlignsWithDiscoveryGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencyAlignsWithDiscoveryGridTest::RunTest(const FString& Parameters)
{
	const float Lado = RegionResidency::ChunkSideUnits();

	TestEqual(TEXT("o lado do pedaco e' o lado da regiao vezes o multiplicador"),
		Lado,
		FWorldDiscovery::RegionSizeUnits * static_cast<float>(RegionResidency::ChunkSideInRegions));

	TestTrue(TEXT("e cabe um numero inteiro de regioes dentro dele"),
		FMath::IsNearlyZero(FMath::Fmod(Lado, FWorldDiscovery::RegionSizeUnits), 0.01f));

	return true;
}

/**
 * Ida e volta: o centro de um pedaço cai naquele pedaço.
 *
 * Inclui coordenadas negativas de propósito. Truncar em vez de tomar o piso
 * faz -1 e +1 caírem no mesmo pedaço, e a ilha ganharia uma costura de largura
 * dupla em cima de cada eixo — visível só depois de plantada.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencyRoundTripsChunkCoordinatesTest,
	"BattleSquare.RegionResidency.RoundTripsChunkCoordinates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencyRoundTripsChunkCoordinatesTest::RunTest(const FString& Parameters)
{
	const TArray<FIntPoint> Amostras = {
		FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1),
		FIntPoint(-1, 0), FIntPoint(0, -1), FIntPoint(-1, -1),
		FIntPoint(3, -2), FIntPoint(-4, 5),
	};

	for (const FIntPoint& Pedaco : Amostras)
	{
		TestEqual(
			FString::Printf(TEXT("o centro de (%d,%d) cai no proprio pedaco"), Pedaco.X, Pedaco.Y),
			RegionResidency::ChunkAt(MeioDoPedacoResidente(Pedaco)),
			Pedaco);
	}

	TestNotEqual(TEXT("o pedaco antes do eixo nao e' o pedaco depois dele"),
		RegionResidency::ChunkAt(FVector2D(-1.0f, -1.0f)),
		RegionResidency::ChunkAt(FVector2D(1.0f, 1.0f)));

	return true;
}

/**
 * O custo acompanha a TERRA, não a área do quadrado que a contém.
 *
 * Sem esta pergunta, andar pela borda montaria mata no meio do oceano — que é
 * o mesmo trabalho de montar a ilha, gasto onde não há nada para ver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencySkipsTheOpenSeaTest,
	"BattleSquare.RegionResidency.SkipsTheOpenSea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencySkipsTheOpenSeaTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("o pedaco do centro da ilha e' terra"),
		RegionResidency::TouchesLand(FIntPoint(0, 0)));

	const float Raio = IslandGeography::LandRadiusUnits();
	const int32 BemLonge = FMath::CeilToInt(Raio / RegionResidency::ChunkSideUnits()) + 3;

	TestFalse(TEXT("um pedaco muito alem do raio e' mar aberto"),
		RegionResidency::TouchesLand(FIntPoint(BemLonge, BemLonge)));

	return true;
}

/**
 * Nenhum pedaço COM terra dentro fica de fora.
 *
 * O contrário do teste anterior, e o mais caro de descobrir jogando: um pedaço
 * descartado por engano é um buraco no chão do tamanho de 6400 unidades, e
 * quem cai nele cai da ilha.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencyKeepsEveryChunkThatHoldsLandTest,
	"BattleSquare.RegionResidency.KeepsEveryChunkThatHoldsLand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencyKeepsEveryChunkThatHoldsLandTest::RunTest(const FString& Parameters)
{
	const float Lado = RegionResidency::ChunkSideUnits();
	const float Raio = IslandGeography::LandRadiusUnits();
	const int32 Alcance = FMath::CeilToInt(Raio / Lado) + 1;

	constexpr int32 AmostrasPorEixo = 17;

	for (int32 Coluna = -Alcance; Coluna <= Alcance; ++Coluna)
	{
		for (int32 Linha = -Alcance; Linha <= Alcance; ++Linha)
		{
			const FIntPoint Pedaco(Coluna, Linha);
			if (RegionResidency::TouchesLand(Pedaco))
			{
				continue;
			}

			// Uma asserção por PEDAÇO, não por amostra: 289 acertos silenciosos
			// vezes vinte pedaços afogariam o único que importa se falhasse.
			bool bAchouTerra = false;
			for (int32 X = 0; X < AmostrasPorEixo && !bAchouTerra; ++X)
			{
				for (int32 Y = 0; Y < AmostrasPorEixo && !bAchouTerra; ++Y)
				{
					const FVector2D Ponto(
						(static_cast<float>(Coluna) + static_cast<float>(X) / (AmostrasPorEixo - 1)) * Lado,
						(static_cast<float>(Linha) + static_cast<float>(Y) / (AmostrasPorEixo - 1)) * Lado);

					bAchouTerra = IslandGeography::IsOnLand(Ponto);
				}
			}

			TestFalse(
				FString::Printf(TEXT("o pedaco (%d,%d), dado como mar, nao contem terra"),
					Coluna, Linha),
				bAchouTerra);
		}
	}

	return true;
}

/**
 * O mesmo pedaço nasce igual toda vez que se volta a ele.
 *
 * Sair andando e voltar não pode reembaralhar a floresta que se acabou de
 * atravessar: quem usou uma árvore como referência de caminho a perderia, e o
 * mapa deixaria de ser um lugar para virar um sorteio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencyGivesEachChunkAStableSeedTest,
	"BattleSquare.RegionResidency.GivesEachChunkAStableSeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencyGivesEachChunkAStableSeedTest::RunTest(const FString& Parameters)
{
	constexpr uint32 SementeDoMundo = 20260830u;

	TestEqual(TEXT("a semente de um pedaco nao muda entre chamadas"),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(2, -3)),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(2, -3)));

	TestNotEqual(TEXT("pedacos vizinhos nao compartilham semente"),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(2, -3)),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(3, -3)));

	TestNotEqual(TEXT("nem os vizinhos do outro eixo"),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(2, -3)),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(2, -2)));

	TestNotEqual(TEXT("e trocar a semente do mundo troca a do pedaco"),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(2, -3)),
		RegionResidency::ChunkSeed(SementeDoMundo + 1u, FIntPoint(2, -3)));

	// A transposta: (2,-3) e (-3,2) são pontos diferentes da ilha, e uma
	// mistura que só somasse os eixos os daria como o mesmo lugar.
	TestNotEqual(TEXT("a transposta de um pedaco e' outro pedaco"),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(2, -3)),
		RegionResidency::ChunkSeed(SementeDoMundo, FIntPoint(-3, 2)));

	return true;
}

/**
 * Andar dentro do mesmo pedaço não monta nem derruba nada.
 *
 * É o caso comum — a maior parte dos passos acontece longe da costura. Um
 * plano que respondesse "os nove certos" em vez do delta faria o chamador
 * repetir esta conta a cada quadro para descobrir que não havia trabalho.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencyDoesNothingWhileInsideOneChunkTest,
	"BattleSquare.RegionResidency.DoesNothingWhileInsideOneChunk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencyDoesNothingWhileInsideOneChunkTest::RunTest(const FString& Parameters)
{
	const FVector2D Centro = MeioDoPedacoResidente(FIntPoint(0, 0));
	const TSet<FIntPoint> Vivos = VizinhancaResidenteDe(Centro);

	TestTrue(TEXT("o primeiro plano monta alguma coisa"), Vivos.Num() > 0);

	const float UmPasso = RegionResidency::ChunkSideUnits() * 0.25f;
	const RegionResidency::FResidencyChange Depois =
		RegionResidency::PlanChange(Vivos, Centro + FVector2D(UmPasso, -UmPasso));

	TestEqual(TEXT("andar dentro do pedaco nao monta nada"), Depois.ToBuild.Num(), 0);
	TestEqual(TEXT("nem derruba nada"), Depois.ToDrop.Num(), 0);

	return true;
}

/**
 * Cruzar a costura troca SÓ as bordas: entra uma coluna, sai a oposta.
 *
 * Se o plano derrubasse os nove e remontasse os nove, atravessar a ilha
 * refaria a mata inteira a cada 6400 unidades — que é exatamente a lentidão
 * que esta residência existe para não ter.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencySwapsOnlyTheEdgeColumnTest,
	"BattleSquare.RegionResidency.SwapsOnlyTheEdgeColumn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencySwapsOnlyTheEdgeColumnTest::RunTest(const FString& Parameters)
{
	const TSet<FIntPoint> Vivos = VizinhancaResidenteDe(MeioDoPedacoResidente(FIntPoint(0, 0)));

	const RegionResidency::FResidencyChange Mudanca =
		RegionResidency::PlanChange(Vivos, MeioDoPedacoResidente(FIntPoint(1, 0)));

	for (const FIntPoint& Novo : Mudanca.ToBuild)
	{
		TestEqual(TEXT("so entra a coluna que ficou na frente"), Novo.X, 2);
	}

	for (const FIntPoint& Velho : Mudanca.ToDrop)
	{
		TestEqual(TEXT("so sai a coluna que ficou para tras"), Velho.X, -1);
	}

	TestTrue(TEXT("a coluna de tras realmente sai"), Mudanca.ToDrop.Num() > 0);

	return true;
}

/**
 * A ordem do plano é estável.
 *
 * A de um `TSet` depende da tabela de dispersão. Um plano que muda de ordem
 * entre execuções transforma "por que este pedaço nasceu antes daquele" numa
 * leitura de sorte, e é o tipo de coisa que só se descobre investigando outro
 * defeito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionResidencyOrdersThePlanTheSameWayTwiceTest,
	"BattleSquare.RegionResidency.OrdersThePlanTheSameWayTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionResidencyOrdersThePlanTheSameWayTwiceTest::RunTest(const FString& Parameters)
{
	const FVector2D Onde = MeioDoPedacoResidente(FIntPoint(0, 0));

	const RegionResidency::FResidencyChange Primeira =
		RegionResidency::PlanChange(TSet<FIntPoint>(), Onde);
	const RegionResidency::FResidencyChange Segunda =
		RegionResidency::PlanChange(TSet<FIntPoint>(), Onde);

	TestEqual(TEXT("os dois planos tem o mesmo tamanho"),
		Primeira.ToBuild.Num(), Segunda.ToBuild.Num());

	for (int32 Indice = 0; Indice < Primeira.ToBuild.Num(); ++Indice)
	{
		TestEqual(TEXT("e a mesma ordem"), Primeira.ToBuild[Indice], Segunda.ToBuild[Indice]);
	}

	// E todo pedaço planejado tem terra: montar sobre o mar seria trabalho
	// gasto onde não há nada para ver.
	for (const FIntPoint& Pedaco : Primeira.ToBuild)
	{
		TestTrue(TEXT("todo pedaco do plano encosta em terra"),
			RegionResidency::TouchesLand(Pedaco));
	}

	return true;
}
