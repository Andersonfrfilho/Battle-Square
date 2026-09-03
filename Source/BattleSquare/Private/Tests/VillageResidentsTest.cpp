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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResidentHasHoursAndTheHouseIsNotYoursTest,
	"BattleSquare.World.Moradores.ACasaNaoEhSua",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidentHasHoursAndTheHouseIsNotYoursTest::RunTest(const FString&)
{
	// AS DUAS EMENDAS DA DECISÃO 65: "não é legal entrar na casa de todo
	// mundo a hora que quiser" e "se não tiver ninguém, não tem por que ser
	// convidado". A janela do morador é a mecânica das duas — e ela nunca é o
	// dia inteiro NEM dia nenhum: todo morador tem hora de estar e hora de
	// não estar, senão a visita vira cômodo público (ou porta morta).
	const ESettlementKind Tipos[] = {
		ESettlementKind::VilaInicial, ESettlementKind::VilaDoMercado,
		ESettlementKind::CidadeGrande,
	};

	for (ESettlementKind Tipo : Tipos)
	{
		for (int32 Porta = 0; Porta < 8; ++Porta)
		{
			const VillageResidents::FResident Morador =
				VillageResidents::ResidentFor(Tipo, Porta);

			int32 HorasEmCasa = 0;
			for (int32 Hora = 0; Hora < 24; ++Hora)
			{
				if (VillageResidents::IsHomeAtHour(Morador, static_cast<float>(Hora)))
				{
					++HorasEmCasa;
				}
			}

			TestTrue(TEXT("todo morador tem hora de ESTAR"), HorasEmCasa > 0);
			TestTrue(TEXT("e hora de NAO estar — a casa nao e do visitante"),
				HorasEmCasa < 24);
		}
	}

	// A janela que CRUZA a meia-noite funciona: das 20 às 6 é morador que
	// passa o dia fora — e o teste monta o caso à mão em vez de torcer pelo
	// sorteio (regra 12).
	VillageResidents::FResident Noturno;
	Noturno.HomeStartHour = 20;
	Noturno.HomeEndHour = 6;
	TestTrue(TEXT("as 23h o noturno atende"),
		VillageResidents::IsHomeAtHour(Noturno, 23.0f));
	TestTrue(TEXT("as 3h tambem"), VillageResidents::IsHomeAtHour(Noturno, 3.0f));
	TestFalse(TEXT("ao meio-dia nao"), VillageResidents::IsHomeAtHour(Noturno, 12.0f));

	return true;
}
