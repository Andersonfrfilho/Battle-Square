// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/TreeGrowth.h"
#include "Misc/AutomationTest.h"

/**
 * MV2 — a árvore cresce com a idade do MUNDO: a mesma árvore, medida em duas
 * idades do mundo, dá duas escalas diferentes; e nunca cresce para sempre.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreeGrowthWithWorldAgeTest,
	"BattleSquare.Environment.Crescimento.ArvoreCresceComOMundo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTreeGrowthWithWorldAgeTest::RunTest(const FString&)
{
	using namespace TreeGrowth;

	FGrowthConfig Config;
	Config.DaysToMaturity = 30;
	Config.SaplingScale = 0.35f;

	// O ACEITE: a MESMA árvore (mesma config) em duas idades do mundo dá duas
	// escalas DIFERENTES — hoje daria sempre a mesma, sem o terceiro argumento.
	const float NoDiaZero = ScaleFactorFor(0, Config);
	const float NaMetade = ScaleFactorFor(15, Config);
	const float NaMaturidade = ScaleFactorFor(30, Config);
	TestTrue(TEXT("muda no dia zero"), FMath::IsNearlyEqual(NoDiaZero, 0.35f));
	TestTrue(TEXT("cresceu na metade do caminho"), NaMetade > NoDiaZero);
	TestTrue(TEXT("adulta na maturidade"), FMath::IsNearlyEqual(NaMaturidade, 1.0f));

	// SATURA: um mundo velho demais não faz árvore gigante — teto em 1.0.
	const float MuitoVelho = ScaleFactorFor(3000, Config);
	TestTrue(TEXT("satura no adulto, nao cresce para sempre"),
		FMath::IsNearlyEqual(MuitoVelho, 1.0f));

	// MONÓTONO: mundo mais velho nunca encolhe a mata.
	float Anterior = -1.0f;
	for (int32 Dia = 0; Dia <= 60; Dia += 5)
	{
		const float F = ScaleFactorFor(Dia, Config);
		TestTrue(TEXT("nunca encolhe com o tempo"), F >= Anterior);
		Anterior = F;
	}

	// CONTRAPESO da idade desconhecida (MV1): idade negativa NÃO some com a
	// árvore nem a deixa muda — cai no adulto, mata madura por não saber.
	TestTrue(TEXT("idade desconhecida vira mata adulta, nunca vazio"),
		FMath::IsNearlyEqual(ScaleFactorFor(-1, Config), 1.0f));

	// Config degenerada não divide por zero.
	FGrowthConfig Degenerada; Degenerada.DaysToMaturity = 0;
	TestTrue(TEXT("maturidade em zero dia nasce adulta"),
		FMath::IsNearlyEqual(ScaleFactorFor(10, Degenerada), 1.0f));

	return true;
}
