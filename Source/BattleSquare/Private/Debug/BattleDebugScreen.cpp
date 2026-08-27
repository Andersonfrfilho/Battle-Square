// Copyright 2026 Anderson. All Rights Reserved.

#include "Debug/BattleDebugScreen.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// Ligado por padrão: enquanto o jogo está sendo construído, ver o que
	// acontece vale mais que a tela limpa.
	int32 GShowBattleDebug = 1;

	FAutoConsoleVariableRef CVarShowBattleDebug(
		TEXT("bs.ShowBattleDebug"),
		GShowBattleDebug,
		TEXT("Mostra o painel de depuração da batalha na tela (0 desliga)."),
		ECVF_Cheat);

	TArray<FBattleDebugScreen::FLine>& MutableLines()
	{
		static TArray<FBattleDebugScreen::FLine> Lines;
		return Lines;
	}

	FAutoConsoleCommand CmdCopyBattleDebug(
		TEXT("bs.CopyBattleDebug"),
		TEXT("Copia o painel de depuração da batalha para a área de transferência."),
		FConsoleCommandDelegate::CreateStatic(&FBattleDebugScreen::CopyToClipboard));

	FAutoConsoleCommand CmdClearBattleDebug(
		TEXT("bs.ClearBattleDebug"),
		TEXT("Esvazia o painel de depuração da batalha."),
		FConsoleCommandDelegate::CreateStatic(&FBattleDebugScreen::Clear));
}

bool FBattleDebugScreen::IsEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return GShowBattleDebug != 0;
#endif
}

const TArray<FBattleDebugScreen::FLine>& FBattleDebugScreen::GetLines()
{
	return MutableLines();
}

void FBattleDebugScreen::Clear()
{
	MutableLines().Reset();
}

void FBattleDebugScreen::Show(const FString& Message, float /*Seconds*/, const FColor& Color, int32 Key)
{
	if (!IsEnabled())
	{
		return;
	}

	TArray<FLine>& Lines = MutableLines();

	// Chave >= 0 atualiza no lugar: estado que muda a cada turno (casa, vida)
	// não pode empurrar o histórico de eventos para fora do painel.
	if (Key >= 0)
	{
		for (FLine& Existing : Lines)
		{
			if (Existing.Key == Key)
			{
				Existing.Text = Message;
				Existing.Color = Color;
				return;
			}
		}
	}

	Lines.Add(FLine{ Message, Color, Key });

	while (Lines.Num() > MaxLines)
	{
		Lines.RemoveAt(0);
	}
}

void FBattleDebugScreen::CopyToClipboard()
{
	const TArray<FLine>& Lines = MutableLines();

	FString Texto;
	for (const FLine& Line : Lines)
	{
		Texto += Line.Text;
		Texto += LINE_TERMINATOR;
	}

	FPlatformApplicationMisc::ClipboardCopy(*Texto);

	// Caminho que NÃO depende da área de transferência: em PIE ela nem sempre
	// funciona, e sem alternativa a informação simplesmente não chega a
	// ninguém. O arquivo pode ser lido direto por quem estiver ajudando.
	const FString Caminho = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BattleDebug.txt"));
	const bool bGravou = FFileHelper::SaveStringToFile(Texto, *Caminho);

	UE_LOG(LogTemp, Display, TEXT("[painel] %d linhas -> area de transferencia e %s"),
		Lines.Num(), bGravou ? *Caminho : TEXT("(falha ao gravar arquivo)"));

	// Confirmação no próprio painel: sem ela não há como saber se copiou.
	Show(FString::Printf(TEXT(">> %d linhas copiadas (e salvas em Saved/BattleDebug.txt)"), Lines.Num()),
		8.0f, FColor::Yellow, /*Key=*/999);
}
