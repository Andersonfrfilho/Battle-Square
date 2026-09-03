// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldTimeOfDay.h"
#include "Misc/AutomationTest.h"
#include "World/EncounterPredominance.h"

/**
 * A FAUNA DO LUGAR — decisão 62, e a palavra que a governa é PREDOMINÂNCIA.
 *
 * O contrapeso vem nas palavras do próprio dono: predominância NÃO é
 * exclusividade. O peso decide quem é comum; zero decidiria quem existe — e
 * trancar espécie em silêncio é o defeito de sempre com pelo.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPredominanceFavorsTheForestCommonsTest,
	"BattleSquare.World.Fauna.AMataTemSeusComuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPredominanceFavorsTheForestCommonsTest::RunTest(const FString&)
{
	EncounterPredominance::FSpawnSurroundings MataSeca;
	MataSeca.Biome = EIslandBiome::Forest;

	// O comum da natureza comum: planta e terra pesam MAIS que o neutro, e o
	// fogo — que não é bicho de floresta — pesa menos.
	const int32 Planta = EncounterPredominance::PlaceWeightPercent(
		TEXT("Fisica/Planta"), MataSeca);
	const int32 Fogo = EncounterPredominance::PlaceWeightPercent(
		TEXT("Fisica/Fogo"), MataSeca);

	TestTrue(TEXT("planta e o comum da mata"), Planta > 100);
	TestTrue(TEXT("fogo e o deslocado"), Fogo < 100);

	// "ALGUNS aquáticos... desses ambientes": comum na margem, raro longe.
	EncounterPredominance::FSpawnSurroundings Margem = MataSeca;
	Margem.bNearWater = true;

	TestTrue(TEXT("o aquatico pesa mais na margem que longe dela"),
		EncounterPredominance::PlaceWeightPercent(TEXT("Fisica/Agua"), Margem)
			> EncounterPredominance::PlaceWeightPercent(TEXT("Fisica/Agua"), MataSeca));

	// E o cavernícola perto da gruta.
	EncounterPredominance::FSpawnSurroundings Gruta = MataSeca;
	Gruta.bNearCave = true;

	TestTrue(TEXT("o fantasma pesa mais perto da gruta"),
		EncounterPredominance::PlaceWeightPercent(TEXT("Espiritual/Fantasma"), Gruta)
			> EncounterPredominance::PlaceWeightPercent(
				TEXT("Espiritual/Fantasma"), MataSeca));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPredominanceNeverExcludesTest,
	"BattleSquare.World.Fauna.PredominanciaNaoEhExclusividade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPredominanceNeverExcludesTest::RunTest(const FString&)
{
	// NENHUM tipo pesa zero em NENHUM contexto — são as palavras da decisão.
	// E os biomas sem fauna desenhada pesam NEUTRO: pesar por palpite seria
	// decidir a fauna do deserto sem ninguém pedir.
	const TCHAR* Tipos[] = {
		TEXT("Fisica/Planta"), TEXT("Fisica/Terra"), TEXT("Fisica/Agua"),
		TEXT("Fisica/Fogo"), TEXT("Espiritual/Fantasma"), TEXT("Natural/Luz"),
		TEXT("Psiquica/Raio"), TEXT("tipo-desconhecido"),
	};

	const EIslandBiome Biomas[] = {
		EIslandBiome::Forest, EIslandBiome::Desert,
		EIslandBiome::Glacier, EIslandBiome::Swamp,
	};

	for (EIslandBiome Bioma : Biomas)
	{
		for (int32 Agua = 0; Agua <= 1; ++Agua)
		{
			for (int32 Gruta = 0; Gruta <= 1; ++Gruta)
			{
				EncounterPredominance::FSpawnSurroundings EmVolta;
				EmVolta.Biome = Bioma;
				EmVolta.bNearWater = Agua != 0;
				EmVolta.bNearCave = Gruta != 0;

				for (const TCHAR* Tipo : Tipos)
				{
					TestTrue(TEXT("nenhum peso e zero — predominancia nao tranca"),
						EncounterPredominance::PlaceWeightPercent(Tipo, EmVolta) > 0);
				}
			}
		}
	}

	TestEqual(TEXT("bioma sem fauna desenhada e neutro"),
		EncounterPredominance::PlaceWeightPercent(TEXT("Fisica/Fogo"),
			{ false, false, EIslandBiome::Desert }), 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPredominanceComposesWithTheClockTest,
	"BattleSquare.World.Fauna.OLugarMultiplicaAHora",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPredominanceComposesWithTheClockTest::RunTest(const FString&)
{
	// O sorteio composto RESPONDE ao peso do lugar — e o teste ISOLA esse
	// fator: as duas espécies precisam ter a MESMA atividade de relógio,
	// senão o peso da fase entra no resultado e o teste mede a hora, não o
	// lugar. Foi exatamente assim que a primeira versão dele falhou: a
	// espécie "pesada" calhou de ser noturna, e ao meio-dia a leve venceu —
	// que é o COMPORTAMENTO CERTO da multiplicação, medido pelo teste errado.
	FString EspecieA;
	FString EspecieB;
	for (int32 Tentativa = 0; Tentativa < 64 && EspecieB.IsEmpty(); ++Tentativa)
	{
		const FString Candidata = FString::Printf(TEXT("especie-%d"), Tentativa);
		if (EspecieA.IsEmpty())
		{
			EspecieA = Candidata;
		}
		else if (WorldTimeOfDay::ActivityForSpecies(Candidata)
			== WorldTimeOfDay::ActivityForSpecies(EspecieA))
		{
			EspecieB = Candidata;
		}
	}

	TestFalse(TEXT("achou um par de mesma atividade"), EspecieB.IsEmpty());
	if (EspecieB.IsEmpty())
	{
		return true;
	}

	const TArray<FString> Ids = { EspecieA, EspecieB };
	const TArray<int32> Pesos = { 300, 30 };

	FRandomStream Sorteio(1234);
	int32 SaiuAPesada = 0;
	constexpr int32 Tiradas = 200;

	for (int32 Tirada = 0; Tirada < Tiradas; ++Tirada)
	{
		if (WorldTimeOfDay::PickSpeciesForPhaseAndPlace(
			Ids, EDayPhase::Day, Pesos, Sorteio) == 0)
		{
			++SaiuAPesada;
		}
	}

	TestTrue(TEXT("a pesada predomina"), SaiuAPesada > Tiradas / 2);
	TestTrue(TEXT("mas a leve EXISTE — nunca zero aparicoes"),
		SaiuAPesada < Tiradas);

	return true;
}
