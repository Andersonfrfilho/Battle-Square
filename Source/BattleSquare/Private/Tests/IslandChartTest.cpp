// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "World/IslandBakedPlan.h"
#include "World/IslandChart.h"
#include "World/LandUseLayout.h"

namespace CartaEscondida
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	FBakedGroundUse ManchaParaAContagemDaCarta(EGroundUse Uso, bool bEscondida, float X)
	{
		FBakedGroundUse Mancha;
		Mancha.Use = Uso;
		Mancha.bHidden = bEscondida;
		Mancha.CenterUnits = FVector2D(X, 0.0f);
		Mancha.HalfExtentUnits = 100.0f;
		return Mancha;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandChartCountsHiddenTest,
	"BattleSquare.World.IslandChart.ContaOEscondido",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandChartCountsHiddenTest::RunTest(const FString& Parameters)
{
	// AS TRÊS CONTAGENS, e cada uma prende uma ponta.
	//
	// Só o mostrado deixaria esconder e APAGAR terem a mesma cara — as duas
	// tirariam um da vitrine, e o gabarito não distinguiria. É o mesmo
	// raciocínio dos `0 pontes`: o zero é uma medição, não uma ausência dela.
	const TArray<FBakedGroundUse> Manchas = {
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::Loja, false, 0.0f),
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::Loja, false, 500.0f),
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::MercadoNegro, true, 900.0f),
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::MercadoNegro, true, 1400.0f),
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::Bosque, false, 2000.0f),
	};

	const IslandChart::FUseCount Lojas = IslandChart::CountOf(Manchas, EGroundUse::Loja);
	TestEqual(TEXT("duas lojas mostradas"), Lojas.Shown, 2);
	TestEqual(TEXT("nenhuma loja escondida"), Lojas.Hidden, 0);
	TestEqual(TEXT("e a soma sao duas"), Lojas.Total(), 2);

	const IslandChart::FUseCount Mercados =
		IslandChart::CountOf(Manchas, EGroundUse::MercadoNegro);
	TestEqual(TEXT("nenhum mercado-negro mostrado"), Mercados.Shown, 0);
	TestEqual(TEXT("dois escondidos"), Mercados.Hidden, 2);
	TestEqual(TEXT("e a soma sao dois"), Mercados.Total(), 2);

	// APAGAR REPROVA. Tirar um mercado do mundo muda a soma; escondê-lo, não.
	// Sem esta metade, "escondido" seria um jeito educado de sumir com coisa.
	TArray<FBakedGroundUse> Apagado = Manchas;
	Apagado.RemoveAt(2);
	TestEqual(TEXT("apagar muda a soma"),
		IslandChart::CountOf(Apagado, EGroundUse::MercadoNegro).Total(), 1);

	TArray<FBakedGroundUse> Revelado = Manchas;
	Revelado[2].bHidden = false;
	TestEqual(TEXT("revelar NAO muda a soma"),
		IslandChart::CountOf(Revelado, EGroundUse::MercadoNegro).Total(), 2);
	TestEqual(TEXT("so muda de coluna"),
		IslandChart::CountOf(Revelado, EGroundUse::MercadoNegro).Shown, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandChartDoesNotLeakWhereTest,
	"BattleSquare.World.IslandChart.ContaSemDizerOnde",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandChartDoesNotLeakWhereTest::RunTest(const FString& Parameters)
{
	// CONTAR NÃO PODE VAZAR QUAL, e isso se prova movendo o escondido.
	//
	// Se algum número da carta mudasse ao arrastar um mercado-negro para o
	// outro lado da ilha, a carta estaria dizendo alguma coisa sobre onde ele
	// está — e quem lê o gabarito duas vezes descobriria a posição por
	// diferença.
	//
	// A prova é sobre manchas montadas à mão, e não procurada no mundo:
	// procurar mediria o sorteio em vez da regra.
	const TArray<FBakedGroundUse> Aqui = {
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::MercadoNegro, true, 0.0f),
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::MercadoNegro, true, 100.0f),
		CartaEscondida::ManchaParaAContagemDaCarta(EGroundUse::Loja, false, 200.0f),
	};

	TArray<FBakedGroundUse> Acola = Aqui;
	Acola[0].CenterUnits = FVector2D(-120000.0f, 90000.0f);
	Acola[1].CenterUnits = FVector2D(70000.0f, -110000.0f);

	const IslandChart::FUseCount Antes =
		IslandChart::CountOf(Aqui, EGroundUse::MercadoNegro);
	const IslandChart::FUseCount Depois =
		IslandChart::CountOf(Acola, EGroundUse::MercadoNegro);

	TestEqual(TEXT("mover o escondido nao muda o mostrado"), Depois.Shown, Antes.Shown);
	TestEqual(TEXT("nem o escondido"), Depois.Hidden, Antes.Hidden);
	TestEqual(TEXT("nem a soma"), Depois.Total(), Antes.Total());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandChartHidesTheBlackMarketTest,
	"BattleSquare.World.IslandChart.OMercadoNegroEhEscondido",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandChartHidesTheBlackMarketTest::RunTest(const FString& Parameters)
{
	// E O PRIMEIRO CLIENTE REAL DA TERCEIRA COLUNA: o mercado-negro nasce
	// escondido no traçado, não marcado depois por quem imprime a carta.
	int32 Mercados = 0;
	int32 Escondidos = 0;

	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use != EGroundUse::MercadoNegro)
		{
			// E ninguém mais se escondeu junto: `bHidden` é falso por omissão,
			// e o dia em que outra mancha nascer escondida tem de ser um dia
			// em que alguém escreveu isso de propósito.
			TestFalse(TEXT("nenhuma outra mancha nasce escondida"), Mancha.bHidden);
			continue;
		}

		++Mercados;
		if (Mancha.bHidden)
		{
			++Escondidos;
		}
	}

	TestTrue(TEXT("ha mercado-negro"), Mercados > 0);
	TestEqual(TEXT("e todos sao escondidos"), Escondidos, Mercados);

	return true;
}
