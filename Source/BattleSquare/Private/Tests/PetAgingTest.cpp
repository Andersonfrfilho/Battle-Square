// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetAging.h"
#include "Misc/AutomationTest.h"

/**
 * MV6 — o pet envelhece (decisões 32/33/34): idade é subtração da idade do
 * mundo (offline-safe), morre de velho, e o cuidado ATIVO estica a vida. O
 * contrapeso — recém-nascido vivo, e o tempo sozinho não salva — no centro.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetAgesAndDiesTest,
	"BattleSquare.Meta.Envelhecimento.EnvelheceMorreEOCuidadoEstica",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetAgesAndDiesTest::RunTest(const FString&)
{
	using namespace PetAging;

	FAgingConfig Config;
	Config.LifespanDays = 120;
	Config.CareDaysPerAct = 2.0f;

	// IDADE = subtração da idade do mundo (decisão 33, offline-safe): nasceu no
	// dia 50, hoje é dia 80 -> 30 dias, mesmo sem ninguém ter jogado no meio.
	TestEqual(TEXT("idade e a subtracao da idade do mundo"),
		RawAgeInDays(50, 80), 30);
	TestEqual(TEXT("relogio torto nunca da idade negativa"),
		RawAgeInDays(80, 50), 0);

	// RECÉM-NASCIDO está VIVO (o contrapeso): idade zero nunca é morte.
	TestFalse(TEXT("recem-nascido nao morre de velho"),
		IsDeceased(EffectiveAgeInDays(0, 0, Config), Config));

	// MORRE DE VELHO (decisão 32): sem cuidado, ao alcançar o tempo de vida.
	TestFalse(TEXT("vespera do fim, ainda vivo"),
		IsDeceased(EffectiveAgeInDays(119, 0, Config), Config));
	TestTrue(TEXT("no tempo de vida, morreu de velho"),
		IsDeceased(EffectiveAgeInDays(120, 0, Config), Config));

	// O CUIDADO ATIVO ESTICA A VIDA (decisão 34): um pet de 120 dias que
	// recebeu cuidado continua VIVO — o cuidado desfez parte do envelhecimento.
	const int32 IdadeComCuidado = EffectiveAgeInDays(120, /*CareActs=*/10, Config);
	TestTrue(TEXT("o cuidado rejuvenesce o relogio da morte"), IdadeComCuidado < 120);
	TestFalse(TEXT("pet bem cuidado vive alem do tempo natural"),
		IsDeceased(IdadeComCuidado, Config));

	// É a AÇÃO que conta, não o tempo: sem ato de cuidar, desconto zero — o
	// tempo sozinho NÃO salva (o passivo que a decisão 34 rejeita).
	TestEqual(TEXT("sem cuidar, nenhum desconto"),
		EffectiveAgeInDays(100, 0, Config), 100);

	// Cuidar não torna o pet mais novo que recém-nascido.
	TestEqual(TEXT("cuidado nao leva a idade abaixo de zero"),
		EffectiveAgeInDays(3, 100, Config), 0);

	// Config degenerada: tempo de vida zero faz imortal, nao natimorto.
	FAgingConfig SemVida; SemVida.LifespanDays = 0;
	TestFalse(TEXT("tempo de vida zero nao mata ninguem"),
		IsDeceased(EffectiveAgeInDays(9999, 0, SemVida), SemVida));

	return true;
}
