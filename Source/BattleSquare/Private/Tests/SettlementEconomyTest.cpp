// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/SettlementEconomy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettlementEconomyCadaVilaTemUmLadoDoLacoTest,
	"BattleSquare.SettlementEconomy.CadaVilaTemUmLadoDoLaco",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettlementEconomyCadaVilaTemUmLadoDoLacoTest::RunTest(const FString& Parameters)
{
	// Vila sem papel econômico é vila que se visita uma vez. Este teste é o
	// que impede uma vila nova de nascer sem lado do laço.
	TestTrue(TEXT("a vila inicial cura"),
		SettlementEconomy::Offers(ESettlementKind::VilaInicial, ESettlementService::Cura));
	TestTrue(TEXT("a vila da academia treina"),
		SettlementEconomy::Offers(ESettlementKind::VilaDaAcademia, ESettlementService::Academia));
	TestTrue(TEXT("a vila do mercado compra pet"),
		SettlementEconomy::Offers(ESettlementKind::VilaDoMercado, ESettlementService::Venda));

	// A ausência é o desenho: posto com comércio vira destino, e a fronteira
	// deixa de ser porta.
	TestFalse(TEXT("o posto de fronteira não cura"),
		SettlementEconomy::Offers(ESettlementKind::PostoDeFronteira, ESettlementService::Cura));
	TestFalse(TEXT("nem compra"),
		SettlementEconomy::Offers(ESettlementKind::PostoDeFronteira, ESettlementService::Venda));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettlementEconomyACuraDeCasaEhDeGracaTest,
	"BattleSquare.SettlementEconomy.ACuraDeCasaEhDeGraca",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettlementEconomyACuraDeCasaEhDeGracaTest::RunTest(const FString& Parameters)
{
	// O chão da economia. Sem ele fecha o laço de morte: "não tenho dinheiro
	// para curar para poder lutar para ganhar dinheiro".
	TestEqual(TEXT("curar em casa não custa nada"),
		SettlementEconomy::PricePercent(ESettlementKind::VilaInicial, ESettlementService::Cura), 0);

	// E o zero de "de graça" não pode ser o mesmo zero de "não oferece": quem
	// confundir os dois faz a cidade curar de graça.
	TestTrue(TEXT("a cidade cobra pela cura"),
		SettlementEconomy::PricePercent(ESettlementKind::CidadeGrande, ESettlementService::Cura) > 0);
	TestFalse(TEXT("e o posto nem oferece"),
		SettlementEconomy::Offers(ESettlementKind::PostoDeFronteira, ESettlementService::Cura));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettlementEconomyACidadeTemTudoEEhAMaisCaraTest,
	"BattleSquare.SettlementEconomy.ACidadeTemTudoEEhAMaisCara",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettlementEconomyACidadeTemTudoEEhAMaisCaraTest::RunTest(const FString& Parameters)
{
	const ESettlementService Servicos[] = {
		ESettlementService::Cura,
		ESettlementService::Academia,
		ESettlementService::Venda,
		ESettlementService::PremioDeRanking,
	};

	for (const ESettlementService Servico : Servicos)
	{
		TestTrue(TEXT("a cidade oferece tudo"),
			SettlementEconomy::Offers(ESettlementKind::CidadeGrande, Servico));
	}

	// A decisão que mantém as três vilas vivas depois que a cidade abre.
	// Cidade barata e completa mataria as vilas no instante em que abrisse, e
	// a região viraria um corredor até ela.
	TestTrue(TEXT("a academia da cidade é mais cara que a da vila"),
		SettlementEconomy::PricePercent(ESettlementKind::CidadeGrande, ESettlementService::Academia)
			> SettlementEconomy::PricePercent(ESettlementKind::VilaDaAcademia, ESettlementService::Academia));

	TestTrue(TEXT("e ela paga menos pelo pet que o mercado"),
		SettlementEconomy::PayoutPercent(ESettlementKind::CidadeGrande, ESettlementService::Venda)
			< SettlementEconomy::PayoutPercent(ESettlementKind::VilaDoMercado, ESettlementService::Venda));

	// O que a cidade tem de melhor é o único que ela deve ter: o prêmio grande
	// é a fonte principal, e é ele que abre a fronteira.
	TestTrue(TEXT("mas o prêmio grande é dela"),
		SettlementEconomy::PayoutPercent(ESettlementKind::CidadeGrande, ESettlementService::PremioDeRanking)
			> SettlementEconomy::PayoutPercent(ESettlementKind::VilaInicial, ESettlementService::PremioDeRanking));

	return true;
}
