// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class UWorld;

/**
 * As teclas de depuração da batalha, num lugar só.
 *
 * DUAS portas, UMA implementação. O widget de ações tem o foco do teclado e
 * recebe as teclas antes do PlayerController; se ele engolisse, a tecla
 * morreria em silêncio — que é exatamente como o F9 falhou na primeira vez,
 * custando uma rodada inteira de "apertei e não aconteceu nada".
 *
 * Chamar de dois lugares não é duplicar a regra: a regra mora aqui, e os dois
 * chamadores são só entradas.
 */
class BATTLESQUARE_API FBattleDebugKeys
{
public:
	/** Devolve true se a tecla era de depuração e foi tratada. */
	static bool Handle(const FKey& Key, UWorld* World);
};
