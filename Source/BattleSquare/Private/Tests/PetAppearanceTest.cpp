// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetAppearance.h"
#include "Misc/AutomationTest.h"

// O TIPO precisa mudar alguma coisa na tela.
//
// Antes disto, dois pets de tipos diferentes eram esferas idênticas: a skill
// que só um deles tem não tinha de onde ser adivinhada, e o jogador tinha que
// decorar de quem era cada bola.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetAppearanceDistinguishesTypesTest,
	"BattleSquare.PetAppearance.DistinguishesTypes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetAppearanceDistinguishesTypesTest::RunTest(const FString& Parameters)
{
	// Os três tipos que Config/PetSkills.json usa para dar skill.
	const FPetAppearance Fogo = FPetAppearance::ForType(TEXT("Fogo"));
	const FPetAppearance Agua = FPetAppearance::ForType(TEXT("Agua"));
	const FPetAppearance Planta = FPetAppearance::ForType(TEXT("Planta"));

	TestTrue(TEXT("Fogo tem chama"), Fogo.CrestShape == EPetCrestShape::Chama);
	TestTrue(TEXT("Agua tem barbatana"), Agua.CrestShape == EPetCrestShape::Barbatana);
	TestTrue(TEXT("Planta tem folha"), Planta.CrestShape == EPetCrestShape::Folha);

	// Forma E cor: quem não distingue as formas no ângulo do diorama ainda
	// distingue a cor, e vice-versa. Uma só das duas deixaria metade dos
	// jogadores sem a informação.
	TestFalse(TEXT("Fogo e Agua não compartilham a cor de acento"),
		Fogo.AccentColor.Equals(Agua.AccentColor, 0.01f));
	TestFalse(TEXT("Agua e Planta não compartilham a cor de acento"),
		Agua.AccentColor.Equals(Planta.AccentColor, 0.01f));
	TestFalse(TEXT("Fogo e Planta não compartilham a cor de acento"),
		Fogo.AccentColor.Equals(Planta.AccentColor, 0.01f));

	return true;
}

// Tipo desconhecido devolve aparência neutra, e NÃO falha.
//
// O catálogo de pets é dado assinado e pode ganhar tipo novo sem recompilar —
// o pet que hoje está em jogo é do tipo "Cat", que esta tabela não conhece. Um
// pet invisível ou um crash seriam piores que um pet genérico.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetAppearanceFallsBackForUnknownTypeTest,
	"BattleSquare.PetAppearance.FallsBackForUnknownType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetAppearanceFallsBackForUnknownTypeTest::RunTest(const FString& Parameters)
{
	const FPetAppearance Desconhecido = FPetAppearance::ForType(TEXT("Cat"));
	const FPetAppearance Vazio = FPetAppearance::ForType(FString());

	TestTrue(TEXT("Tipo desconhecido cai na orelha neutra"),
		Desconhecido.CrestShape == EPetCrestShape::Orelha);
	TestTrue(TEXT("Tipo vazio também"), Vazio.CrestShape == EPetCrestShape::Orelha);

	// O adorno precisa ter tamanho: escala zero é o mesmo que não existir, e
	// voltaria à esfera lisa sem que nenhum teste acusasse.
	TestTrue(TEXT("O adorno neutro tem tamanho"), Desconhecido.CrestScale.GetMin() > 0.0f);

	// O nome do tipo vem de dado autorado — comparar com sensibilidade a
	// caixa faria "fogo" virar um tipo diferente de "Fogo".
	TestTrue(TEXT("O tipo casa sem depender da caixa"),
		FPetAppearance::ForType(TEXT("fOgO")).CrestShape == EPetCrestShape::Chama);

	return true;
}

