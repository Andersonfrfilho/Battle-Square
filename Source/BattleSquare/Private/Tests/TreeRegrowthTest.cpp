// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TreeRegrowth.h"
#include "Misc/AutomationTest.h"

/**
 * MV3 — a árvore cortada é exceção com PRAZO: cortada antes do prazo, rebrotada
 * depois. Os dois lados do aceite, num teste só.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreeRegrowthDeadlineTest,
	"BattleSquare.World.Rebrota.CortadaAteOPrazo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTreeRegrowthDeadlineTest::RunTest(const FString&)
{
	using TreeRegrowth::HasRegrown;

	const int32 Corte = 100;   // idade do mundo no dia do corte
	const int32 Prazo = 7;     // rebrota em sete dias de mundo

	// ANTES do prazo: continua cortada — reabrir o jogo antes nao a traz de volta.
	TestFalse(TEXT("no dia do corte, cortada"), HasRegrown(Corte, 100, Prazo));
	TestFalse(TEXT("um dia depois, ainda cortada"), HasRegrown(Corte, 101, Prazo));
	TestFalse(TEXT("vespera do prazo, ainda cortada"), HasRegrown(Corte, 106, Prazo));

	// NO prazo e DEPOIS: rebrotou sozinha — reabrir depois a traz de volta.
	TestTrue(TEXT("no prazo cheio, rebrotou"), HasRegrown(Corte, 107, Prazo));
	TestTrue(TEXT("bem depois, rebrotada"), HasRegrown(Corte, 500, Prazo));

	// Prazo degenerado nunca trava a mata cortada para sempre.
	TestTrue(TEXT("prazo zero rebrota na hora"), HasRegrown(Corte, 100, 0));
	TestTrue(TEXT("prazo negativo rebrota na hora"), HasRegrown(Corte, 100, -3));

	// Relógio incoerente: na duvida continua cortada, nao rebrota por engano.
	TestFalse(TEXT("agora antes do corte nao rebrota"), HasRegrown(Corte, 90, Prazo));

	// QUANTO FALTA (MV5, o que a tela mostra): prazo inteiro no dia do corte,
	// menos a cada dia, zero quando rebrota — e nunca negativo depois.
	using TreeRegrowth::DaysUntilRegrowth;
	TestEqual(TEXT("no corte, falta o prazo inteiro"), DaysUntilRegrowth(Corte, 100, Prazo), 7);
	TestEqual(TEXT("na metade, falta a metade"), DaysUntilRegrowth(Corte, 103, Prazo), 4);
	TestEqual(TEXT("no prazo, falta zero"), DaysUntilRegrowth(Corte, 107, Prazo), 0);
	TestEqual(TEXT("passado o prazo, falta zero, nunca negativo"),
		DaysUntilRegrowth(Corte, 200, Prazo), 0);

	return true;
}
