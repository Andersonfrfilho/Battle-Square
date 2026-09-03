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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResidentStoryUnfoldsByVisitTest,
	"BattleSquare.World.Moradores.AHistoriaAvancaPorVisita",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidentStoryUnfoldsByVisitTest::RunTest(const FString&)
{
	// A DECISÃO 15: histórias próprias e EVOLUTIVAS. Própria = o arco é do
	// morador, estável como o nome. Evolutiva = avança por visita atendida:
	// apresentação, quem sou, o que houve, a confidência.
	const VillageResidents::FResident Morador =
		VillageResidents::ResidentFor(ESettlementKind::VilaInicial, 2);

	const FString Primeira = VillageResidents::StoryLineFor(
		Morador, ESettlementKind::VilaInicial, 2, 1);
	const FString Segunda = VillageResidents::StoryLineFor(
		Morador, ESettlementKind::VilaInicial, 2, 2);
	const FString Terceira = VillageResidents::StoryLineFor(
		Morador, ESettlementKind::VilaInicial, 2, 3);
	const FString Quarta = VillageResidents::StoryLineFor(
		Morador, ESettlementKind::VilaInicial, 2, 4);

	// A primeira é apresentação + conselho: estranho não ganha história.
	TestTrue(TEXT("a primeira visita apresenta"), Primeira.Contains(Morador.Name));
	TestTrue(TEXT("e aconselha — a dica de sempre"), Primeira.Contains(Morador.TipLine));

	// Da segunda em diante a história É OUTRA a cada visita — evolutiva.
	TestNotEqual(TEXT("a segunda visita conta outra coisa"), Segunda, Primeira);
	TestNotEqual(TEXT("a terceira avanca"), Terceira, Segunda);
	TestNotEqual(TEXT("e a quarta fecha o arco"), Quarta, Terceira);

	// Da quarta em diante REPETE a confidência: amigo repete causo — isso é
	// gente, não defeito. E repetir é o que impede a história de "andar" sem
	// visita nenhuma nova de verdade.
	TestEqual(TEXT("da quinta em diante, o mesmo causo"),
		VillageResidents::StoryLineFor(Morador, ESettlementKind::VilaInicial, 2, 5),
		Quarta);

	// E a história é DELE: a mesma visita, consultada duas vezes, conta o
	// mesmo — estável como o nome (regra 5).
	TestEqual(TEXT("a historia e estavel"),
		VillageResidents::StoryLineFor(Morador, ESettlementKind::VilaInicial, 2, 3),
		Terceira);

	// A chave do save é a mesma do mundo — duas chaves divergiriam na
	// primeira edição, e o sintoma seria a história recomeçando do nada.
	TestEqual(TEXT("a chave e estavel e legivel"),
		VillageResidents::ResidentKeyFor(ESettlementKind::VilaInicial, 2),
		FString(TEXT("vila-inicial-2")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResidentStoryReactsToDeedsTest,
	"BattleSquare.World.Moradores.AHistoriaReageAoFeito",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidentStoryReactsToDeedsTest::RunTest(const FString&)
{
	// A METADE "EVOLUTIVA" DE VERDADE: o arco da Arena PEDE em texto — "se
	// você o vencer, volte aqui e me conte" — e um pedido que ignora o feito
	// cumprido é o NPC provando que não escuta. Acha-se um morador com esse
	// arco varrendo portas (ele existe por construção: seis arcos, muitas
	// portas), e afirma-se a reação NELE.
	bool bAchouOArcoDaArena = false;

	for (int32 Porta = 0; Porta < 48 && !bAchouOArcoDaArena; ++Porta)
	{
		const VillageResidents::FResident Morador =
			VillageResidents::ResidentFor(ESettlementKind::VilaInicial, Porta);

		const FString SemFeito = VillageResidents::StoryLineFor(
			Morador, ESettlementKind::VilaInicial, Porta, 4);
		if (!SemFeito.Contains(TEXT("se voce o vencer")))
		{
			continue;
		}

		bAchouOArcoDaArena = true;

		// COM o feito, a confidência vira reação — a mesma visita, outra
		// resposta.
		VillageResidents::FPlayerDeeds Venceu;
		Venceu.bBeatChampion = true;
		const FString ComFeito = VillageResidents::StoryLineReacting(
			Morador, ESettlementKind::VilaInicial, Porta, 4, Venceu);
		TestNotEqual(TEXT("quem venceu ouve a reacao, nao o pedido"),
			ComFeito, SemFeito);
		TestTrue(TEXT("e a reacao reconhece o feito"),
			ComFeito.Contains(TEXT("VENCEU")));

		// SEM o feito, a versão que reage responde IGUAL à que não reage:
		// reagir a tudo é tão falso quanto não reagir a nada.
		TestEqual(TEXT("sem o feito, nada muda"),
			VillageResidents::StoryLineReacting(
				Morador, ESettlementKind::VilaInicial, Porta, 4,
				VillageResidents::FPlayerDeeds()),
			SemFeito);

		// E os OUTROS estágios do mesmo arco não reagem — o gancho é a
		// confidência, não o morador inteiro.
		TestEqual(TEXT("a apresentacao nao muda com o feito"),
			VillageResidents::StoryLineReacting(
				Morador, ESettlementKind::VilaInicial, Porta, 2, Venceu),
			VillageResidents::StoryLineFor(
				Morador, ESettlementKind::VilaInicial, Porta, 2));
	}

	TestTrue(TEXT("existe morador com o arco da Arena para testar"),
		bAchouOArcoDaArena);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResidentStoryHasThreeHooksTest,
	"BattleSquare.World.Moradores.TresGanchosTresFeitos",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidentStoryHasThreeHooksTest::RunTest(const FString&)
{
	// OS DOIS GANCHOS NOVOS, pelo mesmo protocolo do primeiro: acha-se o
	// morador com o arco varrendo portas, e afirma-se NELE — com o feito a
	// confidência muda, sem o feito nada muda, e o feito ERRADO também não
	// muda nada (vencer o campeão não mexe no arco do Mercado).
	struct FGancho
	{
		const TCHAR* Pede;
		VillageResidents::FPlayerDeeds Feito;
		VillageResidents::FPlayerDeeds FeitoErrado;
	};

	FGancho Mercado;
	Mercado.Pede = TEXT("dinheiro vai, a historia fica");
	Mercado.Feito.bHasSoldAPet = true;
	Mercado.FeitoErrado.bBeatChampion = true;

	FGancho PetDeAgua;
	PetDeAgua.Pede = TEXT("barulho dele na agua");
	PetDeAgua.Feito.bHasWaterPet = true;
	PetDeAgua.FeitoErrado.bHasSoldAPet = true;

	const FGancho Ganchos[] = { Mercado, PetDeAgua };

	for (const FGancho& Gancho : Ganchos)
	{
		bool bAchou = false;
		for (int32 Porta = 0; Porta < 48 && !bAchou; ++Porta)
		{
			const VillageResidents::FResident Morador =
				VillageResidents::ResidentFor(ESettlementKind::VilaInicial, Porta);
			const FString SemFeito = VillageResidents::StoryLineFor(
				Morador, ESettlementKind::VilaInicial, Porta, 4);
			if (!SemFeito.Contains(Gancho.Pede))
			{
				continue;
			}

			bAchou = true;

			TestNotEqual(TEXT("com o feito, a confidencia reage"),
				VillageResidents::StoryLineReacting(Morador,
					ESettlementKind::VilaInicial, Porta, 4, Gancho.Feito),
				SemFeito);

			TestEqual(TEXT("com o feito ERRADO, nada muda"),
				VillageResidents::StoryLineReacting(Morador,
					ESettlementKind::VilaInicial, Porta, 4, Gancho.FeitoErrado),
				SemFeito);
		}

		TestTrue(TEXT("existe morador com o arco deste gancho"), bAchou);
	}

	return true;
}
