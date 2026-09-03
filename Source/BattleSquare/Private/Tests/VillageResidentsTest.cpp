// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/VillageResidents.h"

/**
 * OS MORADORES — decisão 65, e as duas coisas que fazem deles VIZINHOS.
 *
 * Um vizinho tem nome estável (a mesma porta, o mesmo morador, sempre) e tem
 * voz que vale a pena ouvir (fala VERDADE do mundo). Sem a primeira é sorteio
 * com telhado; sem a segunda é placa com nome.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResidentIsAlwaysTheSameNeighborTest,
	"BattleSquare.World.Moradores.OVizinhoEhSempreOMesmo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidentIsAlwaysTheSameNeighborTest::RunTest(const FString&)
{
	// A MESMA porta dá o MESMO morador, em duas consultas separadas: o nome
	// sai da geometria (vila + porta), nunca do relógio — regra 5 da geração
	// procedural, a mesma do campeão da Arena.
	const VillageResidents::FResident Primeira =
		VillageResidents::ResidentFor(ESettlementKind::VilaInicial, 3);
	const VillageResidents::FResident Segunda =
		VillageResidents::ResidentFor(ESettlementKind::VilaInicial, 3);

	TestEqual(TEXT("mesma porta, mesmo nome"), Primeira.Name, Segunda.Name);
	TestEqual(TEXT("e mesma fala"), Primeira.TipLine, Segunda.TipLine);

	TestFalse(TEXT("o morador tem nome"), Primeira.Name.IsEmpty());
	TestFalse(TEXT("e tem voz"), Primeira.TipLine.IsEmpty());

	// E portas diferentes são vizinhos de verdade: em ALGUMA das outras
	// portas mora outra pessoa. Forma existencial de propósito — homônimo em
	// cidade pequena é legítimo, e exigir todos diferentes reprovaria o
	// sorteio, não a regra (regra 12).
	bool bHaOutroVizinho = false;
	for (int32 Porta = 0; Porta < 8 && !bHaOutroVizinho; ++Porta)
	{
		bHaOutroVizinho = VillageResidents::ResidentFor(
			ESettlementKind::VilaInicial, Porta).Name != Primeira.Name;
	}

	TestTrue(TEXT("existe porta com OUTRO morador"), bHaOutroVizinho);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResidentSpeaksTruthAndKeepsSecretsTest,
	"BattleSquare.World.Moradores.FalaVerdadeEGuardaSegredo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidentSpeaksTruthAndKeepsSecretsTest::RunTest(const FString&)
{
	// NENHUM morador aponta segredo. A carta conta e não aponta (J4), e o
	// vizinho tagarela seria a exceção pela porta dos fundos — o mercado-negro
	// com placa, só que falada.
	const ESettlementKind Tipos[] = {
		ESettlementKind::VilaInicial, ESettlementKind::VilaDaAcademia,
		ESettlementKind::VilaDoMercado, ESettlementKind::CidadeGrande,
		ESettlementKind::PostoDeFronteira,
	};

	for (ESettlementKind Tipo : Tipos)
	{
		for (int32 Porta = 0; Porta < 12; ++Porta)
		{
			const VillageResidents::FResident Morador =
				VillageResidents::ResidentFor(Tipo, Porta);

			TestFalse(TEXT("morador nao fala de mercado-negro"),
				Morador.TipLine.Contains(TEXT("mercado-negro"))
					|| Morador.TipLine.Contains(TEXT("escondido")));
		}
	}

	return true;
}
