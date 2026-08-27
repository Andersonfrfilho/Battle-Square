// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugScreen.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// Ligado por padrão: enquanto o jogo está sendo construído, ver o que
	// acontece vale mais que a tela limpa. Desligar com: bs.ShowBattleDebug 0
	int32 GShowBattleDebug = 1;

	FAutoConsoleVariableRef CVarShowBattleDebug(
		TEXT("bs.ShowBattleDebug"),
		GShowBattleDebug,
		TEXT("Mostra na tela o que está acontecendo na batalha (0 desliga)."),
		ECVF_Cheat);
}

bool FBattleDebugScreen::IsEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return GShowBattleDebug != 0 && GEngine != nullptr;
#endif
}

void FBattleDebugScreen::Show(const FString& Message, float Seconds, const FColor& Color, int32 Key)
{
	if (!IsEnabled())
	{
		return;
	}
	GEngine->AddOnScreenDebugMessage(Key, Seconds, Color, Message);
}
