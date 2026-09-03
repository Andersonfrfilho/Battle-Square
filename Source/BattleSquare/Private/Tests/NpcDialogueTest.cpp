// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/NpcDialogue.h"

/**
 * A CONVERSA LIVRE (decisão 67) — e o teste guarda as duas fronteiras dela.
 *
 * A primeira é o DIGESTO: o modelo só sabe o que ele carrega, então o que não
 * pode sair na fala não pode entrar nele — e isso se afirma no NEGATIVO. A
 * segunda é o MODO RESTRITO: determinístico, honesto sobre o limite, e vivo
 * sem rede nenhuma.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueDigestCarriesTruthOnlyTest,
	"BattleSquare.World.Conversa.ODigestoSoCarregaVerdade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDialogueDigestCarriesTruthOnlyTest::RunTest(const FString&)
{
	const VillageResidents::FResident Morador =
		VillageResidents::ResidentFor(ESettlementKind::VilaInicial, 1);

	VillageResidents::FPlayerDeeds Feitos;
	Feitos.bBeatChampion = true;

	const FString Digesto = NpcDialogue::BuildDigestJson(
		Morador, 3, Feitos, TEXT("qual o melhor lugar para pescar?"));

	// O que o modelo PRECISA saber está lá: quem é o NPC, o que o jogador
	// disse, e os feitos.
	TestTrue(TEXT("o digesto sabe quem fala"), Digesto.Contains(Morador.Name));
	TestTrue(TEXT("e o que o jogador disse"),
		Digesto.Contains(TEXT("melhor lugar para pescar")));
	TestTrue(TEXT("e os feitos verdadeiros"),
		Digesto.Contains(TEXT("\"beatChampion\":true")));

	// E O NEGATIVO, que é a fronteira: nenhum segredo entra no digesto — o
	// que não entra não sai na fala, POR CONSTRUÇÃO. É a mesma regra da carta
	// (conta e não aponta) valendo para o NPC mais eloquente do mundo.
	TestFalse(TEXT("nenhum mercado-negro no digesto"),
		Digesto.Contains(TEXT("mercado-negro")));
	TestFalse(TEXT("nenhum 'escondido' no digesto"),
		Digesto.Contains(TEXT("escondido")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueRestrictedModeIsAliveAndHonestTest,
	"BattleSquare.World.Conversa.OModoRestritoViveSemRede",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDialogueRestrictedModeIsAliveAndHonestTest::RunTest(const FString&)
{
	// O MODO RESTRITO é a base e o para-quedas: offline joga-se local
	// (decisão 39), então a conversa sem rede tem de EXISTIR — limitada,
	// nunca morta. E ela ADMITE o limite antes de responder: fingir o modo
	// dinâmico sem o modelo seria a mentira mais cara.
	const VillageResidents::FResident Morador =
		VillageResidents::ResidentFor(ESettlementKind::VilaInicial, 1);

	const FString Resposta = NpcDialogue::RestrictedReply(
		Morador, ESettlementKind::VilaInicial, 1, 3,
		VillageResidents::FPlayerDeeds());

	TestFalse(TEXT("ha resposta sem rede nenhuma"), Resposta.IsEmpty());
	TestTrue(TEXT("e ela admite o limite"),
		Resposta.Contains(TEXT("nao sei falar")));

	// Determinística: a mesma pergunta, a mesma resposta — é o que separa o
	// modo restrito de um improviso quebrado.
	TestEqual(TEXT("e deterministica"),
		NpcDialogue::RestrictedReply(Morador, ESettlementKind::VilaInicial, 1, 3,
			VillageResidents::FPlayerDeeds()),
		Resposta);

	// E os FEITOS chegam nela: o modo restrito reage como a visita reage — a
	// história que ele devolve é a que escuta o jogo.
	VillageResidents::FPlayerDeeds Venceu;
	Venceu.bBeatChampion = true;

	bool bAlgumaRespostaReage = false;
	for (int32 Porta = 0; Porta < 48 && !bAlgumaRespostaReage; ++Porta)
	{
		const VillageResidents::FResident Outro =
			VillageResidents::ResidentFor(ESettlementKind::VilaInicial, Porta);
		bAlgumaRespostaReage = NpcDialogue::RestrictedReply(
				Outro, ESettlementKind::VilaInicial, Porta, 4, Venceu)
			!= NpcDialogue::RestrictedReply(
				Outro, ESettlementKind::VilaInicial, Porta, 4,
				VillageResidents::FPlayerDeeds());
	}

	TestTrue(TEXT("existe morador cujo modo restrito reage ao feito"),
		bAlgumaRespostaReage);

	return true;
}
