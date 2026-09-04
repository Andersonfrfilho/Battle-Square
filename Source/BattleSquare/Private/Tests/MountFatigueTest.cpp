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
