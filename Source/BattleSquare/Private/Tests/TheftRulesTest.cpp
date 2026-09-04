// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TheftRules.h"
#include "Misc/AutomationTest.h"

/**
 * CR2 — o roubo é escolha do vencedor, e só há o que roubar no pet de outro.
 *
 * O aceite da task, ao pe da letra: o MESMO confronto, uma vez "roubar" e
 * outra "nao", termina em posse diferente. E o contrapeso do CR1: selvagem e
 * o proprio nao sao roubo.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTheftIsTheWinnersChoiceTest,
	"BattleSquare.Meta.Roubo.EhEscolhaDoVencedor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTheftIsTheWinnersChoiceTest::RunTest(const FString&)
{
	using PetOwnershipQuery::EOwnerRelation;
	using TheftRules::TransfersOwnership;

	// O MESMO CONFRONTO (pet de outro, treinador vulneravel), duas escolhas,
	// duas posses — a escolha importa, nao so a vitoria.
	TestTrue(TEXT("escolher roubar transfere a posse"),
		TransfersOwnership(EOwnerRelation::OtherAccount, /*Vulneravel=*/true, /*Rouba=*/true));
	TestFalse(TEXT("NAO roubar mantem a posse — mesma vitoria, outra escolha"),
		TransfersOwnership(EOwnerRelation::OtherAccount, true, /*Rouba=*/false));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTheftOnlyAgainstAnotherOwnerTest,
	"BattleSquare.Meta.Roubo.SoContraOutroDono",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTheftOnlyAgainstAnotherOwnerTest::RunTest(const FString&)
{
	using PetOwnershipQuery::EOwnerRelation;
	using TheftRules::OptionFor;
	using TheftRules::ETheftOption;

	// SELVAGEM nao e roubo — e captura (o proprio caminho, ja testado no
	// PS11). "Nothing", nao "protected": nao ha vulnerabilidade que torne
	// roubavel o que nao tem dono.
	TestEqual(TEXT("selvagem: nada a roubar"),
		static_cast<int32>(OptionFor(EOwnerRelation::Wild, true)),
		static_cast<int32>(ETheftOption::Nothing));

	// O PROPRIO pet tambem nao — mesmo com o treinador vulneravel.
	TestEqual(TEXT("o proprio pet: nada a roubar"),
		static_cast<int32>(OptionFor(EOwnerRelation::Self, true)),
		static_cast<int32>(ETheftOption::Nothing));

	// O de OUTRO, com o treinador vulneravel, PODE ser roubado.
	TestEqual(TEXT("de outro + vulneravel: CanSteal"),
		static_cast<int32>(OptionFor(EOwnerRelation::OtherAccount, true)),
		static_cast<int32>(ETheftOption::CanSteal));

	// O de outro com o treinador PROTEGIDO (decisao 22): a vitoria nao basta.
	TestEqual(TEXT("de outro + protegido: Protected, nao CanSteal"),
		static_cast<int32>(OptionFor(EOwnerRelation::OtherAccount, /*Vulneravel=*/false)),
		static_cast<int32>(ETheftOption::Protected));
	TestFalse(TEXT("e protegido nao transfere nem escolhendo roubar"),
		TheftRules::TransfersOwnership(EOwnerRelation::OtherAccount, false, true));

	return true;
}
