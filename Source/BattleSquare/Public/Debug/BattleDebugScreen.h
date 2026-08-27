// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Informação de depuração NA TELA, para quem está jogando poder dizer o que
 * viu sem abrir o Log de Saída.
 *
 * POR QUE ISTO EXISTE: em 2026-08-26/27, de oito defeitos sérios desta fase,
 * sete só apareceram quando um humano olhou a tela — pets invisíveis, câmera
 * apontada para o lugar errado, "Baixo" andando para a direita, oponente
 * previsível. Nenhum teste headless podia pegá-los, e cada ciclo de
 * "joga → descreve → eu leio o log" custava caro. Escrever na tela encurta
 * isso para "joga → lê".
 *
 * Desligado por padrão fora do jogo e silenciável a qualquer momento:
 *   bs.ShowBattleDebug 0|1
 */
class BATTLESQUARE_API FBattleDebugScreen
{
public:
	/**
	 * Escreve uma linha na tela. Não faz nada em Shipping nem com o CVar
	 * desligado — chamar é sempre seguro e nunca custa em build final.
	 *
	 * @param Key   linhas com a mesma chave se SUBSTITUEM (bom para estado que
	 *              muda a cada turno); -1 empilha (bom para eventos).
	 */
	static void Show(const FString& Message, float Seconds = 8.0f,
		const FColor& Color = FColor::White, int32 Key = -1);

	/** True quando a depuração em tela está ligada. */
	static bool IsEnabled();
};
