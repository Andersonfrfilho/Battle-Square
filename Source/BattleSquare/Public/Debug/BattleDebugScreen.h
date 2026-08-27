// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Painel de depuração NA TELA, para quem está jogando poder dizer o que viu
 * sem abrir o Log de Saída.
 *
 * POR QUE ISTO EXISTE: em 26–27/08/2026, de oito defeitos sérios desta fase,
 * sete só apareceram quando um humano olhou a tela. O ciclo "joga → descreve →
 * eu leio o log → deduzo" custava horas.
 *
 * POR QUE É UM PAINEL, e não mensagem que some: mensagem temporária obriga a
 * ler depressa e a repetir a partida para reler. O painel mantém as últimas
 * linhas visíveis o tempo todo — dá para jogar dois turnos e comparar.
 *
 * Desligado em Shipping e silenciável com:  bs.ShowBattleDebug 0
 */
class BATTLESQUARE_API FBattleDebugScreen
{
public:
	struct FLine
	{
		FString Text;
		FColor Color = FColor::White;
		/** >= 0 substitui a linha de mesma chave; < 0 empilha. */
		int32 Key = -1;
	};

	/**
	 * Acrescenta (ou atualiza) uma linha do painel.
	 *
	 * @param Key  >= 0 substitui no lugar — bom para estado que muda a cada
	 *             turno. < 0 empilha — bom para eventos.
	 */
	static void Show(const FString& Message, float Seconds = 8.0f,
		const FColor& Color = FColor::White, int32 Key = -1);

	/** Esvazia o painel. Console: bs.ClearBattleDebug */
	static void Clear();

	/**
	 * Copia o painel inteiro para a área de transferência, pronto para colar.
	 * Console: bs.CopyBattleDebug
	 *
	 * Existe porque ler da tela e transcrever à mão é lento e perde detalhe —
	 * e é justamente o detalhe (a direção exata, a casa exata) que separou
	 * "Atacar Esquerda" de "Mover Esquerda" numa investigação real.
	 */
	static void CopyToClipboard();

	/** Grava o painel em Saved/BattleDebug.txt. Chamado a cada mudança. */
	static void SaveToFile();

	static bool IsEnabled();

	/** Linhas atuais, da mais antiga para a mais recente. */
	static const TArray<FLine>& GetLines();

	/** Teto de linhas mantidas — além disso, a mais antiga sai. */
	static constexpr int32 MaxLines = 12;
};
