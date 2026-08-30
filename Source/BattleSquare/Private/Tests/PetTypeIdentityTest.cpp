// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetTypeIdentity.h"
#include "Misc/AutomationTest.h"

// O tipo tem DOIS eixos, e ler os dois é o que permite a tabela composta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTypeIdentityReadsBothAxesTest,
	"BattleSquare.Balance.TypeIdentity.ReadsBothAxes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTypeIdentityReadsBothAxesTest::RunTest(const FString& Parameters)
{
	const FPetTypeIdentity Par = FPetTypeIdentity::Parse(TEXT("Psiquica/Terra"));
	TestTrue(TEXT("O par é válido"), Par.IsValid());
	TestEqual(TEXT("Escola lida"), Par.School, FString(TEXT("Psiquica")));
	TestEqual(TEXT("Elemento lido"), Par.Element, FString(TEXT("Terra")));
	TestEqual(TEXT("E volta à forma canônica"), Par.ToTypeString(), FString(TEXT("Psiquica/Terra")));

	// Caixa e espaço não podem decidir se um pet tem tipo: o catálogo do
	// backend não promete grafia.
	const FPetTypeIdentity Bagunçado = FPetTypeIdentity::Parse(TEXT(" fisica / TERRA "));
	TestEqual(TEXT("Escola normalizada"), Bagunçado.School, FString(TEXT("Fisica")));
	TestEqual(TEXT("Elemento normalizado"), Bagunçado.Element, FString(TEXT("Terra")));

	return true;
}

// Os nomes ANTIGOS continuam valendo, e não por gentileza: eles já foram
// ASSINADOS. Recusá-los invalidaria pets que existem, e dado assinado é
// justamente o que não se reescreve de fora.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSignedLegacyNamesStillResolveTest,
	"BattleSquare.Balance.TypeIdentity.SignedLegacyNamesStillResolve",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSignedLegacyNamesStillResolveTest::RunTest(const FString& Parameters)
{
	struct FCaso { const TCHAR* Antigo; const TCHAR* Escola; const TCHAR* Elemento; };
	const FCaso Casos[] = {
		{ TEXT("Fogo"),     TEXT("Natural"),  TEXT("Fogo")   },
		{ TEXT("Agua"),     TEXT("Natural"),  TEXT("Agua")   },
		{ TEXT("Planta"),   TEXT("Natural"),  TEXT("Planta") },
		{ TEXT("Inseto"),   TEXT("Fisica"),   TEXT("Planta") },
		{ TEXT("Caverna"),  TEXT("Fisica"),   TEXT("Terra")  },
		{ TEXT("Psiquico"), TEXT("Psiquica"), TEXT("Agua")   },
		{ TEXT("Magico"),   TEXT("Psiquica"), TEXT("Fogo")   },
	};

	for (const FCaso& Caso : Casos)
	{
		const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(Caso.Antigo);
		TestTrue(*FString::Printf(TEXT("'%s' resolve"), Caso.Antigo), Identidade.IsValid());
		TestEqual(*FString::Printf(TEXT("'%s' tem a escola certa"), Caso.Antigo),
			Identidade.School, FString(Caso.Escola));
		TestEqual(*FString::Printf(TEXT("'%s' tem o elemento certo"), Caso.Antigo),
			Identidade.Element, FString(Caso.Elemento));
	}

	return true;
}

// Metade reconhecida NÃO é tipo válido.
//
// "Natural/Telepatia" precisa falhar no elemento em vez de virar um tipo que
// existe pela metade e não bate com nada — o modo de falhar silencioso que
// este projeto já viu com efetividade que nunca chegava ao jogo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHalfKnownTypeIsNotValidTest,
	"BattleSquare.Balance.TypeIdentity.HalfKnownTypeIsNotValid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHalfKnownTypeIsNotValidTest::RunTest(const FString& Parameters)
{
	const FPetTypeIdentity ElementoInvalido = FPetTypeIdentity::Parse(TEXT("Natural/Telepatia"));
	TestFalse(TEXT("Elemento desconhecido invalida o par"), ElementoInvalido.IsValid());
	TestEqual(TEXT("Mas a escola que dá para ler é lida"),
		ElementoInvalido.School, FString(TEXT("Natural")));

	const FPetTypeIdentity EscolaInvalida = FPetTypeIdentity::Parse(TEXT("Alquimia/Fogo"));
	TestFalse(TEXT("Escola desconhecida invalida o par"), EscolaInvalida.IsValid());
	TestEqual(TEXT("Mas o elemento que dá para ler é lido"),
		EscolaInvalida.Element, FString(TEXT("Fogo")));

	TestFalse(TEXT("Nome solto desconhecido não resolve"),
		FPetTypeIdentity::Parse(TEXT("Cachorro")).IsValid());
	TestFalse(TEXT("Vazio não resolve"), FPetTypeIdentity::Parse(FString()).IsValid());

	return true;
}
