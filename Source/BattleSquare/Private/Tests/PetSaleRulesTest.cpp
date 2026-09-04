// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetSaleRules.h"
#include "Misc/AutomationTest.h"
#include "World/SettlementEconomy.h"

namespace VendaDePetTeste
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	FOwnedPetInstance PetCapturadoParaVenda(const TCHAR* CatalogId, const TCHAR* Type)
	{
		FOwnedPetInstance Pet;
		Pet.CatalogId = CatalogId;
		Pet.Type = Type;
		return Pet;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetSaleRemovesExactlyOneTest,
	"BattleSquare.Meta.Venda.VenderRemoveExatamenteUm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetSaleRemovesExactlyOneTest::RunTest(const FString&)
{
	// O CONTRAPESO da task: vender sem remover duplicaria pet e dinheiro ao
	// mesmo tempo — o mesmo formato de defeito que I3 evitou para item
	// equipado. E remover DOIS seria o defeito oposto com a mesma causa.
	TArray<FOwnedPetInstance> Colecao = {
		VendaDePetTeste::PetCapturadoParaVenda(TEXT("cap-1"), TEXT("Fisica/Terra")),
		VendaDePetTeste::PetCapturadoParaVenda(TEXT("cap-2"), TEXT("Fisica/Terra")),
		VendaDePetTeste::PetCapturadoParaVenda(TEXT("cap-3"), TEXT("Etereo/Luz")),
	};

	// Dois pets do MESMO tipo, ids diferentes: capturas independentes. Vender
	// um não pode levar o outro — é por isso que a chave é o id de catálogo.
	TestEqual(TEXT("vende o pedido"),
		static_cast<int32>(FPetSaleRules::TrySell(Colecao, TEXT("cap-2"), TEXT("ativo-0"), TSet<FString>())),
		static_cast<int32>(EPetSaleVerdict::Sold));
	TestEqual(TEXT("saiu exatamente um"), Colecao.Num(), 2);
	TestTrue(TEXT("e foi o CERTO — o gemeo de tipo ficou"),
		Colecao[0].CatalogId == TEXT("cap-1") && Colecao[1].CatalogId == TEXT("cap-3"));

	// Vender de novo o mesmo id é recusa nomeada, não silêncio: "nada
	// aconteceu" é indistinguível de defeito.
	TestEqual(TEXT("revender o vendido e NotOwned"),
		static_cast<int32>(FPetSaleRules::TrySell(Colecao, TEXT("cap-2"), TEXT("ativo-0"), TSet<FString>())),
		static_cast<int32>(EPetSaleVerdict::NotOwned));
	TestEqual(TEXT("e a recusa nao mexe na colecao"), Colecao.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetSaleRefusesTheCompanionTest,
	"BattleSquare.Meta.Venda.OCompanheiroNaoSeVende",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetSaleRefusesTheCompanionTest::RunTest(const FString&)
{
	// Vender o pet ATIVO deixaria o jogador sem pet nenhum no mundo. A recusa
	// é a metade reversível: permitir depois é acrescentar; ter permitido e
	// proibir depois seria tirar dinheiro que alguém já contou.
	TArray<FOwnedPetInstance> Colecao = {
		VendaDePetTeste::PetCapturadoParaVenda(TEXT("ativo-0"), TEXT("Fisica/Fogo")),
		VendaDePetTeste::PetCapturadoParaVenda(TEXT("cap-1"), TEXT("Fisica/Agua")),
	};

	TestEqual(TEXT("o ativo e recusado COMO ativo, nao como ausente"),
		static_cast<int32>(FPetSaleRules::TrySell(Colecao, TEXT("ativo-0"), TEXT("ativo-0"), TSet<FString>())),
		static_cast<int32>(EPetSaleVerdict::ActivePet));
	TestEqual(TEXT("e continua na colecao"), Colecao.Num(), 2);

	// Sem ativo declarado (vazio), nada é companheiro — cadastro antigo não
	// ganha proteção que ninguém pediu.
	TestEqual(TEXT("com ativo vazio, vende normal"),
		static_cast<int32>(FPetSaleRules::TrySell(Colecao, TEXT("ativo-0"), FString(), TSet<FString>())),
		static_cast<int32>(EPetSaleVerdict::Sold));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetSalePayoutComesFromTheTableTest,
	"BattleSquare.Meta.Venda.OPagamentoVemDaTabela",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetSalePayoutComesFromTheTableTest::RunTest(const FString&)
{
	// O aceite da task: a proporção EXATA que `PayoutPercent` já calcula e já
	// testa — afirmada CONTRA a tabela, nunca como número digitado aqui. Um
	// número à mão seria a tabela duplicada, que é o defeito que a invariante
	// 15 nomeia.
	const ESettlementKind Lugares[] = {
		ESettlementKind::VilaInicial, ESettlementKind::VilaDaAcademia,
		ESettlementKind::VilaDoMercado, ESettlementKind::CidadeGrande,
		ESettlementKind::PostoDeFronteira,
	};

	for (ESettlementKind Lugar : Lugares)
	{
		TestEqual(TEXT("a conta pronta e valor base x porcentagem"),
			SettlementEconomy::SalePayout(Lugar),
			SettlementEconomy::PetBaseValue()
				* SettlementEconomy::PayoutPercent(Lugar, ESettlementService::Venda) / 100);
	}

	// E a DIFERENÇA que faz viajar valer: o Mercado paga mais que a cidade, e
	// quem não tem Mercado não paga nada. Se um dia isto inverter, a tabela
	// mudou de história — e este teste é quem conta.
	TestTrue(TEXT("o Mercado paga MAIS que a cidade grande"),
		SettlementEconomy::SalePayout(ESettlementKind::VilaDoMercado)
			> SettlementEconomy::SalePayout(ESettlementKind::CidadeGrande));
	TestEqual(TEXT("e onde nao ha Mercado nao ha venda"),
		SettlementEconomy::SalePayout(ESettlementKind::VilaInicial), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetSaleRefusesStolenTest,
	"BattleSquare.Meta.Venda.RoubadoNaoSeVende",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetSaleRefusesStolenTest::RunTest(const FString&)
{
	// CR6: pet marcado como roubado nao se vende no Mercado comum — e onde o
	// roubo tentaria virar dinheiro limpo. A marca vem do cache da posse (do
	// servidor), passada como o conjunto de roubados.
	TArray<FOwnedPetInstance> Colecao = {
		VendaDePetTeste::PetCapturadoParaVenda(TEXT("roubado-1"), TEXT("Fisica/Fogo")),
		VendaDePetTeste::PetCapturadoParaVenda(TEXT("limpo-2"), TEXT("Fisica/Agua")),
	};

	TSet<FString> Roubados;
	Roubados.Add(TEXT("roubado-1"));

	TestEqual(TEXT("o roubado e RECUSADO, e nao sai da colecao"),
		static_cast<int32>(FPetSaleRules::TrySell(Colecao, TEXT("roubado-1"), TEXT("ativo"), Roubados)),
		static_cast<int32>(EPetSaleVerdict::Stolen));
	TestEqual(TEXT("a colecao segue com os dois"), Colecao.Num(), 2);

	// E o LIMPO vende normal — a marca e por pet, nao por coleção.
	TestEqual(TEXT("o pet limpo vende"),
		static_cast<int32>(FPetSaleRules::TrySell(Colecao, TEXT("limpo-2"), TEXT("ativo"), Roubados)),
		static_cast<int32>(EPetSaleVerdict::Sold));

	return true;
}
