// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"
#include "Environment/IslandGeography.h"

namespace TracadoDaRegiao
{
	int32 Contar(ESettlementKind Tipo)
	{
		int32 Quantos = 0;
		for (const FSettlementPlacement& Peca : RegionLayout::Plan())
		{
			if (Peca.Kind == Tipo)
			{
				++Quantos;
			}
		}
		return Quantos;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRegionLayoutTemTresVilasUmaCidadeEFronteirasTest,
	"BattleSquare.RegionLayout.TemTresVilasUmaCidadeEFronteiras",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionLayoutTemTresVilasUmaCidadeEFronteirasTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("uma vila inicial"),
		TracadoDaRegiao::Contar(ESettlementKind::VilaInicial), 1);
	TestEqual(TEXT("uma vila de academia"),
		TracadoDaRegiao::Contar(ESettlementKind::VilaDaAcademia), 1);
	TestEqual(TEXT("uma vila de mercado"),
		TracadoDaRegiao::Contar(ESettlementKind::VilaDoMercado), 1);
	TestEqual(TEXT("uma cidade grande"),
		TracadoDaRegiao::Contar(ESettlementKind::CidadeGrande), 1);
	TestEqual(TEXT("os postos de fronteira"),
		TracadoDaRegiao::Contar(ESettlementKind::PostoDeFronteira),
		RegionLayout::BorderPostCount());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRegionLayoutCadaVilaTemFuncaoPropriaTest,
	"BattleSquare.RegionLayout.CadaVilaTemFuncaoPropria",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionLayoutCadaVilaTemFuncaoPropriaTest::RunTest(const FString& Parameters)
{
	// A regra que ordena a spec: três vilas IGUAIS é uma vila visitada três
	// vezes, e a viagem até a segunda vira imposto. Este teste é o que impede
	// que "mais uma vila" seja acrescentada sem função.
	const TArray<FVillagePlacement> Inicial =
		VillageLayout::PlanFor(ESettlementKind::VilaInicial);
	const TArray<FVillagePlacement> DaAcademia =
		VillageLayout::PlanFor(ESettlementKind::VilaDaAcademia);
	const TArray<FVillagePlacement> DoMercado =
		VillageLayout::PlanFor(ESettlementKind::VilaDoMercado);
	const TArray<FVillagePlacement> Cidade =
		VillageLayout::PlanFor(ESettlementKind::CidadeGrande);

	TestTrue(TEXT("a vila inicial tem a Escola"),
		VillageLayout::HasBuilding(Inicial, EVillageBuilding::Escola));

	// A ausência é o desenho: sem academia em casa, a primeira viagem tem
	// motivo. Afirmar a ausência é o que impede alguém de "completar" a vila
	// inicial e apagar o motivo sem perceber.
	TestFalse(TEXT("e NÃO tem academia — é o que dá motivo à primeira viagem"),
		VillageLayout::HasBuilding(Inicial, EVillageBuilding::Academia));

	TestTrue(TEXT("a vila da academia tem academia"),
		VillageLayout::HasBuilding(DaAcademia, EVillageBuilding::Academia));
	TestFalse(TEXT("e não tem a Escola, que é de casa"),
		VillageLayout::HasBuilding(DaAcademia, EVillageBuilding::Escola));

	TestTrue(TEXT("a vila do mercado tem mercado"),
		VillageLayout::HasBuilding(DoMercado, EVillageBuilding::Mercado));
	TestFalse(TEXT("e não tem academia"),
		VillageLayout::HasBuilding(DoMercado, EVillageBuilding::Academia));

	// A cidade tem TUDO porque a região precisa ser autossuficiente: quem
	// nasce aqui não visita outra região antes de vencer esta.
	TestTrue(TEXT("a cidade tem arena"),
		VillageLayout::HasBuilding(Cidade, EVillageBuilding::Arena));
	TestTrue(TEXT("a cidade tem academia"),
		VillageLayout::HasBuilding(Cidade, EVillageBuilding::Academia));
	TestTrue(TEXT("a cidade tem mercado"),
		VillageLayout::HasBuilding(Cidade, EVillageBuilding::Mercado));
	TestTrue(TEXT("a cidade cura"),
		VillageLayout::HasBuilding(Cidade, EVillageBuilding::CentroDeRecuperacao));

	// O posto não é vila: é uma PORTA. Se ele ganhar cura ou mercado, a
	// fronteira vira destino, e ninguém mais precisa da cidade.
	const TArray<FVillagePlacement> Posto =
		VillageLayout::PlanFor(ESettlementKind::PostoDeFronteira);
	TestTrue(TEXT("o posto tem portão"),
		VillageLayout::HasBuilding(Posto, EVillageBuilding::Portao));
	TestFalse(TEXT("e não cura — posto não é destino"),
		VillageLayout::HasBuilding(Posto, EVillageBuilding::CentroDeRecuperacao));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRegionLayoutNadaSeSobrepoeNemCaiNoMarTest,
	"BattleSquare.RegionLayout.NadaSeSobrepoeNemCaiNoMar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionLayoutNadaSeSobrepoeNemCaiNoMarTest::RunTest(const FString& Parameters)
{
	const TArray<FSettlementPlacement> Pecas = RegionLayout::Plan();

	for (int32 Primeiro = 0; Primeiro < Pecas.Num(); ++Primeiro)
	{
		const float MetadeA = VillageLayout::ClearingHalfExtentUnitsFor(Pecas[Primeiro].Kind);

		// Cada assentamento inteiro cai em terra. Um posto com metade no mar é
		// exatamente o defeito da cinta de praia que não cabia no bloco:
		// número escolhido quando só existia um tamanho.
		TestTrue(TEXT("o assentamento inteiro cai em terra"),
			Pecas[Primeiro].CenterUnits.Size() + MetadeA <= IslandGeography::LandRadiusUnits());

		for (int32 Segundo = Primeiro + 1; Segundo < Pecas.Num(); ++Segundo)
		{
			const float MetadeB = VillageLayout::ClearingHalfExtentUnitsFor(Pecas[Segundo].Kind);
			const FVector2D Distancia = Pecas[Primeiro].CenterUnits - Pecas[Segundo].CenterUnits;

			TestFalse(TEXT("duas clareiras não se invadem"),
				FMath::Abs(Distancia.X) < (MetadeA + MetadeB)
					&& FMath::Abs(Distancia.Y) < (MetadeA + MetadeB));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRegionLayoutNinguemMoraNaRochaQueimadaTest,
	"BattleSquare.RegionLayout.NinguemMoraNaRochaQueimada",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionLayoutNinguemMoraNaRochaQueimadaTest::RunTest(const FString& Parameters)
{
	// O vulcão é marco, e o chão em volta da cratera é rocha queimada. Uma
	// vila ali seria cenário mentindo sobre onde a pessoa está — e o rumo dela
	// é justamente o número que alguém mexe sem lembrar do vulcão.
	for (const FSettlementPlacement& Peca : RegionLayout::Plan())
	{
		const float ateOVulcao =
			FVector2D::Distance(Peca.CenterUnits, IslandGeography::VolcanoCenterUnits());
		const float Folga = VillageLayout::ClearingHalfExtentUnitsFor(Peca.Kind);

		TestTrue(TEXT("ninguém mora dentro da rocha queimada"),
			ateOVulcao - Folga > IslandGeography::VolcanoScorchedRadiusUnits());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRegionLayoutTudoCabeNoLoteDoSeuTipoTest,
	"BattleSquare.RegionLayout.TudoCabeNoLoteDoSeuTipo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionLayoutTudoCabeNoLoteDoSeuTipoTest::RunTest(const FString& Parameters)
{
	const ESettlementKind Tipos[] = {
		ESettlementKind::VilaInicial,
		ESettlementKind::VilaDaAcademia,
		ESettlementKind::VilaDoMercado,
		ESettlementKind::CidadeGrande,
		ESettlementKind::PostoDeFronteira,
	};

	for (const ESettlementKind Tipo : Tipos)
	{
		const TArray<FVillagePlacement> Pecas = VillageLayout::PlanFor(Tipo);
		TestTrue(TEXT("o assentamento tem prédios"), Pecas.Num() > 0);

		for (int32 Primeiro = 0; Primeiro < Pecas.Num(); ++Primeiro)
		{
			TestTrue(TEXT("o prédio cabe no lote do seu tipo"),
				VillageLayout::FitsInPlotFor(Tipo, Pecas[Primeiro]));

			for (int32 Segundo = Primeiro + 1; Segundo < Pecas.Num(); ++Segundo)
			{
				TestFalse(TEXT("nenhum prédio invade o outro"),
					VillageLayout::Overlaps(Pecas[Primeiro], Pecas[Segundo]));
			}
		}
	}

	return true;
}
