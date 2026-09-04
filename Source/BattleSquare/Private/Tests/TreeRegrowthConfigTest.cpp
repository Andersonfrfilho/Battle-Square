// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TreeRegrowth.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

/**
 * MV4 — o prazo de rebrota é NÚMERO DE CONFIGURAÇÃO: mudar o valor no arquivo
 * muda o tempo de rebrota sem recompilar. O teste lê dois valores diferentes da
 * MESMA chave e confere dois prazos diferentes, pela mesma via que o jogo usa
 * (`GConfig` sobre `[/Script/BattleSquare.WorldAge]`).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreeRegrowthDeadlineIsConfigTest,
	"BattleSquare.World.Rebrota.PrazoEConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTreeRegrowthDeadlineIsConfigTest::RunTest(const FString&)
{
	const TCHAR* Secao = TEXT("/Script/BattleSquare.WorldAge");
	const TCHAR* Chave = TEXT("TreeRegrowthDeadlineDays");

	// Guarda o valor real para restaurar — o teste não pode sujar a config.
	int32 Original = 7;
	const bool bTinha = GConfig->GetInt(Secao, Chave, Original, GGameIni);

	auto PrazoLido = [&]() -> int32
	{
		int32 Dias = 7;
		GConfig->GetInt(Secao, Chave, Dias, GGameIni);
		return Dias;
	};

	const int32 Corte = 100;

	// PRAZO CURTO: três dias. Uma árvore cortada no dia 100 já rebrotou no 103.
	GConfig->SetInt(Secao, Chave, 3, GGameIni);
	TestEqual(TEXT("o arquivo agora diz 3"), PrazoLido(), 3);
	TestTrue(TEXT("com prazo 3, rebrotou no dia 103"),
		TreeRegrowth::HasRegrown(Corte, 103, PrazoLido()));

	// PRAZO LONGO: trinta dias. A MESMA árvore, no MESMO dia 103, continua
	// cortada — só o número no arquivo mudou, sem recompilar nada.
	GConfig->SetInt(Secao, Chave, 30, GGameIni);
	TestEqual(TEXT("o arquivo agora diz 30"), PrazoLido(), 30);
	TestFalse(TEXT("com prazo 30, ainda cortada no dia 103"),
		TreeRegrowth::HasRegrown(Corte, 103, PrazoLido()));

	// Restaura para não afetar outros testes.
	if (bTinha)
	{
		GConfig->SetInt(Secao, Chave, Original, GGameIni);
	}
	else
	{
		GConfig->RemoveKey(Secao, Chave, GGameIni);
	}

	return true;
}