// Cachorro não pode sair igual a gato.
//
// Os dois são os ÚNICOS tipos que o espelho de pets tem hoje, e é por eles que
// o sistema de tipos aparece ou não aparece na tela. Enquanto "Dog" caía no
// neutro, a tabela inteira era código sem prova visual — o mesmo modo de falhar
// dos atores sem malha atribuída, que passam em todo teste de lógica.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetAppearanceTellsDogFromCatTest,
	"BattleSquare.PetAppearance.TellsDogFromCat",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetAppearanceTellsDogFromCatTest::RunTest(const FString& Parameters)
{
	const FPetAppearance Cachorro = FPetAppearance::ForType(TEXT("Dog"));
	const FPetAppearance Gato = FPetAppearance::ForType(TEXT("Cat"));

	TestTrue(TEXT("A orelha do cachorro CAI, a do gato aponta"),
		Cachorro.CrestShape == EPetCrestShape::OrelhaCaida
			&& Gato.CrestShape == EPetCrestShape::Orelha);

	// Forma diferente com o mesmo tombo seria diferença só no enum: quem olha
	// a tela vê o ângulo, não o nome da constante.
	TestTrue(TEXT("E cai bem mais que a do gato"),
		FMath::Abs(Cachorro.CrestRotation.Roll) > FMath::Abs(Gato.CrestRotation.Roll) + 20.0f);

	TestFalse(TEXT("A cor de acento não é a neutra"),
		Cachorro.AccentColor.Equals(Gato.AccentColor, 0.01f));

	TestTrue(TEXT("O adorno do cachorro tem tamanho"), Cachorro.CrestScale.GetMin() > 0.0f);

	// Mesma tolerância a caixa dos outros tipos: o nome vem de dado assinado.
	TestTrue(TEXT("O tipo casa sem depender da caixa"),
		FPetAppearance::ForType(TEXT("dOg")).CrestShape == EPetCrestShape::OrelhaCaida);

	return true;
}

// SETE tipos, e cada um precisa ser distinguível dos outros seis.
//
// Com três tipos a matiz bastava sozinha. Com sete, dois problemas aparecem
// juntos: o olho não separa sete matizes num bicho em movimento, e sete matizes
// distintas não cabem sem encostar nas cores que o TERRENO já usa.
//
// A saída foi pôr a SILHUETA para identificar e a cor para agrupar. Este teste
// protege as duas metades: nenhum par de tipos partilha forma E cor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEverySevenTypesAreTellableApartTest,
	"BattleSquare.Battle.PetAppearance.EverySevenTypesAreTellableApart",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEverySevenTypesAreTellableApartTest::RunTest(const FString& Parameters)
{
	const TCHAR* Tipos[] = {
		TEXT("Fogo"), TEXT("Agua"), TEXT("Planta"),
		TEXT("Inseto"), TEXT("Psiquico"), TEXT("Magico"), TEXT("Caverna"),
	};
	constexpr int32 Quantos = UE_ARRAY_COUNT(Tipos);

	for (int32 A = 0; A < Quantos; ++A)
	{
		const FPetAppearance PrimeiraAparencia = FPetAppearance::ForType(Tipos[A]);

		for (int32 B = A + 1; B < Quantos; ++B)
		{
			const FPetAppearance SegundaAparencia = FPetAppearance::ForType(Tipos[B]);

			const bool bMesmaForma = PrimeiraAparencia.CrestShape == SegundaAparencia.CrestShape;
			const bool bMesmaCor = PrimeiraAparencia.AccentColor.Equals(SegundaAparencia.AccentColor, 0.001f);

			TestFalse(
				*FString::Printf(TEXT("%s e %s não têm forma E cor iguais"), Tipos[A], Tipos[B]),
				bMesmaForma && bMesmaCor);
		}
	}

	return true;
}

// Tipo NOVO não pode sair igual ao pet sem tipo.
//
// O ramo padrão devolve a aparência neutra, então um nome escrito errado —
// "Psíquico" com acento, "magico" noutra grafia — produz um bicho que existe,
// luta e parece genérico. É o modo de falhar de L-045: o ramo existe, o dado
// nunca cai nele, e nada avisa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNewTypesDoNotFallBackToNeutralTest,
	"BattleSquare.Battle.PetAppearance.NewTypesDoNotFallBackToNeutral",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNewTypesDoNotFallBackToNeutralTest::RunTest(const FString& Parameters)
{
	const FPetAppearance Neutro = FPetAppearance::ForType(TEXT("tipo-que-nao-existe"));

	const TCHAR* Novos[] = {
		TEXT("Inseto"), TEXT("Psiquico"), TEXT("Magico"), TEXT("Caverna"),
	};

	for (const TCHAR* Tipo : Novos)
	{
		const FPetAppearance Aparencia = FPetAppearance::ForType(Tipo);
		TestFalse(
			*FString::Printf(TEXT("%s não cai no ramo neutro"), Tipo),
			Aparencia.CrestShape == Neutro.CrestShape
				&& Aparencia.AccentColor.Equals(Neutro.AccentColor, 0.001f));
	}

	// E o nome é casado sem diferenciar maiúscula: o catálogo do backend não
	// promete grafia, e um pet "PSIQUICO" não pode virar genérico por isso.
	TestTrue(TEXT("A grafia em caixa alta encontra o mesmo tipo"),
		FPetAppearance::ForType(TEXT("PSIQUICO")).CrestShape
			== FPetAppearance::ForType(TEXT("Psiquico")).CrestShape);

	return true;
}
