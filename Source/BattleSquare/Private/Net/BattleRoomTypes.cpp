// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleRoomTypes.h"

FString GenerateCandidateRoomCode()
{
	FString Code;
	Code.Reserve(BattleRoomConstants::RoomCodeLength);

	const int32 AlphabetLength = BattleRoomConstants::RoomCodeAlphabet.Len();
	for (int32 Index = 0; Index < BattleRoomConstants::RoomCodeLength; ++Index)
	{
		const int32 CharIndex = FMath::RandRange(0, AlphabetLength - 1);
		Code.AppendChar(BattleRoomConstants::RoomCodeAlphabet[CharIndex]);
	}

	return Code;
}
