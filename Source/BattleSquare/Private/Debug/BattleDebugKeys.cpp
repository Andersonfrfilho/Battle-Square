// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugKeys.h"

#include "Battle/BattleArena.h"
#include "Debug/BattleDebugScreen.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

bool GbLogEveryKey = false;

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GLogKeysCommand(
		TEXT("bs.LogKeys"),
		TEXT("1 mostra no painel TODA tecla que chega ao jogo, e por qual camada. Sonda para descobrir tecla engolida."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld*)
			{
				GbLogEveryKey = Args.Num() == 0 || Args[0] != TEXT("0");
				FBattleDebugScreen::Show(
					GbLogEveryKey
						? TEXT("sonda de teclas LIGADA — aperte as teclas e veja o que chega")
						: TEXT("sonda de teclas desligada"),
					8.0f, FColor::Orange, /*Key=*/20);
			}));
}

void FBattleDebugKeys::Observe(const FKey& Key, const TCHAR* Camada)
{
	if (!GbLogEveryKey)
	{
		return;
	}

	// Key=-1: empilha. Numa sonda de tecla, a SEQUÊNCIA é a informação — qual
	// camada viu primeiro, e se a outra chegou a ver.
	FBattleDebugScreen::Show(
		FString::Printf(TEXT("tecla '%s' chegou em: %s"), *Key.ToString(), Camada),
		12.0f, FColor::Yellow, /*Key=*/-1);
}

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

	// F7, e NÃO F8: F8 é a tecla de Eject do PIE, consumida pelo editor antes
	// de chegar ao jogo. Escolhi F8 sem verificar isso e custou uma rodada.
#if !UE_BUILD_SHIPPING
	if (Key == EKeys::F7)
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

void FBattleDebugKeys::Install(UWorld* World)
{
	// Aqui havia um ouvinte de pré-input do Slate. Ele foi REMOVIDO ao
	// descobrir, compilando o alvo Shipping, que
	// OnApplicationPreInputKeyDownListener não existe fora do editor: a
	// solução nunca funcionaria em jogo empacotado, e eu não tinha verificado.
	//
	// O caminho confiável para estas ações passou a ser a BARRA DE BOTÕES
	// (FBattleDebugToolbar) — clique é o que comprovadamente funciona aqui.
	// As teclas continuam amarradas no PlayerController como conveniência.
	(void)World;
}

void FBattleDebugKeys::Uninstall()
{
}
