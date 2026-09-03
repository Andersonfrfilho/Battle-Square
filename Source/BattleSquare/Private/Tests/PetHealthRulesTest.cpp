// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetHealthRules.h"
#include "Misc/AutomationTest.h"

/**
 * A VIDA QUE PERSISTE (decisão 61) — e os dois modos de falhar que ela nunca
 * pode ter: matar o save antigo, e começar uma luta já perdida.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetHealthOldSaveIsFullTest,
	"BattleSquare.Meta.Vida.SaveAntigoEstaCheio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetHealthOldSaveIsFullTest::RunTest(const FString&)
{
	// O CONTRAPESO PRIMEIRO: o campo novo não pode matar os pets de quem já
	// jogava. Negativo é "não informado", e não informado é CHEIO — se o
	// padrão fosse zero, todo save antigo acordaria com a coleção morta.
	TestEqual(TEXT("save antigo entra com a vida cheia"),
		FPetHealthRules::StartingHealthFor(-1, 200), 200);
	TestFalse(TEXT("e nao conta como machucado"), FPetHealthRules::IsHurt(-1));

	// Teto inválido também devolve cheio: pet que não se mede não sai morto
	// da medição.
	TestEqual(TEXT("teto invalido nao vira zero"),
		FPetHealthRules::PercentAfterBattle(50, 0), 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetHealthCarriesAndFloorsTest,
	"BattleSquare.Meta.Vida.CarregaEComPiso",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetHealthCarriesAndFloorsTest::RunTest(const FString&)
{
	// O CAMINHO REDONDO: sai da batalha com 60 de 200 → 30% → volta com 60 no
	// MESMO teto. A porcentagem é o formato porque o teto muda com o nível, e
	// a fração sobrevive à subida.
	const int32 Percentual = FPetHealthRules::PercentAfterBattle(60, 200);
	TestEqual(TEXT("60 de 200 sao 30%"), Percentual, 30);
	TestEqual(TEXT("e 30% de 200 voltam a ser 60"),
		FPetHealthRules::StartingHealthFor(Percentual, 200), 60);
	TestTrue(TEXT("30% e machucado"), FPetHealthRules::IsHurt(Percentual));

	// E A FRAÇÃO SOBREVIVE À SUBIDA: o mesmo 30% num teto que cresceu para
	// 300 entra com 90 — meia-vida de um pet forte não vira meia-vida de um
	// pet fraco.
	TestEqual(TEXT("30% de um teto maior escala junto"),
		FPetHealthRules::StartingHealthFor(Percentual, 300), 90);

	// O PISO DE UM: batalha que começa com zero é luta perdida antes do
	// primeiro turno. Zero por cento entra com 1, nunca com 0.
	TestEqual(TEXT("0% entra com UM, nunca com zero"),
		FPetHealthRules::StartingHealthFor(0, 200), 1);

	// E cheio é cheio, sem aritmética esquisita nas pontas.
	TestEqual(TEXT("100% entra cheio"),
		FPetHealthRules::StartingHealthFor(100, 200), 200);
	TestEqual(TEXT("vida acima do teto satura em 100%"),
		FPetHealthRules::PercentAfterBattle(999, 200), 100);

	return true;
}
