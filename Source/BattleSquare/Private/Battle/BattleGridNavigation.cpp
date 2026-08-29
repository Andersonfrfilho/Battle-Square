// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleGridNavigation.h"

void FBattleGridNavigation::ProjectCell(uint8 StartColumn, uint8 StartRow,
	const TArray<FBattleAction>& ConfirmedActions,
	int32 GridColumns, int32 GridRows,
	uint8& OutColumn, uint8& OutRow)
{
	int32 Column = StartColumn;
	int32 Row = StartRow;

	for (const FBattleAction& Action : ConfirmedActions)
	{
		if (Action.Type != EActionType::Mover)
		{
			continue;
		}

		int8 DeltaColumn = 0;
		int8 DeltaRow = 0;
		// A tabela é do NÚCLEO. Manter uma cópia aqui seria uma segunda fonte
		// de verdade sobre para onde cada direção aponta.
		GetDirectionDelta(Action.Direction, DeltaColumn, DeltaRow);

		// Movimento que sairia é recusado pelo núcleo: a projeção precisa
		// recusar igual, senão ela mente sobre onde o pet vai parar.
		if (IsInsideGrid(Column + DeltaColumn, Row + DeltaRow, GridColumns, GridRows))
		{
			Column += DeltaColumn;
			Row += DeltaRow;
		}
	}

	OutColumn = static_cast<uint8>(Column);
	OutRow = static_cast<uint8>(Row);
}

bool FBattleGridNavigation::WouldLeaveGrid(uint8 Column, uint8 Row, EBattleDirection Direction,
	int32 GridColumns, int32 GridRows)
{
	int8 DeltaColumn = 0;
	int8 DeltaRow = 0;
	GetDirectionDelta(Direction, DeltaColumn, DeltaRow);
	return !IsInsideGrid(static_cast<int32>(Column) + DeltaColumn, static_cast<int32>(Row) + DeltaRow,
		GridColumns, GridRows);
}
