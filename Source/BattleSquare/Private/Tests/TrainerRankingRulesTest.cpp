// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TrainerRankingRules.h"
#include "Misc/AutomationTest.h"
#include "World/SettlementEconomy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankingOnlyClimbsTest,
	"BattleSquare.Meta.Ranking.SoSobe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRankingOnlyClimbsTest::RunTest(const FString&)
{
	// A regra inteira numa frase: vitória soma, e NADA subtrai. O desconto por
	// derrota é a metade irreversível — tirar ponto que alguém já contou — e o
	// dono do mundo decidiu que o ranking é placar, não cancela (decisão 58).
	FTrainerProfile Perfil;
	TestEqual(TEXT("nasce em zero"), Perfil.RankingPoints, 0);

	FTrainerRankingRules::AwardArenaVictory(Perfil);
	TestEqual(TEXT("uma vitoria vale a constante nomeada"),
		Perfil.RankingPoints, FTrainerRankingRules::PointsPerArenaVictory);

	FTrainerRankingRules::AwardArenaVictory(Perfil);
	FTrainerRankingRules::AwardArenaVictory(Perfil);
	TestEqual(TEXT("tres vitorias, tres constantes"),
		Perfil.RankingPoints, 3 * FTrainerRankingRules::PointsPerArenaVictory);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankingPrizeComesFromTheTableTest,
	"BattleSquare.Meta.Ranking.OPremioVemDaTabela",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRankingPrizeComesFromTheTableTest::RunTest(const FString&)
{
	// A DIFERENÇA que a spec desenhou: as arenas das vilas são o dinheiro do
	// COMEÇO, e o prêmio grande mora na cidade — é ele que dava nome à
	// fronteira. Afirmado CONTRA a tabela, nunca como número digitado aqui.
	const ESettlementKind Lugares[] = {
		ESettlementKind::VilaInicial, ESettlementKind::VilaDaAcademia,
		ESettlementKind::VilaDoMercado, ESettlementKind::CidadeGrande,
		ESettlementKind::PostoDeFronteira,
	};

	for (ESettlementKind Lugar : Lugares)
	{
		const bool bTemArena = SettlementEconomy::Offers(
			Lugar, ESettlementService::PremioDeRanking);

		TestEqual(TEXT("premio zero e exatamente 'sem Arena'"),
			SettlementEconomy::RankingPrize(Lugar) > 0, bTemArena);
	}

	TestTrue(TEXT("a cidade paga MAIS que a vila inicial"),
		SettlementEconomy::RankingPrize(ESettlementKind::CidadeGrande)
			> SettlementEconomy::RankingPrize(ESettlementKind::VilaInicial));

	return true;
}
