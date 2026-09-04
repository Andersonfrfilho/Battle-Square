// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SettlementDisaster.h"
#include "Misc/AutomationTest.h"

/**
 * MB4 — a vila atingida por um desastre acima do limiar deixa de oferecer
 * serviço; a MESMA vila, sem o evento, continua oferecendo (o contrapeso). E o
 * abandono é derivado (mesma entrada, mesma resposta) — nunca salvo.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisasterAbandonsVillageTest,
	"BattleSquare.World.Desastre.CidadeAbandonadaDepoisDoDesastre",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDisasterAbandonsVillageTest::RunTest(const FString&)
{
	using namespace SettlementDisaster;

	const float Limiar = 0.6f;

	// A Cidade Grande oferece Cura de pé — escolha um serviço que ela ofereça.
	const ESettlementKind Cidade = ESettlementKind::CidadeGrande;
	// Confirma que, sem desastre, ela oferece ALGO — senão o teste seria vazio.
	const bool OferecePorPadrao =
		SettlementEconomy::Offers(Cidade, ESettlementService::Cura)
		|| SettlementEconomy::Offers(Cidade, ESettlementService::Academia)
		|| SettlementEconomy::Offers(Cidade, ESettlementService::Venda);
	TestTrue(TEXT("a Cidade Grande oferece algo de pe"), OferecePorPadrao);

	// DESASTRE FORTE (acima do limiar): abandonada, não oferece NADA.
	const bool bAbandonada = IsAbandoned(/*Magnitude=*/0.9f, Limiar);
	TestTrue(TEXT("desastre acima do limiar abandona"), bAbandonada);
	TestFalse(TEXT("abandonada nao cura"),
		OffersAfterDisaster(Cidade, ESettlementService::Cura, bAbandonada));
	TestFalse(TEXT("abandonada nao treina"),
		OffersAfterDisaster(Cidade, ESettlementService::Academia, bAbandonada));

	// O CONTRAPESO: a MESMA vila, com desastre ABAIXO do limiar (ou nenhum),
	// continua oferecendo o que a tabela diz.
	const bool bDePe = IsAbandoned(/*Magnitude=*/0.3f, Limiar);
	TestFalse(TEXT("desastre fraco nao abandona"), bDePe);
	TestEqual(TEXT("de pe, oferece o de sempre"),
		OffersAfterDisaster(Cidade, ESettlementService::Cura, bDePe),
		SettlementEconomy::Offers(Cidade, ESettlementService::Cura));

	// Limiar degenerado nunca abandona.
	TestFalse(TEXT("limiar zero nunca abandona"), IsAbandoned(1.0f, 0.0f));

	// DERIVADO, NAO SALVO: mesma magnitude, mesma resposta — sem estado que uma
	// visita anterior pudesse ter apagado.
	TestEqual(TEXT("abandono e funcao pura da entrada"),
		IsAbandoned(0.9f, Limiar), IsAbandoned(0.9f, Limiar));

	return true;
}
