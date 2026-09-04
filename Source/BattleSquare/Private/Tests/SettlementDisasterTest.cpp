// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SettlementDisaster.h"
#include "Misc/AutomationTest.h"

/**
 * MB4 (decisão 52) — o desastre INTERROMPE a vila enquanto passa; a MESMA vila,
 * sem o evento, oferece de sempre (o contrapeso). E a interrupção é derivada
 * (mesma entrada, mesma resposta) — nunca salva: passado o desastre, reconstrói.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisasterDisruptsVillageTest,
	"BattleSquare.World.Desastre.DesastreInterrompeEVilaSeReconstroi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDisasterDisruptsVillageTest::RunTest(const FString&)
{
	using namespace SettlementDisaster;

	const float Limiar = 0.6f;
	const ESettlementKind Cidade = ESettlementKind::CidadeGrande;

	const bool OferecePorPadrao =
		SettlementEconomy::Offers(Cidade, ESettlementService::Cura)
		|| SettlementEconomy::Offers(Cidade, ESettlementService::Academia)
		|| SettlementEconomy::Offers(Cidade, ESettlementService::Venda);
	TestTrue(TEXT("a Cidade Grande oferece algo de pe"), OferecePorPadrao);

	// DESASTRE FORTE (magnitude alta, o evento passando): interrompida, nao
	// oferece NADA enquanto passa.
	const bool bInterrompida = IsDisrupted(/*Magnitude=*/0.9f, Limiar);
	TestTrue(TEXT("desastre acima do limiar interrompe"), bInterrompida);
	TestFalse(TEXT("interrompida nao cura"),
		OffersDuringDisaster(Cidade, ESettlementService::Cura, bInterrompida));

	// RECONSTRUCAO (decisao 52): passado o desastre, a magnitude cai e a vila
	// VOLTA a oferecer sozinha — a MESMA vila, sem evento, esta de pe.
	const bool bReconstruida = IsDisrupted(/*Magnitude=*/0.1f, Limiar);
	TestFalse(TEXT("passado o desastre, nao esta mais interrompida"), bReconstruida);
	TestEqual(TEXT("reconstruida, oferece o de sempre"),
		OffersDuringDisaster(Cidade, ESettlementService::Cura, bReconstruida),
		SettlementEconomy::Offers(Cidade, ESettlementService::Cura));

	// Limiar degenerado nunca interrompe.
	TestFalse(TEXT("limiar zero nunca interrompe"), IsDisrupted(1.0f, 0.0f));

	// DERIVADO, NAO SALVO: mesma magnitude, mesma resposta — sem estado que uma
	// visita anterior pudesse ter apagado.
	TestEqual(TEXT("interrupcao e funcao pura da entrada"),
		IsDisrupted(0.9f, Limiar), IsDisrupted(0.9f, Limiar));

	return true;
}
