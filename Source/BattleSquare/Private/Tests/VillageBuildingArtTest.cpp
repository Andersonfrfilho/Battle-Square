// Copyright 2026 Anderson. All Rights Reserved.

#include "World/VillageBuildingArt.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

/**
 * AR8 — o prédio veste malha de verdade: a escala sai da CAIXA MEDIDA, e o
 * override é dado. Os contrapesos no centro: sem chave cai no cubo, e escala
 * nunca é zero (escala zero é malha atribuída e invisível).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillageBuildingScaleFromBoundsTest,
	"BattleSquare.World.VilaArte.EscalaSaiDaCaixaMedida",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageBuildingScaleFromBoundsTest::RunTest(const FString&)
{
	using VillageBuildingArt::ScaleToFootprint;

	// A conta ANTIGA dividia por 100 (o lado do cubo da engine). Uma malha de
	// 250 unidades numa pegada de 500 tem de escalar 2, nao 5.
	const FVector Caixa(250.0f, 250.0f, 400.0f);
	const FVector2D Pegada(500.0f, 500.0f);
	const FVector Escala = ScaleToFootprint(Caixa, Pegada, /*Altura=*/800.0f);
	TestTrue(TEXT("X escala pela caixa medida, nao por 100"),
		FMath::IsNearlyEqual(Escala.X, 2.0f, 0.001f));
	TestTrue(TEXT("Y idem"), FMath::IsNearlyEqual(Escala.Y, 2.0f, 0.001f));
	TestTrue(TEXT("Z pela altura pedida sobre a altura medida"),
		FMath::IsNearlyEqual(Escala.Z, 2.0f, 0.001f));

	// O CUBO da engine continua funcionando pela MESMA conta: caixa 100 e
	// pegada 500 dao 5 — exatamente o que a formula antiga dava.
	const FVector Cubo(100.0f, 100.0f, 100.0f);
	const FVector EscalaCubo = ScaleToFootprint(Cubo, Pegada, /*Altura=*/300.0f);
	TestTrue(TEXT("o cubo da engine da o mesmo que a conta antiga (X)"),
		FMath::IsNearlyEqual(EscalaCubo.X, 5.0f, 0.001f));
	TestTrue(TEXT("e o mesmo em Z"),
		FMath::IsNearlyEqual(EscalaCubo.Z, 3.0f, 0.001f));

	// CONTRAPESO: eixo sem medida cai em 1, NUNCA em zero. Escala zero e malha
	// atribuida e INVISIVEL — o defeito que o teste da vila ja cobra.
	const FVector Degenerada = ScaleToFootprint(FVector(0.0f, 0.0f, 0.0f), Pegada, 300.0f);
	TestTrue(TEXT("eixo sem medida cai em 1, nunca em zero"),
		FMath::IsNearlyEqual(Degenerada.X, 1.0f) && Degenerada.X > 0.0f);
	TestTrue(TEXT("nem em Z"), Degenerada.Z > 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillageBuildingMeshIsDataTest,
	"BattleSquare.World.VilaArte.MalhaVemDoDado",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageBuildingMeshIsDataTest::RunTest(const FString&)
{
	using namespace VillageBuildingArt;

	const TCHAR* Secao = TEXT("/Script/BattleSquare.Art");
	const TCHAR* Chave = BuildingConfigKey(EVillageBuilding::Mercado);

	FString Original;
	const bool bTinha = GConfig->GetString(Secao, Chave, Original, GGameIni);

	// SEM override: vazio — e o chamador cai no cubo (invariante 20, o jogo
	// fecha verde SEM pacote).
	GConfig->RemoveKey(Secao, Chave, GGameIni);
	TestTrue(TEXT("sem chave, caminho vazio — o predio fica no cubo"),
		MeshPathFor(EVillageBuilding::Mercado).IsEmpty());

	// COM override: o caminho do dado, sem invencao.
	GConfig->SetString(Secao, Chave, TEXT("/Game/KayKit/Buildings/SM_market.SM_market"), GGameIni);
	TestEqual(TEXT("com chave, devolve o caminho do dado"),
		MeshPathFor(EVillageBuilding::Mercado),
		FString(TEXT("/Game/KayKit/Buildings/SM_market.SM_market")));

	// Cada predio tem chave PROPRIA — duas nao colidem.
	TestNotEqual(TEXT("Mercado e Escola tem chaves distintas"),
		FString(BuildingConfigKey(EVillageBuilding::Mercado)),
		FString(BuildingConfigKey(EVillageBuilding::Escola)));

	if (bTinha) { GConfig->SetString(Secao, Chave, *Original, GGameIni); }
	else { GConfig->RemoveKey(Secao, Chave, GGameIni); }

	return true;
}
