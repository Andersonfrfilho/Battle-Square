// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * A FILA DA CAPTURA (PS8) — as regras puras de "o que entra, o que sobe de
 * novo, o que desiste".
 *
 * A captura ja aconteceu no jogo (a colecao local ja tem o pet); a fila e so
 * o EFEITO EXTERNO, subir a posse ao servidor. Separar decisao de efeito e o
 * que `CaptureIfNewInMemory` ja fazia, e e o que permite a captura funcionar
 * offline: o jogo nao espera o servidor para dar o pet.
 */
namespace CaptureQueueRules
{
	/** O teto de tentativas. Estourou: para de retentar e vira "abandonada". */
	inline constexpr int32 MaxAttempts = 5;

	/**
	 * Enfileira uma captura, se ela ja nao estiver na fila.
	 *
	 * Idempotente na PROPRIA fila: capturar o mesmo catalogo duas vezes antes
	 * de a primeira subir nao cria duas linhas — a chave de idempotencia e por
	 * (dono, catalogo), e o mesmo catalogo na mesma fila e a mesma captura.
	 * `IdempotencyKey` e injetada (o chamador conhece o accountId), para estas
	 * regras ficarem puras.
	 */
	BATTLESQUARE_API void Enqueue(TArray<FPendingCapture>& Queue,
		const FString& CatalogId, const FString& IdempotencyKey);

	/** Uma subiu com sucesso: sai da fila. */
	BATTLESQUARE_API void MarkSent(TArray<FPendingCapture>& Queue, const FString& CatalogId);

	/**
	 * Uma tentativa FALHOU: conta mais uma. Devolve true se ainda ha tentativa
	 * — false quando estourou o teto e a captura vira ABANDONADA (fica na fila,
	 * mas MarkExhausted a reconhece, para PS10 mostra-la).
	 */
	BATTLESQUARE_API bool RegisterAttempt(TArray<FPendingCapture>& Queue,
		const FString& CatalogId);

	/** Uma captura estourou o teto e nao vai mais subir? */
	BATTLESQUARE_API bool IsExhausted(const FPendingCapture& Capture);

	/** As que ainda VALE tentar subir (dentro do teto). */
	BATTLESQUARE_API TArray<FPendingCapture> Sendable(const TArray<FPendingCapture>& Queue);

	/** As que DESISTIRAM — o que PS10 tem de mostrar em vez de esconder. */
	BATTLESQUARE_API TArray<FPendingCapture> Exhausted(const TArray<FPendingCapture>& Queue);
}
