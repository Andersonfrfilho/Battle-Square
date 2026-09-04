// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A IDENTIDADE ESTÁVEL de uma árvore no mundo (mundo-vivo, MV3).
 *
 * A marca do corte precisa nomear a árvore de um jeito que sobreviva a fechar e
 * reabrir o jogo — e o índice de instância NÃO serve: a mata reordena as
 * instâncias a cada replantio da semente. A identidade que dura é a POSIÇÃO no
 * mundo, quantizada numa grade: a mesma árvore, plantada de novo da mesma
 * semente, cai na mesma célula, e a marca a reencontra.
 *
 * Puro de propósito: a mesma função roda no corte (para gravar) e no replantio
 * (para suprimir), e as duas TÊM de concordar — uma segunda forma de calcular a
 * célula seria a segunda fonte de verdade que L-032 documentou.
 */
namespace WorldCellKey
{
	/** O pedaço do mundo como chave estável ("cx:cy"). */
	BATTLESQUARE_API FString ChunkKeyOf(const FIntPoint& Chunk);

	/** A célula de uma posição de mundo, quantizada por `QuantumUnits` ("x:y"). */
	BATTLESQUARE_API FString CellKeyOf(const FVector2D& WorldPositionUnits, float QuantumUnits);
}
