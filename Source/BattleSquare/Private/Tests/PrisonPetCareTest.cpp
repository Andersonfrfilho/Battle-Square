// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PrisonPetCare.h"
#include "Meta/PetAging.h"
#include "Misc/AutomationTest.h"

/**
 * CR10 — o pet confiscado envelhece na prisão pelo MESMO relógio (PetAging), e
 * sem cuidador não recebe cuidado nenhum. Compõe as duas regras, sem inventar
 * um segundo envelhecimento.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPrisonPetAgesAndCareDecidesTest,
	"BattleSquare.Meta.Prisao.PetEnvelheceEAlguemCuida",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPrisonPetAgesAndCareDecidesTest::RunTest(const FString&)
{
	using namespace PetAging;
	using PrisonPetCare::EffectiveCareActs;

	FAgingConfig Config;
	Config.LifespanDays = 120;
	Config.CareDaysPerAct = 2.0f;

	// O pet foi confiscado velho: nasceu no dia 0, o mundo está no dia 120.
	const int32 IdadeCrua = RawAgeInDays(/*Genesis=*/0, /*Agora=*/120);
	TestEqual(TEXT("o pet preso envelheceu pelo relogio do mundo"), IdadeCrua, 120);

	// SEM CUIDADOR (o contrapeso): o abandono na cela não recebe cuidado
	// nenhum, mesmo que atos tenham sido registrados por engano — envelhece a
	// plena velocidade e morre de velho.
	const int32 CuidadoAbandonado = EffectiveCareActs(/*bHasCaretaker=*/false, /*Given=*/10);
	TestEqual(TEXT("preso sem cuidador nao recebe cuidado"), CuidadoAbandonado, 0);
	TestTrue(TEXT("o pet abandonado na cela morre de velho"),
		IsDeceased(EffectiveAgeInDays(IdadeCrua, CuidadoAbandonado, Config), Config));

	// COM CUIDADOR que assumiu a tarefa: os atos contam, e o MESMO pet, no
	// MESMO dia, continua vivo — alguém decidiu cuidar.
	const int32 CuidadoAssumido = EffectiveCareActs(/*bHasCaretaker=*/true, /*Given=*/10);
	TestEqual(TEXT("com cuidador, os atos contam"), CuidadoAssumido, 10);
	TestFalse(TEXT("o pet cuidado na prisao nao morre de velho"),
		IsDeceased(EffectiveAgeInDays(IdadeCrua, CuidadoAssumido, Config), Config));

	// Cuidador sem ato nenhum ainda é zero — cuidar é a AÇÃO, não a presença.
	TestEqual(TEXT("cuidador que nao age nao adianta nada"),
		EffectiveCareActs(true, 0), 0);

	return true;
}
