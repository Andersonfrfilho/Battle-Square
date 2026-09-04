// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldFinding.h"
#include "Misc/AutomationTest.h"

/**
 * decisão 57 — o achado: só os destinos fora da trilha guardam dinheiro, e o
 * valor e deterministico da semente do lugar (o mesmo canto, o mesmo achado).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldFindingTest,
	"BattleSquare.World.Achado.ForaDaTrilhaEDeterministico",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldFindingTest::RunTest(const FString&)
{
	using namespace WorldFinding;

	// SÓ OS DESTINOS FORA DA TRILHA guardam achado (decisão 57).
	TestTrue(TEXT("ruina guarda achado"), HasFinding(EGroundUse::Ruina));
	TestTrue(TEXT("clareira fechada guarda achado"), HasFinding(EGroundUse::ClareiraFechada));
	TestTrue(TEXT("mercado-negro guarda achado"), HasFinding(EGroundUse::MercadoNegro));

	// O CONTRAPESO: lugar de caminho/uso comum NAO guarda — o achado paga por
	// sair da trilha, nao por andar nela.
	TestFalse(TEXT("fazenda nao guarda achado"), HasFinding(EGroundUse::Fazenda));
	TestFalse(TEXT("bosque nao guarda achado"), HasFinding(EGroundUse::Bosque));
	TestFalse(TEXT("nenhum uso nao guarda achado"), HasFinding(EGroundUse::Nenhum));

	FFindingConfig Config; Config.MinAmount = 10; Config.MaxAmount = 60;

	// DETERMINISTICO: a MESMA semente da o MESMO achado (o mesmo canto do mundo).
	const uint32 SementeA = 12345u;
	TestEqual(TEXT("mesmo lugar, mesmo achado"),
		AmountAt(SementeA, Config), AmountAt(SementeA, Config));

	// Dentro de [Min, Max].
	for (uint32 S = 1; S <= 50; ++S)
	{
		const int32 V = AmountAt(S * 7919u, Config);
		TestTrue(TEXT("achado dentro da faixa (min)"), V >= 10);
		TestTrue(TEXT("achado dentro da faixa (max)"), V <= 60);
	}

	// Sementes diferentes tendem a achados diferentes (nao um valor fixo).
	TestNotEqual(TEXT("lugares diferentes, achados diferentes"),
		AmountAt(1u, Config), AmountAt(999999u, Config));

	// Config degenerada nao inverte o intervalo.
	FFindingConfig Ruim; Ruim.MinAmount = 50; Ruim.MaxAmount = 10;
	TestEqual(TEXT("config invertida devolve o minimo"), AmountAt(5u, Ruim), 50);

	return true;
}
