// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/BattleDataTranslator.h"
#include "Meta/PetAttributeProgression.h"
#include "Meta/PetMoveRequirements.h"
#include "Misc/AutomationTest.h"

namespace
{
	FOwnedPetInstance PetComAtributos(int32 Musculatura, int32 Personalidade, int32 Voo)
	{
		FOwnedPetInstance Pet;
		Pet.CatalogId = TEXT("faisca");
		Pet.Musculature = Musculatura;
		Pet.Personality = Personalidade;
		Pet.SkillProficiency[FPetAttributeProgression::Flight] = Voo;
		return Pet;
	}

	FPetPresentationInfo ApresentacaoComGolpes(
		const TArray<FString>& Atributos, const TArray<int32>& Valores)
	{
		FPetPresentationInfo Apresentacao;
		Apresentacao.CatalogId = TEXT("faisca");
		for (int32 Indice = 0; Indice < Atributos.Num(); ++Indice)
		{
			Apresentacao.MoveNames.Add(FString::Printf(TEXT("golpe %d"), Indice));
			Apresentacao.MoveRequiresAttribute.Add(Atributos[Indice]);
			Apresentacao.MoveRequiresValue.Add(Valores[Indice]);
			Apresentacao.MoveUnlocked.Add(true);
		}
		return Apresentacao;
	}
}

// O requisito compara com o ATRIBUTO CERTO. Ler musculatura quando o golpe
// pede voo destrancaria golpe aéreo para quem só bateu forte — e o erro
// passaria por qualquer teste que use um pet bom em tudo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRequirementReadsTheNamedAttributeTest,
	"BattleSquare.Meta.MoveRequirements.ReadsTheNamedAttribute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRequirementReadsTheNamedAttributeTest::RunTest(const FString& Parameters)
{
	const FOwnedPetInstance Forte = PetComAtributos(/*Musculatura=*/50, /*Personalidade=*/0, /*Voo=*/0);

	TestTrue(TEXT("Musculatura 50 alcança exigência de 10"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 10, Forte));
	TestFalse(TEXT("Voo 0 NÃO alcança exigência de 10, mesmo com músculo de sobra"),
		FPetMoveRequirements::IsMet(TEXT("flight"), 10, Forte));

	const FOwnedPetInstance Voador = PetComAtributos(/*Musculatura=*/0, /*Personalidade=*/0, /*Voo=*/12);
	TestTrue(TEXT("Voo 12 alcança exigência de 12 — o limite é inclusivo"),
		FPetMoveRequirements::IsMet(TEXT("flight"), 12, Voador));
	TestFalse(TEXT("Voo 12 não alcança 13"),
		FPetMoveRequirements::IsMet(TEXT("flight"), 13, Voador));

	return true;
}

// Requisito ausente, zerado ou com nome que esta versão não conhece NÃO
// tranca. Destrancar é reversível; trancar em silêncio deixa um golpe
// inalcançável para sempre, e o jogador sem como descobrir o motivo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnknownOrEmptyRequirementNeverLocksTest,
	"BattleSquare.Meta.MoveRequirements.UnknownOrEmptyNeverLocks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUnknownOrEmptyRequirementNeverLocksTest::RunTest(const FString& Parameters)
{
	const FOwnedPetInstance Zerado = PetComAtributos(0, 0, 0);

	TestTrue(TEXT("'none' com 0 é golpe sem requisito"),
		FPetMoveRequirements::IsMet(TEXT("none"), 0, Zerado));
	TestTrue(TEXT("Valor zero não exige nada, qualquer que seja o nome"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 0, Zerado));
	TestTrue(TEXT("Valor negativo também não exige nada"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), -5, Zerado));
	TestTrue(TEXT("Atributo que esta versão não conhece não tranca"),
		FPetMoveRequirements::IsMet(TEXT("telepatia"), 99, Zerado));

	return true;
}

// Pet FORA da coleção não é trancado por atributos que ele não tem — senão o
// oponente selvagem entraria na batalha sem golpe nenhum.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetOutsideCollectionKeepsEveryMoveTest,
	"BattleSquare.Meta.MoveRequirements.PetOutsideCollectionKeepsEveryMove",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetOutsideCollectionKeepsEveryMoveTest::RunTest(const FString& Parameters)
{
	FPetPresentationInfo Selvagem = ApresentacaoComGolpes(
		{ TEXT("flight"), TEXT("musculature") }, { 99, 99 });

	FPetMoveRequirements::ApplyToPresentation(Selvagem, /*Instance=*/nullptr);

	TestTrue(TEXT("Primeiro golpe segue destrancado"), Selvagem.MoveUnlocked[0]);
	TestTrue(TEXT("Segundo golpe segue destrancado"), Selvagem.MoveUnlocked[1]);

	return true;
}

// Trancar é POR ÍNDICE, e o índice não anda. Filtrar a lista deslocaria os
// golpes seguintes, e o índice é o que viaja no commit (DP-golpe-04) — o
// jogador escolheria um golpe e o resolvedor usaria outro.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLockingKeepsMoveIndicesInPlaceTest,
	"BattleSquare.Meta.MoveRequirements.LockingKeepsMoveIndicesInPlace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLockingKeepsMoveIndicesInPlaceTest::RunTest(const FString& Parameters)
{
	// O golpe do MEIO é o trancado: se a lista encolhesse, o terceiro passaria
	// a ocupar o índice do segundo, e o teste que só contasse itens passaria.
	FPetPresentationInfo Apresentacao = ApresentacaoComGolpes(
		{ TEXT("none"), TEXT("flight"), TEXT("none") }, { 0, 20, 0 });

	const FOwnedPetInstance Pet = PetComAtributos(/*Musculatura=*/0, /*Personalidade=*/0, /*Voo=*/5);
	FPetMoveRequirements::ApplyToPresentation(Apresentacao, &Pet);

	TestEqual(TEXT("A lista continua com três golpes"), Apresentacao.MoveUnlocked.Num(), 3);
	TestTrue(TEXT("Índice 0 destrancado"), Apresentacao.MoveUnlocked[0]);
	TestFalse(TEXT("Índice 1 trancado — voo 5 não alcança 20"), Apresentacao.MoveUnlocked[1]);
	TestTrue(TEXT("Índice 2 destrancado, e continua sendo o índice 2"),
		Apresentacao.MoveUnlocked[2]);
	TestEqual(TEXT("O nome do índice 2 não andou"), Apresentacao.MoveNames[2], FString(TEXT("golpe 2")));

	return true;
}
