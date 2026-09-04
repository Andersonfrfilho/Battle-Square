// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SettlementEconomy.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

/**
 * mae-natureza MN5 — o preco do assentamento e numero de config: mudar o valor
 * no arquivo muda PricePercent sem recompilar (mesmo teste-padrao de MV4).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettlementPriceIsConfigTest,
	"BattleSquare.World.MaeNatureza.PrecoEConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettlementPriceIsConfigTest::RunTest(const FString&)
{
	const TCHAR* Secao = TEXT("/Script/BattleSquare.SettlementEconomy");
	const TCHAR* Chave = TEXT("AgioDaCidade");

	int32 Original = 160;
	const bool bTinha = GConfig->GetInt(Secao, Chave, Original, GGameIni);

	// O preco da Academia na Cidade Grande sai de AgioDaCidade. A Cidade Grande
	// e autossuficiente (tem Academia), entao este servico sempre existe.
	GConfig->SetInt(Secao, Chave, 160, GGameIni);
	const int32 PrecoBaixo =
		SettlementEconomy::PricePercent(ESettlementKind::CidadeGrande, ESettlementService::Academia);

	GConfig->SetInt(Secao, Chave, 320, GGameIni);
	const int32 PrecoAlto =
		SettlementEconomy::PricePercent(ESettlementKind::CidadeGrande, ESettlementService::Academia);

	TestTrue(TEXT("a Cidade Grande cobra Academia"), PrecoBaixo > 0);
	TestEqual(TEXT("o preco baixo e o do arquivo"), PrecoBaixo, 160);
	TestEqual(TEXT("o preco alto e o novo do arquivo"), PrecoAlto, 320);
	TestTrue(TEXT("mudar o numero no arquivo mudou o preco, sem recompilar"),
		PrecoAlto > PrecoBaixo);

	if (bTinha) { GConfig->SetInt(Secao, Chave, Original, GGameIni); }
	else { GConfig->RemoveKey(Secao, Chave, GGameIni); }

	return true;
}
