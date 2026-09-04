// Copyright 2026 Anderson. All Rights Reserved.

#include "World/BorderGate.h"
#include "Misc/AutomationTest.h"

/**
 * MB1 — o Posto de Fronteira barra sem ranking e libera com ranking, no MESMO
 * ponto; e nenhuma outra passagem trava (o contrapeso).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBorderGateRankingTest,
	"BattleSquare.World.Fronteira.SoOPostoTrava",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBorderGateRankingTest::RunTest(const FString&)
{
	using namespace BorderGate;

	// O MESMO Posto, dois estados: sem ranking barra, com ranking libera.
	TestFalse(TEXT("Posto sem ranking vencido barra"),
		AllowsPassage(ESettlementKind::PostoDeFronteira, /*bWon=*/false));
	TestTrue(TEXT("o MESMO Posto com ranking vencido libera"),
		AllowsPassage(ESettlementKind::PostoDeFronteira, /*bWon=*/true));

	// O CONTRAPESO: nenhuma outra passagem trava, nem sem ranking.
	for (ESettlementKind Kind : {
			ESettlementKind::VilaInicial, ESettlementKind::VilaDaAcademia,
			ESettlementKind::VilaDoMercado, ESettlementKind::CidadeGrande })
	{
		TestTrue(TEXT("passagem comum e sempre livre, mesmo sem ranking"),
			AllowsPassage(Kind, /*bWon=*/false));
	}

	return true;
}
