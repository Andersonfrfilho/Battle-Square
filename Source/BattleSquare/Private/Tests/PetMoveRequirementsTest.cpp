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

// O rótulo do atributo tem UMA dona.
//
// Três telas mostram o mesmo atributo — o golpe trancado, o ganho e o
// anúncio de desbloqueio. Três cópias do texto concordariam até a primeira
// edição, e o sintoma seria o mesmo atributo com dois nomes na mesma partida.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAttributeLabelHasOneSourceTest,
	"BattleSquare.Meta.MoveRequirements.AttributeLabelHasOneSource",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAttributeLabelHasOneSourceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("flight vira Voo"),
		FPetMoveRequirements::GetAttributeLabel(TEXT("flight")).ToString(), FString(TEXT("Voo")));
	TestEqual(TEXT("musculature vira Musculatura"),
		FPetMoveRequirements::GetAttributeLabel(TEXT("musculature")).ToString(),
		FString(TEXT("Musculatura")));

	// Desconhecido volta CRU. Inventar um rótulo plausível esconderia um erro
	// de cadastro atrás de uma palavra bonita — e o requisito errado, que já
	// não tranca nada, ficaria também invisível.
	TestEqual(TEXT("Atributo desconhecido volta como veio"),
		FPetMoveRequirements::GetAttributeLabel(TEXT("telepatia")).ToString(),
		FString(TEXT("telepatia")));

	return true;
}

// Requisito ausente não vira texto: "exige Musculatura 0" mostraria ao
// jogador uma trava que não existe.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRequirementDescriptionIsEmptyWhenThereIsNoneTest,
	"BattleSquare.Meta.MoveRequirements.DescriptionIsEmptyWhenThereIsNone",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRequirementDescriptionIsEmptyWhenThereIsNoneTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Valor zero não descreve requisito"),
		FPetMoveRequirements::DescribeRequirement(TEXT("musculature"), 0).IsEmpty());
	TestTrue(TEXT("Valor negativo também não"),
		FPetMoveRequirements::DescribeRequirement(TEXT("musculature"), -3).IsEmpty());

	TestEqual(TEXT("Requisito real é legível"),
		FPetMoveRequirements::DescribeRequirement(TEXT("flight"), 12).ToString(),
		FString(TEXT("exige Voo 12")));

	return true;
}

// O anúncio de desbloqueio é uma TRANSIÇÃO, não um estado.
//
// A regra que decide isso é a comparação entre o antes e o depois. Olhar só o
// depois anunciaria, a cada batalha, todo golpe já destravado — e o aviso que
// aparece sempre é o aviso que ninguém lê.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnlockIsATransitionNotAStateTest,
	"BattleSquare.Meta.MoveRequirements.UnlockIsATransitionNotAState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUnlockIsATransitionNotAStateTest::RunTest(const FString& Parameters)
{
	const FOwnedPetInstance Antes = PetComAtributos(/*Musculatura=*/11, /*Personalidade=*/0, /*Voo=*/0);
	const FOwnedPetInstance Depois = PetComAtributos(/*Musculatura=*/12, /*Personalidade=*/0, /*Voo=*/0);

	// Cruzou o limite NESTA batalha: é o único caso que se anuncia.
	TestFalse(TEXT("Antes não alcançava"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 12, Antes));
	TestTrue(TEXT("Depois alcança"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 12, Depois));

	// Já alcançava antes: os dois lados verdadeiros, nada a anunciar.
	TestTrue(TEXT("Requisito baixo já valia antes"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 5, Antes));
	TestTrue(TEXT("E continua valendo depois"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 5, Depois));

	// Ainda não alcança: os dois falsos, nada a anunciar.
	TestFalse(TEXT("Requisito alto não valia antes"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 99, Antes));
	TestFalse(TEXT("Nem depois"),
		FPetMoveRequirements::IsMet(TEXT("musculature"), 99, Depois));

	return true;
}
