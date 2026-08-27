// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Barra de botões de depuração, montada em C++.
 *
 * Existe porque a tecla falhou três vezes seguidas e o console é atrito. O
 * clique é o caminho que comprovadamente FUNCIONA — a pessoa já usa os botões
 * de ação sem problema nenhum.
 *
 * Montada em Slate, e não em UMG, porque não depende de asset autorado: o WBP
 * não pode ganhar botões nesta sessão, e uma capacidade que espera edição de
 * asset é uma capacidade que não existe hoje.
 */
class BATTLESQUARE_API FBattleDebugToolbar
{
public:
	static void Show(UWorld* World);
	static void Hide();
};
