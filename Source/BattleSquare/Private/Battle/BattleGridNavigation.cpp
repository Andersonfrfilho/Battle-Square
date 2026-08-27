// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleGridNavigation.h"

namespace
{
	void DirectionDelta(EBattleDirection Direction, int32& OutDeltaColumn, int32& OutDeltaRow)
	{
		OutDeltaColumn = 0;
		OutDeltaRow = 0;
		switch (Direction)
		{
			case EBattleDirection::Cima:          OutDeltaRow = -1; break;
			case EBattleDirection::Baixo:         OutDeltaRow = 1; break;
			case EBattleDirection::Esquerda:      OutDeltaColumn = -1; break;
			case EBattleDirection::Direita:       OutDeltaColumn = 1; break;
			case EBattleDirection::CimaEsquerda:  OutDeltaColumn = -1; OutDeltaRow = -1; break;
			case EBattleDirection::CimaDireita:   OutDeltaColumn = 1;  OutDeltaRow = -1; break;
			case EBattleDirection::BaixoEsquerda: OutDeltaColumn = -1; OutDeltaRow = 1; break;
			case EBattleDirection::BaixoDireita:  OutDeltaColumn = 1;  OutDeltaRow = 1; break;
			default: break;
		}
	}
}

void FBattleGridNavigation::ProjectCell(uint8 StartColumn, uint8 StartRow,
	const TArray<FBattleAction>& ConfirmedActions,
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

		int32 DeltaColumn = 0;
		int32 DeltaRow = 0;
		DirectionDelta(Action.Direction, DeltaColumn, DeltaRow);

		// Movimento que sairia é recusado pelo núcleo: a projeção precisa
		// recusar igual, senão ela mente sobre onde o pet vai parar.
		if (IsInsideGrid(Column + DeltaColumn, Row + DeltaRow))
		{
			Column += DeltaColumn;
			Row += DeltaRow;
		}
	}

	OutColumn = static_cast<uint8>(Column);
	OutRow = static_cast<uint8>(Row);
}

bool FBattleGridNavigation::WouldLeaveGrid(uint8 Column, uint8 Row, EBattleDirection Direction)
{
	int32 DeltaColumn = 0;
	int32 DeltaRow = 0;
	DirectionDelta(Direction, DeltaColumn, DeltaRow);
	return !IsInsideGrid(static_cast<int32>(Column) + DeltaColumn, static_cast<int32>(Row) + DeltaRow);
}
