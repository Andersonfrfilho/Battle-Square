// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandFeatureLayout.h"

#include "Environment/CaveSystem.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	int32 ContarPecasDaIlha(IslandFeatureLayout::EIslandFeature Tipo)
	{
		int32 Quantas = 0;
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature == Tipo)
			{
				++Quantas;
			}
		}
		return Quantas;
	}

	FString NomeDaPecaDaIlha(const IslandFeatureLayout::FFeaturePlacement& Peca)
	{
		switch (Peca.Feature)
		{
		case IslandFeatureLayout::EIslandFeature::Cave:
			return FString::Printf(TEXT("caverna %dx%d a %.0f graus"),
				Peca.CaveSide, Peca.CaveSide, Peca.AngleDegrees);
		case IslandFeatureLayout::EIslandFeature::Volcano:
			return FString::Printf(TEXT("vulcao a %.0f graus"), Peca.AngleDegrees);
		case IslandFeatureLayout::EIslandFeature::WalkableMountain:
			break;
		}

		return FString::Printf(TEXT("montanha a %.0f graus"), Peca.AngleDegrees);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutFitsOnLandTest,
	"BattleSquare.IslandFeatureLayout.EverythingFitsOnLand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutFitsOnLandTest::RunTest(const FString& Parameters)
{
	// A terra é um disco cercado de água. Peça que passa da borda não fica
	// "um pouco fora": fica com metade dentro do mar, e quem sobe nela cai.
	const IslandFeatureLayout::FIslandBounds Ilha;
	const TArray<IslandFeatureLayout::FFeaturePlacement> Pecas = IslandFeatureLayout::Plan();

	TestTrue(TEXT("a ilha tem peças plantadas"), Pecas.Num() > 0);

	for (const IslandFeatureLayout::FFeaturePlacement& Peca : Pecas)
	{
		TestTrue(*FString::Printf(TEXT("%s cabe na terra"), *NomeDaPecaDaIlha(Peca)),
			IslandFeatureLayout::FitsOnLand(Peca, Ilha));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutClearsFieldsTest,
	"BattleSquare.IslandFeatureLayout.NothingTouchesTrainingFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutClearsFieldsTest::RunTest(const FString& Parameters)
{
	// Montanha em cima de clareira de treino tira do mapa a única coisa que
	// dava destino a ele — e o defeito é invisível até alguém ir treinar.
	const IslandFeatureLayout::FIslandBounds Ilha;

	for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
	{
		TestTrue(*FString::Printf(TEXT("%s não invade campo de treino"), *NomeDaPecaDaIlha(Peca)),
			IslandFeatureLayout::ClearsTrainingFields(Peca, Ilha));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutNoOverlapTest,
	"BattleSquare.IslandFeatureLayout.NothingOverlaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutNoOverlapTest::RunTest(const FString& Parameters)
{
	// Caverna dentro de montanha é o defeito que só aparece indo até lá a pé.
	const TArray<IslandFeatureLayout::FFeaturePlacement> Pecas = IslandFeatureLayout::Plan();

	for (int32 Primeira = 0; Primeira < Pecas.Num(); ++Primeira)
	{
		for (int32 Segunda = Primeira + 1; Segunda < Pecas.Num(); ++Segunda)
		{
			TestFalse(*FString::Printf(TEXT("%s invade %s"),
					*NomeDaPecaDaIlha(Pecas[Primeira]), *NomeDaPecaDaIlha(Pecas[Segunda])),
				IslandFeatureLayout::Overlaps(Pecas[Primeira], Pecas[Segunda]));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutHasBothKindsTest,
	"BattleSquare.IslandFeatureLayout.HasMountainsAndCaves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutHasBothKindsTest::RunTest(const FString& Parameters)
{
	// "A MAIORIA das montanhas precisa de trilha" só significa alguma coisa se
	// houver mais de uma montanha; "ALÉM das montanhas, cavernas" só significa
	// alguma coisa se elas convivem no mesmo mapa.
	TestTrue(TEXT("a ilha tem pelo menos três montanhas com trilha"),
		ContarPecasDaIlha(IslandFeatureLayout::EIslandFeature::WalkableMountain) >= 3);

	TestTrue(TEXT("a ilha tem pelo menos três cavernas"),
		ContarPecasDaIlha(IslandFeatureLayout::EIslandFeature::Cave) >= 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutHasLargeCaveTest,
	"BattleSquare.IslandFeatureLayout.HasALargeCave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutHasLargeCaveTest::RunTest(const FString& Parameters)
{
	// "Algumas precisam ser grandes" é verificável: uma das cavernas plantadas
	// é a do lado maior, e ela ocupa mais chão que a menor.
	int32 MaiorLado = 0;
	int32 MenorLado = MAX_int32;

	for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
	{
		if (Peca.Feature != IslandFeatureLayout::EIslandFeature::Cave)
		{
			continue;
		}
		MaiorLado = FMath::Max(MaiorLado, Peca.CaveSide);
		MenorLado = FMath::Min(MenorLado, Peca.CaveSide);
	}

	TestEqual(TEXT("a maior caverna plantada é a grande"), MaiorLado, ACaveSystem::LargeCaveSide);
	TestTrue(TEXT("há caverna menor que a grande"), MenorLado < MaiorLado);
	TestTrue(TEXT("a grande ocupa mais chão que a menor"),
		IslandFeatureLayout::CaveClearanceUnits(MaiorLado)
			> IslandFeatureLayout::CaveClearanceUnits(MenorLado));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutClearanceCoversCornerTest,
	"BattleSquare.IslandFeatureLayout.ClearanceCoversTheCorner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutClearanceCoversCornerTest::RunTest(const FString& Parameters)
{
	// A caverna é um QUADRADO e o vizinho não está alinhado com ele. Folga do
	// tamanho do meio-lado deixa a quina entrar no vizinho sem ninguém ver.
	for (const int32 Lado : { ACaveSystem::SmallCaveSide, ACaveSystem::MediumCaveSide, ACaveSystem::LargeCaveSide })
	{
		const float MeioLado = 0.5f * ACaveSystem::FootprintForSide(Lado);
		TestTrue(*FString::Printf(TEXT("a folga da caverna %d cobre a quina"), Lado),
			IslandFeatureLayout::CaveClearanceUnits(Lado) >= MeioLado * UE_SQRT_2 - 0.01f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutIsStableTest,
	"BattleSquare.IslandFeatureLayout.IsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutIsStableTest::RunTest(const FString& Parameters)
{
	// Paisagem não sorteia. Montanha que muda de lugar a cada partida faz o
	// mapa deixar de ser um lugar que a pessoa aprende.
	const TArray<IslandFeatureLayout::FFeaturePlacement> Primeira = IslandFeatureLayout::Plan();
	const TArray<IslandFeatureLayout::FFeaturePlacement> Segunda = IslandFeatureLayout::Plan();

	TestEqual(TEXT("mesma quantidade de peças"), Segunda.Num(), Primeira.Num());

	for (int32 Indice = 0; Indice < Primeira.Num() && Indice < Segunda.Num(); ++Indice)
	{
		TestEqual(*FString::Printf(TEXT("peça %d no mesmo ângulo"), Indice),
			Segunda[Indice].AngleDegrees, Primeira[Indice].AngleDegrees);
		TestEqual(*FString::Printf(TEXT("peça %d no mesmo raio"), Indice),
			Segunda[Indice].RadiusUnits, Primeira[Indice].RadiusUnits);
	}

	return true;
}

/**
 * O vulcão precisa cair no setor de vulcão.
 *
 * Um marco que anuncia um bioma e planta no bioma vizinho é pior que marco
 * nenhum: ele ensina o mapa errado. E o ângulo dele não é conferível a olho —
 * a divisa entre setores mora na geografia, não neste arquivo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFeatureLayoutPutsTheVolcanoInItsSectorTest,
	"BattleSquare.IslandFeatureLayout.PutsTheVolcanoInItsSector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFeatureLayoutPutsTheVolcanoInItsSectorTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("a ilha tem vulcao"),
		ContarPecasDaIlha(IslandFeatureLayout::EIslandFeature::Volcano) > 0);

	for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
	{
		if (Peca.Feature != IslandFeatureLayout::EIslandFeature::Volcano)
		{
			continue;
		}

		TestEqual(*FString::Printf(TEXT("%s cai no bioma de vulcao"), *NomeDaPecaDaIlha(Peca)),
			static_cast<int32>(IslandGeography::BiomeAt(Peca.CenterUnits())),
			static_cast<int32>(EIslandBiome::Volcano));
	}

	return true;
}

#endif
