// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleRandom.h"

uint32 FBattleRandom::NextUInt32()
{
	const uint64 Previous = State;
	State = Previous * 6364136223846793005ULL + (Increment | 1ULL);

	// Saída PCG XSH-RR de 32 bits a partir do estado de 64 bits.
	const uint32 Xorshifted = static_cast<uint32>(((Previous >> 18u) ^ Previous) >> 27u);
	const uint32 Rotation = static_cast<uint32>(Previous >> 59u);
	return (Xorshifted >> Rotation) | (Xorshifted << ((~Rotation + 1u) & 31u));
}

int32 FBattleRandom::NextRange(int32 Min, int32 Max)
{
	check(Max >= Min);

	const uint32 RangeSize = static_cast<uint32>(Max - Min) + 1u;
	if (RangeSize == 0u)
	{
		// Max - Min == UINT32_MAX: intervalo cobre todo o domínio de uint32.
		return Min + static_cast<int32>(NextUInt32());
	}

	// Rejeição: descarta a fatia que produziria viés de módulo.
	const uint32 Threshold = (~RangeSize + 1u) % RangeSize;
	uint32 Sample;
	do
	{
		Sample = NextUInt32();
	} while (Sample < Threshold);

	return Min + static_cast<int32>(Sample % RangeSize);
}
