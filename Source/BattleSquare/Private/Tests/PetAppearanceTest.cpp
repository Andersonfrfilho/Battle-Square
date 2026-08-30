// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetTypeIdentity.h"
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
	// Os três elementos que Config/PetSkills.json usa para dar skill.
	const FPetAppearance Fogo = FPetAppearance::ForType(TEXT("Fogo"));
	const FPetAppearance Agua = FPetAppearance::ForType(TEXT("Agua"));
	const FPetAppearance Planta = FPetAppearance::ForType(TEXT("Planta"));

	// Os três são NATURAIS, então partilham a malha — a malha virou a escola
	// quando o tipo ganhou dois eixos. O que os separa em silhueta é o TOMBO.
	TestTrue(TEXT("Os naturais partilham a malha da escola"),
		Fogo.CrestShape == Agua.CrestShape && Agua.CrestShape == Planta.CrestShape);

	// SILHUETA e cor, e é por isso que o tombo existe: quem não distingue as
	// cores ainda distingue a inclinação, e vice-versa. Uma só das duas
	// deixaria metade dos jogadores sem a informação.
	TestFalse(TEXT("Fogo e Agua não têm o mesmo tombo"),
		FMath::IsNearlyEqual(Fogo.CrestRotation.Roll, Agua.CrestRotation.Roll, 1.0f));
	TestFalse(TEXT("Agua e Planta não têm o mesmo tombo"),
		FMath::IsNearlyEqual(Agua.CrestRotation.Roll, Planta.CrestRotation.Roll, 1.0f));
	TestFalse(TEXT("Fogo e Planta não têm o mesmo tombo"),
		FMath::IsNearlyEqual(Fogo.CrestRotation.Roll, Planta.CrestRotation.Roll, 1.0f));

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

// DOZE tipos, e cada um distinguível — com três formas e quatro cores.
//
// É o ponto do modelo de dois eixos: alfabetos pequenos que se combinam. Sete
// nomes soltos precisavam de sete matizes e já não cabiam; três silhuetas e
// quatro cores cobrem doze e sobra espaço.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEveryTypePairIsTellableApartTest,
	"BattleSquare.Battle.PetAppearance.EveryTypePairIsTellableApart",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEveryTypePairIsTellableApartTest::RunTest(const FString& Parameters)
{
	TArray<FString> Todos;
	for (const FString& Escola : FPetTypeIdentity::AllSchools())
	{
		for (const FString& Elemento : FPetTypeIdentity::AllElements())
		{
			Todos.Add(FString::Printf(TEXT("%s/%s"), *Escola, *Elemento));
		}
	}

	for (int32 A = 0; A < Todos.Num(); ++A)
	{
		const FPetAppearance Primeira = FPetAppearance::ForType(Todos[A]);
		for (int32 B = A + 1; B < Todos.Num(); ++B)
		{
			const FPetAppearance Segunda = FPetAppearance::ForType(Todos[B]);

			// SILHUETA = malha + tombo. Dois tipos podem partilhar a malha
			// (mesma escola) desde que o tombo os separe.
			const bool bMesmaSilhueta = Primeira.CrestShape == Segunda.CrestShape
				&& FMath::IsNearlyEqual(Primeira.CrestRotation.Roll, Segunda.CrestRotation.Roll, 1.0f);
			const bool bMesmaCor = Primeira.AccentColor.Equals(Segunda.AccentColor, 0.001f);

			// A SILHUETA identifica o tipo INTEIRO — os doze são únicos nela,
			// e é o canal completo. Quem não distingue cor não perde nada.
			TestFalse(*FString::Printf(TEXT("%s e %s não têm a mesma silhueta"),
				*Todos[A], *Todos[B]), bMesmaSilhueta);

			// A COR identifica só o ELEMENTO, e isso é deliberado: são quatro
			// cores para doze tipos, e forçar doze matizes foi exatamente o que
			// não coube. Ela é o canal RÁPIDO e parcial — dois tipos do mesmo
			// elemento partilham a cor de propósito.
			const bool bMesmoElemento =
				FPetTypeIdentity::Parse(Todos[A]).Element == FPetTypeIdentity::Parse(Todos[B]).Element;
			TestEqual(*FString::Printf(TEXT("%s e %s partilham a cor exatamente quando partilham o elemento"),
				*Todos[A], *Todos[B]), bMesmaCor, bMesmoElemento);
		}
	}

	return true;
}

// A SILHUETA é a escola e a COR é o elemento — e é essa separação que faz o
// sistema crescer. Se as duas viessem do mesmo eixo, doze tipos exigiriam doze
// de alguma coisa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShapeIsSchoolAndColorIsElementTest,
	"BattleSquare.Battle.PetAppearance.ShapeIsSchoolAndColorIsElement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FShapeIsSchoolAndColorIsElementTest::RunTest(const FString& Parameters)
{
	// Mesma escola, elementos diferentes: MESMA forma, cores diferentes.
	const FPetAppearance FisicaFogo = FPetAppearance::ForType(TEXT("Fisica/Fogo"));
	const FPetAppearance FisicaAgua = FPetAppearance::ForType(TEXT("Fisica/Agua"));
	TestTrue(TEXT("Mesma escola dá a mesma silhueta"),
		FisicaFogo.CrestShape == FisicaAgua.CrestShape);
	TestFalse(TEXT("E elementos diferentes dão cores diferentes"),
		FisicaFogo.AccentColor.Equals(FisicaAgua.AccentColor, 0.001f));

	// Mesmo elemento, escolas diferentes: MESMA cor, formas diferentes.
	const FPetAppearance PsiquicaFogo = FPetAppearance::ForType(TEXT("Psiquica/Fogo"));
	TestTrue(TEXT("Mesmo elemento dá a mesma cor"),
		FisicaFogo.AccentColor.Equals(PsiquicaFogo.AccentColor, 0.001f));
	TestFalse(TEXT("E escolas diferentes dão silhuetas diferentes"),
		FisicaFogo.CrestShape == PsiquicaFogo.CrestShape);

	// Nome ANTIGO chega à mesma aparência do par que ele virou.
	const FPetAppearance Magico = FPetAppearance::ForType(TEXT("Magico"));
	TestTrue(TEXT("'Magico' tem a silhueta de Psiquica/Fogo"),
		Magico.CrestShape == PsiquicaFogo.CrestShape);
	TestTrue(TEXT("E a cor de Psiquica/Fogo"),
		Magico.AccentColor.Equals(PsiquicaFogo.AccentColor, 0.001f));

	return true;
}

// Tipo irreconhecível não pode sair parecendo um tipo válido.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnknownTypeIsVisiblyUnknownTest,
	"BattleSquare.Battle.PetAppearance.UnknownTypeIsVisiblyUnknown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUnknownTypeIsVisiblyUnknownTest::RunTest(const FString& Parameters)
{
	const FPetAppearance Desconhecido = FPetAppearance::ForType(TEXT("Natural/Telepatia"));

	for (const FString& Elemento : FPetTypeIdentity::AllElements())
	{
		const FPetAppearance Valido =
			FPetAppearance::ForType(FString::Printf(TEXT("Natural/%s"), *Elemento));
		TestFalse(*FString::Printf(TEXT("Não se confunde com Natural/%s"), *Elemento),
			Desconhecido.AccentColor.Equals(Valido.AccentColor, 0.001f));
	}

	return true;
}
