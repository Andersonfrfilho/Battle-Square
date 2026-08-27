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

	/**
	 * Sonda: registra no painel que esta tecla chegou nesta camada.
	 *
	 * Existe porque "apertei e não aconteceu nada" não diz se a tecla foi
	 * engolida pelo editor, pelo Slate, ou se chegou e o código não a tratou.
	 * Ligada por `bs.LogKeys 1`. L-035: medir antes de consertar.
	 */
	static void Observe(const FKey& Key, const TCHAR* Camada);

	/**
	 * Escuta as teclas ANTES de todo mundo, pelo Slate.
	 *
	 * Foi preciso porque as teclas não chegavam e eu não conseguia decidir por
	 * dedução QUEM as engolia — foco do widget, modo de input, ou o editor. Um
	 * ouvinte de pré-input não depende de nenhum dos três: se a tecla existe,
	 * ela passa por aqui.
	 *
	 * É o caminho ÚNICO: ligar isto e também amarrar no PlayerController faria
	 * F7 alternar duas vezes por toque, e o resultado seria "não funciona" de
	 * novo, por motivo oposto.
	 */
	static void Install(UWorld* World);
	static void Uninstall();
};
