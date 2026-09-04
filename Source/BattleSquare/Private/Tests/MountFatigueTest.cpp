// Copyright 2026 Anderson. All Rights Reserved.

#include "World/MountFatigue.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"

/**
 * MT2 — subir cansa mais que descer, na proporção dos pesos de custo; e o
 * cansaço varia com a inclinação, não é constante por metro.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMountFatigueBySlopeTest,
	"BattleSquare.World.Montaria.CansacoSobeMaisNaSubida",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountFatigueBySlopeTest::RunTest(const FString&)
{
	using MountFatigue::FatigueForStretch;

	const float Comprimento = 1000.0f;
	const float Taxa = 0.001f;
	const float PesoSubida = IslandGeography::UphillCostWeight();
	const float PesoDescida = IslandGeography::DownhillCostWeight();

	const float Subindo = FatigueForStretch(Comprimento, PesoSubida, Taxa);
	const float Descendo = FatigueForStretch(Comprimento, PesoDescida, Taxa);

	// SUBIR CANSA MAIS que descer, o mesmo comprimento.
	TestTrue(TEXT("subir cansa mais que descer"), Subindo > Descendo);

	// Na PROPORÇÃO DOS PESOS — não uma conta nova.
	TestTrue(TEXT("a proporcao do cansaco e a dos pesos"),
		FMath::IsNearlyEqual(Subindo / Descendo, PesoSubida / PesoDescida, 0.001f));

	// VARIA com a inclinação — não é constante por metro: dobrar o peso dobra
	// o cansaço do mesmo comprimento.
	TestTrue(TEXT("cansaco varia com a inclinacao"),
		FatigueForStretch(Comprimento, PesoSubida * 2.0f, Taxa) > Subindo * 1.9f);

	// Nunca negativo.
	TestEqual(TEXT("comprimento zero nao cansa"), FatigueForStretch(0.0f, PesoSubida, Taxa), 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMountFatigueByWeightTest,
	"BattleSquare.World.Montaria.PesoCansaMaisNuncaBloqueia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountFatigueByWeightTest::RunTest(const FString&)
{
	using namespace MountFatigue;

	const float Referencia = 100.0f;
	const float Teto = 3.0f;

	const float MultLeve = WeightMultiplier(50.0f, Referencia, Teto);
	const float MultPesado = WeightMultiplier(200.0f, Referencia, Teto);

	// PESADO cansa mais que LEVE — ritmos diferentes.
	TestTrue(TEXT("pet pesado tem multiplicador maior"), MultPesado > MultLeve);

	// O CONTRAPESO MANDATORIO: o multiplicador e SEMPRE finito — peso nunca
	// torna o trajeto impossivel. O pet mais pesado do mundo continua no teto,
	// nunca em infinito.
	const float MultAbsurdo = WeightMultiplier(100000.0f, Referencia, Teto);
	TestTrue(TEXT("peso absurdo trava no teto, nunca infinito"),
		MultAbsurdo <= Teto + KINDA_SMALL_NUMBER);
	TestTrue(TEXT("o teto e finito e positivo"), MultAbsurdo > 0.0f);

	// Logo, a fadiga do trajeto continua FINITA para qualquer peso — completavel.
	const float FadigaBase = 500.0f;
	TestTrue(TEXT("fadiga do pet pesadissimo ainda e finita"),
		FatigueWithWeight(FadigaBase, MultAbsurdo) < FadigaBase * (Teto + 1.0f));

	// Peso de referencia degenerado nao divide por zero.
	TestEqual(TEXT("referencia zero da multiplicador neutro"),
		WeightMultiplier(200.0f, 0.0f, Teto), 1.0f);

	return true;
}
