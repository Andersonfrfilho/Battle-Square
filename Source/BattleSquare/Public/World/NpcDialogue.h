// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "World/VillageResidents.h"

/**
 * A CONVERSA LIVRE (decisão 67, respondida): modelo PRÓPRIO na infra própria,
 * e o jogo com DOIS modos — restrito local, dinâmico quando online.
 *
 * Este arquivo é a metade PURA: o digesto que vai para o modelo e a resposta
 * restrita de quando ele não está lá. A regra da casa vale dobrada aqui: o
 * digesto só carrega fato VERDADEIRO e NUNCA segredo — o modelo conversa
 * bonito, mas só sabe o que o digesto conta, e é assim que nem o NPC mais
 * eloquente aponta um mercado-negro.
 *
 * ## O contrato com o servidor (a infra implementa isto)
 *
 *   POST <NpcDialogueEndpointUrl>
 *   {
 *     "npc":    { "name", "storyStage", "meetings", "homeStartHour" },
 *     "deeds":  { "beatChampion", "soldAPet", "hasWaterPet" },
 *     "facts":  [ "...só mecânicas com teste..." ],
 *     "playerSays": "texto livre do jogador"
 *   }
 *   → 200 { "npcSays": "a resposta, em personagem" }
 *
 * Qualquer outra coisa — timeout, 500, campo ausente — cai no modo restrito,
 * fail-closed: a conversa nunca morre por causa da rede.
 */
namespace NpcDialogue
{
	/**
	 * O digesto em JSON — TUDO que o modelo tem permissão de saber.
	 *
	 * Construído aqui, num lugar só, para o teste poder afirmar o negativo:
	 * nenhuma coordenada, nenhum "escondido", nenhum mercado-negro. O que não
	 * entra no digesto não sai na fala, por construção.
	 */
	BATTLESQUARE_API FString BuildDigestJson(
		const VillageResidents::FResident& Resident, int32 Meetings,
		const VillageResidents::FPlayerDeeds& Deeds, const FString& PlayerSays);

	/**
	 * A resposta RESTRITA — o modo local, e o fallback do online.
	 *
	 * O morador não improvisa: responde com o que SABE — a história dele no
	 * estágio de agora, reagindo aos feitos como sempre — e admite o limite
	 * ("disso eu não sei falar"). Admitir é honesto duas vezes: para o
	 * jogador, que entende o modo, e para o teste, que afirma uma resposta
	 * determinística.
	 */
	BATTLESQUARE_API FString RestrictedReply(
		const VillageResidents::FResident& Resident, ESettlementKind Kind,
		int32 DoorIndex, int32 Meetings, const VillageResidents::FPlayerDeeds& Deeds);
}
