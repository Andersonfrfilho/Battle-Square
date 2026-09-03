// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * QUEM MORA NAS CASAS (decisão 65) — puro, e determinístico pela geometria.
 *
 * O morador sai da vila e do índice da porta, nunca do relógio: vizinho que
 * troca de nome a cada visita não é vizinho — é sorteio com telhado. Mesma
 * regra 5 da geração procedural que dá o mesmo campeão à mesma Arena.
 *
 * E ele diz UMA coisa VERDADEIRA: cada fala do repertório aponta uma mecânica
 * que existe e tem teste. Morador não fala de segredo — a carta conta e não
 * aponta (J4), e o vizinho tagarela seria a exceção pela porta dos fundos.
 *
 * O diálogo FUNDO — história própria, memória, evolução — é a decisão 15, e
 * é feature própria. Isto é a fundação dela: o morador existe, tem nome
 * estável e tem voz.
 */
namespace VillageResidents
{
	struct BATTLESQUARE_API FResident
	{
		FString Name;

		/** O que ele diz ao receber visita. Verdade do mundo, sempre. */
		FString TipLine;

		/**
		 * QUANDO ele está em casa e atende — porque a casa não é do visitante.
		 *
		 * "Às vezes não é legal entrar na casa de todo mundo a hora que
		 * quiser; se não tiver ninguém, não tem por que ser convidado"
		 * (decisão 65). A janela é do MORADOR, seedada como o nome: o mesmo
		 * vizinho atende sempre no mesmo horário, e aprender o horário dele é
		 * aprender a vila. Pode cruzar a meia-noite.
		 */
		int32 HomeStartHour = 0;
		int32 HomeEndHour = 0;
	};

	/** O morador da porta `DoorIndex` da vila deste tipo. Sempre o mesmo. */
	BATTLESQUARE_API FResident ResidentFor(ESettlementKind Kind, int32 DoorIndex);

	/**
	 * A CHAVE ESTÁVEL deste morador ("vila-inicial-3") — a mesma para o
	 * mundo e para o save: duas chaves divergiriam na primeira edição, e o
	 * sintoma seria a história recomeçando do nada.
	 */
	BATTLESQUARE_API FString ResidentKeyFor(ESettlementKind Kind, int32 DoorIndex);

	/**
	 * A FALA da visita de número `Meetings` (decisão 15) — a história em
	 * estágios: apresentação, o que houve, a confidência.
	 *
	 * Cada morador tem UM arco, seedado como o nome, e ele avança por visita
	 * ATENDIDA — não por esbarrão de rua: história se conta a quem volta. Da
	 * quarta visita em diante repete a confidência: quem chegou ao fim da
	 * história é amigo, e amigo repete causo — isso é gente, não defeito.
	 *
	 * E todo arco é ancorado no que EXISTE: a fazenda, a ponte destruída, o
	 * campeão fixo, as grutas que se ligam. Memória de morador é flavor; fato
	 * mecânico falso não entra nem como lembrança.
	 */
	BATTLESQUARE_API FString StoryLineFor(const FResident& Resident,
		ESettlementKind Kind, int32 DoorIndex, int32 Meetings);

	/**
	 * OS FEITOS que as histórias escutam — só o que o save já sabe.
	 *
	 * Nenhum campo aqui cria estado novo: o campeão é o ranking, a venda é o
	 * contador dela, o pet de água é o tipo do companheiro. Feito que
	 * exigisse estado próprio seria a história mandando no jogo — e é o
	 * contrário: ela ESCUTA.
	 */
	struct BATTLESQUARE_API FPlayerDeeds
	{
		bool bBeatChampion = false;
		bool bHasSoldAPet = false;
		bool bHasWaterPet = false;
	};

	/**
	 * A HISTÓRIA QUE REAGE (decisão 15, a metade "evolutiva" de verdade): a
	 * mesma visita, com o feito do jogador mudando a resposta.
	 *
	 * Três ganchos, e cada um cola numa CONFIDÊNCIA que já pedia: a Arena
	 * ("se você o vencer, me conte"), o Mercado ("dinheiro vai, a história
	 * fica" — dita a quem também já vendeu), e o pet de água ("sinto falta do
	 * barulho dele" — dita a quem anda com um). Um pedido que ignora o feito
	 * cumprido é o NPC provando que não escuta.
	 *
	 * Arcos sem gancho respondem igual com e sem — reagir a tudo é tão falso
	 * quanto não reagir a nada.
	 */
	BATTLESQUARE_API FString StoryLineReacting(const FResident& Resident,
		ESettlementKind Kind, int32 DoorIndex, int32 Meetings,
		const FPlayerDeeds& Deeds);

	/**
	 * Ele atende a esta hora?
	 *
	 * A janela nunca é o dia inteiro NEM dia nenhum (10 a 16 horas): todo
	 * morador tem hora de estar e hora de não estar — é o que faz a visita
	 * ser visita, e não um cômodo público.
	 */
	BATTLESQUARE_API bool IsHomeAtHour(const FResident& Resident, float Hour);
}
