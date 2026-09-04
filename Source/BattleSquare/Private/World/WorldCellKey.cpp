// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldCellKey.h"

FString WorldCellKey::ChunkKeyOf(const FIntPoint& Chunk)
{
	return FString::Printf(TEXT("%d:%d"), Chunk.X, Chunk.Y);
}

FString WorldCellKey::CellKeyOf(const FVector2D& WorldPositionUnits, float QuantumUnits)
{
	// Quantum não-positivo cairia em divisão por zero: uma casa mínima segura
	// mantém a função total, e a identidade continua determinística.
	const float Quantum = (QuantumUnits > KINDA_SMALL_NUMBER) ? QuantumUnits : 1.0f;
	const int32 CelulaX = FMath::FloorToInt(WorldPositionUnits.X / Quantum);
	const int32 CelulaY = FMath::FloorToInt(WorldPositionUnits.Y / Quantum);
	return FString::Printf(TEXT("%d:%d"), CelulaX, CelulaY);
}
