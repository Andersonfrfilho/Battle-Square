// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A POSSE VINDA DO SERVIDOR, guardada LOCALMENTE (PS7) — as regras puras de
 * "qual coleção o jogo lê, e o quão fresca ela é".
 *
 * A invariante que esta feature inteira gira em torno (decisão 38-b, e a
 * lembrança do dono: "o jogo deve rodar normalmente offline"): a leitura da
 * posse NUNCA bloqueia. O jogo lê sempre do CACHE — a última posse conhecida —
 * e o HTTP só o ATUALIZA quando responde. Backend fora do ar não impede jogar:
 * ele só congela a coleção no último estado bom, e o jogo DIZ que ela é a
 * última conhecida (PS10).
 *
 * Puras porque são a regra: se atualizar, se está velha, o que fica quando a
 * rede falha. Latência não entra aqui — quem fala HTTP é a camada de fora, e
 * ela entrega o resultado a estas regras já resolvido.
 */
namespace OwnershipCache
{
	/** Uma posse conhecida — o mínimo que o jogo precisa para montar o mundo. */
	struct BATTLESQUARE_API FKnownPet
	{
		FString CatalogId;

		/** Roubado carrega a marca — é o que a batalha delata (crime-e-recompensa). */
		bool bStolen = false;
	};

	/** O cache inteiro de uma conta, com a idade dele. */
	struct BATTLESQUARE_API FState
	{
		TArray<FKnownPet> Pets;

		/**
		 * Quando a última busca BEM-SUCEDIDA respondeu, em segundos de mundo.
		 * Negativo é "nunca sincronizou" — cache que só tem o que o save local
		 * trouxe, ou conta nova.
		 */
		double LastSyncSeconds = -1.0;
	};

	/**
	 * A busca respondeu: substitui a lista e carimba a hora.
	 *
	 * Substitui, não funde: o servidor é a fonte da verdade da posse (decisão
	 * 38-b, "o servidor manda no empate"). Fundir manteria localmente um pet
	 * que o servidor já tirou — que é o roubo sobrevivendo à devolução.
	 */
	BATTLESQUARE_API FState AfterSuccessfulFetch(
		const TArray<FKnownPet>& FromServer, double NowSeconds);

	/**
	 * A busca FALHOU (backend fora do ar, timeout): o cache fica INTACTO.
	 *
	 * A falha não zera nem envelhece de propósito o que já se sabia — ela só
	 * deixa de atualizar. É a diferença entre "não sei mais" e "não consegui
	 * perguntar agora", e é ela que faz offline ser normal em vez de vazio.
	 */
	BATTLESQUARE_API FState AfterFailedFetch(const FState& Current);

	/** O cache está velho o bastante para valer avisar o jogador (PS10)? */
	BATTLESQUARE_API bool IsStale(const FState& State, double NowSeconds,
		double StaleAfterSeconds);
}
