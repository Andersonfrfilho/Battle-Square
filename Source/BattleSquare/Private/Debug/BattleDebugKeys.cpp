// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugKeys.h"

#include "Battle/BattleArena.h"
#include "Debug/BattleDebugScreen.h"
#include "EngineUtils.h"

bool FBattleDebugKeys::Handle(const FKey& Key, UWorld* World)
{
	if (Key == EKeys::F9)
	{
		FBattleDebugScreen::CopyToClipboard();
		return true;
	}

	if (Key == EKeys::F10)
	{
		FBattleDebugScreen::Clear();
		return true;
	}

#if !UE_BUILD_SHIPPING
	if (Key == EKeys::F8)
	{
		if (!World)
		{
			return false;
		}

		for (TActorIterator<ABattleArena> It(World); It; ++It)
		{
			It->SetControllingBothSides(!It->IsControllingBothSides());
		}
		return true;
	}
#endif

	return false;
}
