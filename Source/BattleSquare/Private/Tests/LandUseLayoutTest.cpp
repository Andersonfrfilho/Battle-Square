// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/LandUseLayout.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"
#include "World/TrailLayout.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandGeography.h"

namespace UsoDoSolo
{
	int32 Contar(EGroundUse Uso)
	{
		int32 Quantos = 0;
		for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
		{
			if (Mancha.Use == Uso)
			{
				++Quantos;
			}
		}
		return Quantos;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseTodaVilaTemFazendaTest,
	"BattleSquare.LandUse.TodaVilaTemFazenda",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseTodaVilaTemFazendaTest::RunTest(const FString& Parameters)
{
	// Quem chega numa vila vê roçado antes de telhado. Vila sem lavoura é vila
	// que não come.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
		{
			continue;
		}

		bool bTemFazenda = false;
		const float Perto = VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 4.0f;

		for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
		{
			if (Mancha.Use == EGroundUse::Fazenda
				&& FVector2D::Distance(Mancha.CenterUnits, Assentamento.CenterUnits) <= Perto)
			{
				bTemFazenda = true;
			}
		}

		TestTrue(TEXT("o assentamento tem fazenda ao lado"), bTemFazenda);
	}

	// O cais NÃO planta: ele é uma porta, e dar-lhe lavoura o transformaria em
	// destino — o que a spec da região proíbe.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind != ESettlementKind::PostoDeFronteira)
		{
			continue;
		}

		TestNotEqual(TEXT("o cais não tem lavoura"),
			static_cast<int32>(LandUseLayout::UseAt(Assentamento.CenterUnits)),
			static_cast<int32>(EGroundUse::Fazenda));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseAFazendaOlhaParaAAguaTest,
	"BattleSquare.LandUse.AFazendaOlhaParaAAgua",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseAFazendaOlhaParaAAguaTest::RunTest(const FString& Parameters)
{
	// A direção não é sorteada: é a da água mais perto. Roçado longe do rio é
	// roçado que ninguém rega, e a fazenda estaria ali só porque o gerador
	// precisava pôr uma em algum lugar.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
		{
			continue;
		}

		float MaisPertoDaVila = TNumericLimits<float>::Max();
		float MaisPertoDaFazenda = TNumericLimits<float>::Max();

		for (const FreshWater::FSpring& Fonte : FreshWater::PlanSprings())
		{
			MaisPertoDaVila = FMath::Min(MaisPertoDaVila,
				FVector2D::Distance(Assentamento.CenterUnits, Fonte.CenterUnits));
		}

		bool bAchouFazenda = false;
		const float Perto = VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 4.0f;

		for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
		{
			if (Mancha.Use != EGroundUse::Fazenda
				|| FVector2D::Distance(Mancha.CenterUnits, Assentamento.CenterUnits) > Perto)
			{
				continue;
			}

			bAchouFazenda = true;
			for (const FreshWater::FSpring& Fonte : FreshWater::PlanSprings())
			{
				MaisPertoDaFazenda = FMath::Min(MaisPertoDaFazenda,
					FVector2D::Distance(Mancha.CenterUnits, Fonte.CenterUnits));
			}
		}

		if (bAchouFazenda)
		{
			TestTrue(TEXT("a fazenda está mais perto da água que a vila"),
				MaisPertoDaFazenda <= MaisPertoDaVila);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseAClareiraEhFechadaTest,
	"BattleSquare.LandUse.AClareiraEhFechada",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseAClareiraEhFechadaTest::RunTest(const FString& Parameters)
{
	// "Fechada" é a única coisa que ela é. Clareira que a trilha corta é um
	// alargamento do caminho; o que faz dela um lugar é ninguém passar por ali
	// sem querer.
	TestTrue(TEXT("existe clareira fechada"),
		UsoDoSolo::Contar(EGroundUse::ClareiraFechada) > 0);

	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use != EGroundUse::ClareiraFechada)
		{
			continue;
		}

		TestFalse(TEXT("nenhuma trilha passa pela clareira"),
			TrailLayout::IsOnTrail(Mancha.CenterUnits));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseOBosqueEhMaisCerradoTest,
	"BattleSquare.LandUse.OBosqueEhMaisCerrado",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseOBosqueEhMaisCerradoTest::RunTest(const FString& Parameters)
{
	// Sem isto, a densidade poderia devolver sempre um e a mata continuaria
	// uniforme — a mesma armadilha do relevo que poderia ser uma constante.
	TestTrue(TEXT("existe bosque"), UsoDoSolo::Contar(EGroundUse::Bosque) > 0);

	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use != EGroundUse::Bosque)
		{
			continue;
		}

		TestTrue(TEXT("no bosque nasce mais planta que no chão comum"),
			LandUseLayout::PlantingDensityAt(Mancha.CenterUnits) > 1.0f);
		TestFalse(TEXT("e o bosque não impede plantar"),
			LandUseLayout::BlocksPlanting(Mancha.CenterUnits));
	}

	// Fazenda e clareira são VAZIOS, por motivos opostos.
	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use == EGroundUse::Fazenda || Mancha.Use == EGroundUse::ClareiraFechada)
		{
			TestTrue(TEXT("ali não se planta"),
				LandUseLayout::BlocksPlanting(Mancha.CenterUnits));
			TestEqual(TEXT("e a densidade é zero"),
				LandUseLayout::PlantingDensityAt(Mancha.CenterUnits), 0.0f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseAsFontesMolhamOMioloTest,
	"BattleSquare.LandUse.AsFontesMolhamOMiolo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseAsFontesMolhamOMioloTest::RunTest(const FString& Parameters)
{
	// A ilha só tinha água que descia de montanha, e isso deixava o miolo
	// inteiro seco — que é justamente onde todo mundo mora.
	const TArray<FreshWater::FSpring> Fontes = FreshWater::PlanSprings();
	TestTrue(TEXT("há fontes"), Fontes.Num() > 0);

	const float SaiaDoMonte = FreshWater::Plan().Num() > 0
		? FreshWater::Plan()[0].SourceRadiusUnits
		: IslandGeography::LandRadiusUnits();

	for (const FreshWater::FSpring& Fonte : Fontes)
	{
		TestTrue(TEXT("a fonte brota no miolo, onde nenhum rio chega"),
			Fonte.CenterUnits.Size() < SaiaDoMonte);
		TestTrue(TEXT("e em terra"), IslandGeography::IsOnLand(Fonte.CenterUnits));
	}

	// Água que corre lado a lado sem nunca se encontrar não é bacia, é listras.
	const TArray<FreshWater::FBrook> Corregos = FreshWater::PlanBrooks();
	TestTrue(TEXT("há mais córregos que fontes — os que ligam rio a rio"),
		Corregos.Num() > Fontes.Num());

	for (const FreshWater::FBrook& Corrego : Corregos)
	{
		TestTrue(TEXT("o córrego tem curso"), Corrego.PointsUnits.Num() >= 2);
		TestTrue(TEXT("e é mais estreito que o rio — atravessa-se a pé"),
			Corrego.HalfWidthUnits < FreshWater::RiverHalfWidthUnits());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseDaParaIrDeBarcoATodoLugarTest,
	"BattleSquare.LandUse.DaParaIrDeBarcoATodoLugar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseDaParaIrDeBarcoATodoLugarTest::RunTest(const FString& Parameters)
{
	// "Dá para ir de barco a todo lugar" vira UM grafo, e ele é conexo ou não
	// é. Sem esta pergunta em código, a resposta seria olhar o mapa e achar
	// que sim — e achar é o que este projeto vem trocando por medir.
	TestTrue(TEXT("toda a água doce está ligada, por cima ou por baixo"),
		FreshWater::IsWaterNetworkConnected());

	// E as ligações de baixo existem de fato: se `PlanUnderwaterLinks` voltasse
	// vazio, a conexão viria só dos córregos e o teste acima continuaria verde
	// sem que a rede subterrânea existisse.
	const TArray<FreshWater::FUnderwaterLink> Passagens = FreshWater::PlanUnderwaterLinks();
	TestTrue(TEXT("há passagem subterrânea"), Passagens.Num() > 0);

	for (const FreshWater::FUnderwaterLink& Passagem : Passagens)
	{
		// Passagem de pedra é apertada, sempre. Se ela coubesse barco grande, o
		// atalho de baixo tornaria o rio de cima decorativo.
		TestEqual(TEXT("por baixo só passa barco pequeno"),
			static_cast<int32>(Passagem.Navigability),
			static_cast<int32>(FreshWater::ENavigability::BarcoPequeno));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseNemTodoTrechoAceitaBarcoGrandeTest,
	"BattleSquare.LandUse.NemTodoTrechoAceitaBarcoGrande",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseNemTodoTrechoAceitaBarcoGrandeTest::RunTest(const FString& Parameters)
{
	// A largura decide, e é a MESMA largura que desenha o rio. Uma segunda
	// tabela dizendo onde o barco cabe concordaria com o desenho até a
	// primeira vez que alguém alargasse um rio.
	const TArray<FreshWater::FRiverCourse> Rios = FreshWater::Plan();
	if (!TestTrue(TEXT("há rio"), Rios.Num() > 0))
	{
		return false;
	}

	// No lago passa barco grande; na cabeceira, não. Se os dois dessem o mesmo,
	// a navegação seria uniforme e a largura não significaria nada.
	TestEqual(TEXT("o lago aceita barco grande"),
		static_cast<int32>(FreshWater::NavigabilityForHalfWidth(
			FreshWater::HalfWidthAt(Rios[0], Rios[0].LakeRadiusUnits))),
		static_cast<int32>(FreshWater::ENavigability::BarcoGrande));

	// E o córrego é fio de água: atravessa-se a pé, e é por isso que ele não
	// pede ponte.
	const TArray<FreshWater::FBrook> Corregos = FreshWater::PlanBrooks();
	if (Corregos.Num() > 0)
	{
		TestNotEqual(TEXT("no córrego não passa barco grande"),
			static_cast<int32>(FreshWater::NavigabilityForHalfWidth(Corregos[0].HalfWidthUnits)),
			static_cast<int32>(FreshWater::ENavigability::BarcoGrande));
	}

	return true;
}
