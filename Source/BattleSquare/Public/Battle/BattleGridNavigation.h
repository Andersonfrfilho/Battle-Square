// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"

/**
 * Projeção de posição para a INTERFACE decidir o que oferecer.
 *
 * O núcleo já recusa movimento para fora da grade (IsInsideGrid, fase de
 * movimento). Isto não recalcula nada disso — só antecipa, para a tela não
 * oferecer um botão que resultaria num turno desperdiçado.
 *
 * LIMITE HONESTO, e ele é real: são 3 ações por turno, e as direções válidas
 * da 2ª dependem de onde o pet ESTARÁ. Esta projeção aplica só os movimentos
 * do PRÓPRIO jogador; o oponente pode bloquear no meio do turno e a projeção
 * fica otimista. O commit é às cegas — a tela não tem como saber melhor.
 */
class BATTLESQUARE_API FBattleGridNavigation
{
public:
	/** Casa resultante depois de aplicar os movimentos já confirmados. */
	static void ProjectCell(uint8 StartColumn, uint8 StartRow,
		const TArray<FBattleAction>& ConfirmedActions,
		int32 GridColumns, int32 GridRows,
		uint8& OutColumn, uint8& OutRow);

	/** Direção que sairia da grade a partir da casa dada. */
	static bool WouldLeaveGrid(uint8 Column, uint8 Row, EBattleDirection Direction,
		int32 GridColumns, int32 GridRows);
};
