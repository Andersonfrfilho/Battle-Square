// Copyright 2026 Anderson. All Rights Reserved.

#include "World/MountedMovement.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"

/**
 * MT1 — o MESMO trecho, a pé e montado, dá velocidade maior montado; e a
 * proporção subida/descida continua sendo a dos pesos de custo, não uma conta
 * nova.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMountedFasterSameProportionTest,
	"BattleSquare.World.Montaria.MontadoMaisRapidoMesmaProporcao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountedFasterSameProportionTest::RunTest(const FString&)
{
	using namespace MountedMovement;

	const float OnFoot = 600.0f;
	const float Mult = 1.8f;
	const float PesoSubida = IslandGeography::UphillCostWeight();
	const float PesoDescida = IslandGeography::DownhillCostWeight();

	const float BaseMontado = MountedBaseSpeed(OnFoot, Mult);

	// MONTADO É MAIS RÁPIDO no MESMO trecho — subida e descida.
	TestTrue(TEXT("montado sobe mais rapido que a pe"),
		SpeedOnStretch(BaseMontado, PesoSubida) > SpeedOnStretch(OnFoot, PesoSubida));
	TestTrue(TEXT("montado desce mais rapido que a pe"),
		SpeedOnStretch(BaseMontado, PesoDescida) > SpeedOnStretch(OnFoot, PesoDescida));

	// A PROPORÇÃO subida/descida é a dos PESOS, não uma conta nova — igual a pé
	// e montado (o multiplicador se cancela).
	const float ProporcaoAPe =
		SpeedOnStretch(OnFoot, PesoDescida) / SpeedOnStretch(OnFoot, PesoSubida);
	const float ProporcaoMontado =
		SpeedOnStretch(BaseMontado, PesoDescida) / SpeedOnStretch(BaseMontado, PesoSubida);
	const float ProporcaoDosPesos = PesoSubida / PesoDescida;
	TestTrue(TEXT("a proporcao a pe e a dos pesos"),
		FMath::IsNearlyEqual(ProporcaoAPe, ProporcaoDosPesos, 0.001f));
	TestTrue(TEXT("a proporcao montado e a MESMA — o multiplicador se cancela"),
		FMath::IsNearlyEqual(ProporcaoMontado, ProporcaoAPe, 0.001f));

	// Multiplicador <= 1 nunca deixa montar mais lento.
	TestTrue(TEXT("montar nunca atrapalha"),
		MountedBaseSpeed(OnFoot, 0.5f) >= OnFoot);

	return true;
}
