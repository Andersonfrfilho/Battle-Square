// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/WorldAge.h"
#include "Misc/AutomationTest.h"

/**
 * MV1 (cliente) — o contrapeso: idade desconhecida NUNCA vira zero.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldAgeUnknownIsNotZeroTest,
	"BattleSquare.Meta.IdadeDoMundo.DesconhecidaNaoEZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldAgeUnknownIsNotZeroTest::RunTest(const FString&)
{
	using namespace WorldAge;

	// O ESTADO INICIAL e o de FALHA sao ambos "desconhecida" — o fallback.
	const FWorldAge Inicial;
	TestFalse(TEXT("o estado inicial e desconhecido"), Inicial.bKnown);
	TestFalse(TEXT("Unknown() e desconhecido"), Unknown().bKnown);

	// O CORACAO DO CONTRAPESO: desconhecida e um mundo de idade ZERO sao
	// coisas DIFERENTES na tela. Se o texto fosse igual, backend fora do ar
	// pareceria um mundo recem-nascido — o defeito que a task proibe.
	const FText Desconhecida = Describe(Unknown());
	const FText MundoZero = Describe(Known(0));
	TestTrue(TEXT("desconhecida nao le como um mundo de idade zero"),
		!Desconhecida.EqualTo(MundoZero));
	TestTrue(TEXT("desconhecida diz 'desconhecida'"),
		Desconhecida.ToString().Contains(TEXT("desconhecida")));

	// UMA IDADE QUE CHEGOU: conhecida, com o numero.
	const FWorldAge Chegou = Known(42);
	TestTrue(TEXT("Known e conhecida"), Chegou.bKnown);
	TestEqual(TEXT("Known guarda o numero"), Chegou.AgeInDays, 42);
	TestTrue(TEXT("o texto conhecido traz o numero"),
		Describe(Chegou).ToString().Contains(TEXT("42")));

	// Idade negativa (leitura torta) e pisada em zero, mas continua CONHECIDA.
	TestEqual(TEXT("idade negativa vira zero"), Known(-5).AgeInDays, 0);

	return true;
}
