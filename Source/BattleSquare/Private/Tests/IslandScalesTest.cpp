// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

namespace EscalaDaIlhaTeste
{
	/** Troca o raio da ilha por um instante, e devolve o de antes. */
	struct FRaioTemporario
	{
		explicit FRaioTemporario(float Novo)
		{
			GConfig->GetFloat(TEXT("/Script/BattleSquare.BattleSquareGameMode"),
				TEXT("WorldIslandRadiusUnits"), Anterior, GGameIni);
			GConfig->SetFloat(TEXT("/Script/BattleSquare.BattleSquareGameMode"),
				TEXT("WorldIslandRadiusUnits"), Novo, GGameIni);
		}

		~FRaioTemporario()
		{
			GConfig->SetFloat(TEXT("/Script/BattleSquare.BattleSquareGameMode"),
				TEXT("WorldIslandRadiusUnits"), Anterior, GGameIni);
		}

		float Anterior = 20000.0f;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAsPecasAcompanhamAIlhaTest,
	"BattleSquare.Environment.Island.AsPecasAcompanhamAIlha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAsPecasAcompanhamAIlhaTest::RunTest(const FString&)
{
	// Os anéis eram números ABSOLUTOS, e ninguém notava porque a ilha sempre
	// teve um tamanho só. Crescê-la deixaria montanha, caverna e vulcão
	// amontoados no miolo, com quase um quilômetro de nada em volta — e
	// pareceria defeito de geração, não decisão velha.
	float MaisLongePequena = 0.0f;
	{
		const EscalaDaIlhaTeste::FRaioTemporario Pequena(20000.0f);
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			MaisLongePequena = FMath::Max(MaisLongePequena, Peca.RadiusUnits);
		}
	}

	float MaisLongeGrande = 0.0f;
	{
		const EscalaDaIlhaTeste::FRaioTemporario Grande(100000.0f);
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			MaisLongeGrande = FMath::Max(MaisLongeGrande, Peca.RadiusUnits);
		}
	}

	TestTrue(TEXT("A ilha pequena tem peças"), MaisLongePequena > 0.0f);

	// Cinco vezes maior, peças cinco vezes mais longe. É a RELAÇÃO que
	// importa: o número exato pode mudar por balanço, a proporção não.
	TestTrue(TEXT("As peças se espalham junto com a ilha"),
		MaisLongeGrande > MaisLongePequena * 4.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIlhaDeHojeNaoMudouTest,
	"BattleSquare.Environment.Island.AIlhaDeHojeNaoMudou",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIlhaDeHojeNaoMudouTest::RunTest(const FString&)
{
	// As frações foram escolhidas para dar EXATAMENTE os números de antes com
	// o raio de hoje. Converter absoluto em fração é refatoração, e
	// refatoração que muda o mundo do jogador não é refatoração.
	const EscalaDaIlhaTeste::FRaioTemporario Hoje(20000.0f);

	TSet<int32> Anéis;
	for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
	{
		Anéis.Add(FMath::RoundToInt(Peca.RadiusUnits));
	}

	// Os anéis históricos: 10.500, 12.000, 13.000, 15.000 e 17.000.
	for (const int32 Esperado : { 10500, 12000, 13000, 15000, 17000 })
	{
		TestTrue(*FString::Printf(TEXT("O anel de %d continua existindo"), Esperado),
			Anéis.Contains(Esperado));
	}

	return true;
}
