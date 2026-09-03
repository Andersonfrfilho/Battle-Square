// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TrainerWalletRules.h"
#include "Misc/AutomationTest.h"

/**
 * A CARTEIRA — e o que ela NÃO faz é a metade que importa.
 *
 * A economia inteira já estava calculada e testada sem carteira nenhuma
 * (`SettlementEconomy`: tabela com teste, não preço na tela). Estes testes
 * afirmam o recipiente: dinheiro não nasce duas vezes, não fica negativo, e
 * falir não é renda.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainerWalletGrantsOnceTest,
	"BattleSquare.Meta.Carteira.ABolsaInicialEhUmaSo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainerWalletGrantsOnceTest::RunTest(const FString&)
{
	FTrainerProfile Perfil;

	// O perfil nasce como um save ANTIGO nasce: marca em falso, saldo zero. A
	// primeira carga concede — e é o mesmo caminho para o jogador novo e para
	// quem jogava antes de a carteira existir.
	TestTrue(TEXT("a primeira carga concede"),
		FTrainerWalletRules::GrantStartingMoneyOnce(Perfil, 100));
	TestEqual(TEXT("e a bolsa e a configurada"), Perfil.Money, 100);

	// A SEGUNDA carga não concede. Sem isto, cada abertura do jogo seria um
	// pagamento — a torneira de dinheiro mais silenciosa possível.
	TestFalse(TEXT("a segunda carga NAO concede"),
		FTrainerWalletRules::GrantStartingMoneyOnce(Perfil, 100));
	TestEqual(TEXT("e o saldo nao mudou"), Perfil.Money, 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainerWalletGoingBrokeIsNotIncomeTest,
	"BattleSquare.Meta.Carteira.FalirNaoEhRenda",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainerWalletGoingBrokeIsNotIncomeTest::RunTest(const FString&)
{
	// O CONTRAPESO da bolsa: a marca decide, não o saldo. Um jogador que
	// gastou tudo tem o saldo de um treinador novo — e não é um. Conceder pelo
	// saldo faria falir virar renda, e o chão da economia é a cura de graça em
	// casa (paga em caminhada), nunca dinheiro que renasce.
	FTrainerProfile Perfil;
	FTrainerWalletRules::GrantStartingMoneyOnce(Perfil, 100);

	TestTrue(TEXT("gasta tudo"), FTrainerWalletRules::TrySpend(Perfil, 100));
	TestEqual(TEXT("saldo zero"), Perfil.Money, 0);

	TestFalse(TEXT("e zerado NAO recebe bolsa de novo"),
		FTrainerWalletRules::GrantStartingMoneyOnce(Perfil, 100));
	TestEqual(TEXT("continua zerado"), Perfil.Money, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainerWalletNeverGoesNegativeTest,
	"BattleSquare.Meta.Carteira.NuncaFicaNegativa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainerWalletNeverGoesNegativeTest::RunTest(const FString&)
{
	FTrainerProfile Perfil;
	FTrainerWalletRules::GrantStartingMoneyOnce(Perfil, 30);

	// Gasto maior que o saldo é RECUSA, não dívida: dívida seria mecânica
	// própria, com dono próprio — não um int que passou do zero sem ninguém
	// pedir.
	TestFalse(TEXT("gasto maior que o saldo e recusado"),
		FTrainerWalletRules::TrySpend(Perfil, 31));
	TestEqual(TEXT("e o saldo fica intacto — recusa nao e desconto"),
		Perfil.Money, 30);

	// Valores não positivos não movem nada, nos dois sentidos. "Pagar zero" e
	// "cobrar -5" são os dois jeitos de um bug de chamada virar dinheiro.
	TestFalse(TEXT("gastar zero e recusado"), FTrainerWalletRules::TrySpend(Perfil, 0));
	TestFalse(TEXT("gastar negativo e recusado"), FTrainerWalletRules::TrySpend(Perfil, -5));
	FTrainerWalletRules::Earn(Perfil, 0);
	FTrainerWalletRules::Earn(Perfil, -10);
	TestEqual(TEXT("nada disso moveu o saldo"), Perfil.Money, 30);

	// E o caminho feliz dos dois lados, para o teste não passar com uma
	// carteira que recusa tudo.
	TestTrue(TEXT("gasto que cabe passa"), FTrainerWalletRules::TrySpend(Perfil, 25));
	FTrainerWalletRules::Earn(Perfil, 70);
	TestEqual(TEXT("5 - 25 + 70... o saldo fecha"), Perfil.Money, 75);

	return true;
}
