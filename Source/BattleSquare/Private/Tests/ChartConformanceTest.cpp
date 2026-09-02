// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "World/CrossingMesh.h"
#include "World/GroundUseActor.h"
#include "World/IslandBakedPlan.h"
#include "World/TrailLayout.h"

/**
 * O MUNDO CONTRA A CARTA — `docs/mundo/carta-ilha-de-mata.html`.
 *
 * A carta é o gabarito de aceite desta feature, e até aqui conferir contra ela
 * era DISCIPLINA: alguém abria o mapa, contava, e comparava. Disciplina falha
 * na terceira edição — é o mesmo argumento que fez o mundo e a carta lerem o
 * mesmo arquivo em vez de cada um o seu.
 *
 * Aqui os números da carta viram AFIRMAÇÃO. Eles estão escritos por extenso de
 * propósito: são o aceite combinado com quem desenhou o mundo, e um traçado que
 * passasse a produzir outra quantidade tem de reprovar AQUI, alto, em vez de
 * deixar carta e mundo divergirem em silêncio.
 *
 * Quando um número mudar de verdade, muda-se a carta e muda-se esta lista — nas
 * duas, de propósito, porque a divergência é a informação.
 */

namespace ProvaDaCarta
{
	int32 ManchasDe(const UIslandBakedPlan& Assado, EGroundUse Uso)
	{
		int32 Total = 0;
		for (const FBakedGroundUse& Mancha : Assado.GroundUses)
		{
			if (Mancha.Use == Uso)
			{
				++Total;
			}
		}
		return Total;
	}

	int32 PocosQueDaoAgua(const UIslandBakedPlan& Assado)
	{
		int32 Total = 0;
		for (const FBakedGroundUse& Mancha : Assado.GroundUses)
		{
			if (Mancha.Use == EGroundUse::Poco && Mancha.bYieldsWater)
			{
				++Total;
			}
		}
		return Total;
	}

	int32 TravessiasDe(const UIslandBakedPlan& Assado, TrailLayout::ECrossingKind Tipo)
	{
		int32 Total = 0;
		for (const FBakedCrossing& Onde : Assado.Crossings)
		{
			if (Onde.Kind == static_cast<uint8>(Tipo))
			{
				++Total;
			}
		}
		return Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChartConformanceGroundUseMatchesTest,
	"BattleSquare.ChartConformance.GroundUseMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChartConformanceGroundUseMatchesTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// Cada linha destas é um número que a carta imprime.
	TestEqual(TEXT("carta: 9 bosques"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Bosque), 9);
	TestEqual(TEXT("carta: 6 clareiras"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::ClareiraFechada), 6);
	TestEqual(TEXT("carta: 8 fazendas"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Fazenda), 8);
	TestEqual(TEXT("carta: 4 criadouros"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Criadouro), 4);
	TestEqual(TEXT("carta: 4 lojas"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Loja), 4);
	TestEqual(TEXT("carta: 6 acampamentos"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Acampamento), 6);
	TestEqual(TEXT("carta: 3 pomares"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Pomar), 3);
	TestEqual(TEXT("carta: 7 decks"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Deck), 7);
	TestEqual(TEXT("carta: 5 templos"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Templo), 5);
	TestEqual(TEXT("carta: 4 ruinas"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Ruina), 4);
	TestEqual(TEXT("carta: 7 cemiterios"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Cemiterio), 7);

	// A carta diz DOIS poços que dão água. Contar todos os poços daria doze, e
	// o mapa prometeria água em dez lugares secos.
	TestEqual(TEXT("carta: 2 pocos que dao agua"),
		ProvaDaCarta::PocosQueDaoAgua(*Assado), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChartConformanceCrossingsMatchTest,
	"BattleSquare.ChartConformance.CrossingsMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChartConformanceCrossingsMatchTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	TestEqual(TEXT("carta: 30 vaus"),
		ProvaDaCarta::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Vau), 30);
	TestEqual(TEXT("carta: 25 balsas"),
		ProvaDaCarta::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Balsa), 25);

	// A CARTA DIZ ZERO PONTES, e o zero é afirmado como qualquer outro número.
	//
	// Sem esta linha, o dia em que o traçado começasse a produzir pontes
	// passaria despercebido — e o mundo teria obras que a carta não mostra,
	// que é a divergência silenciosa que esta feature inteira combate. O zero
	// é uma medição, não uma ausência de medição.
	TestEqual(TEXT("carta: 0 pontes"),
		ProvaDaCarta::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Ponte), 0);

	// E o total fecha: 30 + 25 + 1 barranco = 56.
	TestEqual(TEXT("carta: 56 travessias ao todo"), Assado->Crossings.Num(), 56);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChartConformanceWaterAndTrailsMatchTest,
	"BattleSquare.ChartConformance.WaterAndTrailsMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChartConformanceWaterAndTrailsMatchTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// Os números que a spec desta feature levantou como o que o traçado calcula
	// e ninguém construía. Agora todos são construídos, e são estes.
	TestEqual(TEXT("137 cursos d'agua"), Assado->Rivers.Num(), 137);
	TestEqual(TEXT("5 corregos"), Assado->Brooks.Num(), 5);
	TestEqual(TEXT("5 fontes"), Assado->Springs.Num(), 5);
	TestEqual(TEXT("23 trilhas"), Assado->Trails.Num(), 23);
	TestEqual(TEXT("158 galerias"), Assado->UnderwaterLinks.Num(), 158);
	TestEqual(TEXT("2 aquedutos"), Assado->Aqueducts.Num(), 2);

	return true;
}
