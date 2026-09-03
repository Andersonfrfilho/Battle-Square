// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * O JEITO DE JOGAR como dado (decisão 15-d) — as contas puras sobre o
 * histórico: somar uma transição, ler o peso de uma.
 *
 * Puras porque são a regra: quem colhe (a arena) e quem simula (a I.A.) leem
 * a MESMA conta — cada um com a sua cópia, divergiriam na primeira edição.
 */
namespace PlayStyleRules
{
	/** O estado de "início do turno" — o que vem antes da primeira ação. */
	inline constexpr uint8 StartOfTurn = 255;

	/** Soma um "depois de From veio To" no histórico. */
	BATTLESQUARE_API void AddTransition(TArray<FStyleTransition>& Transitions,
		uint8 From, uint8 To);

	/** Quantas vezes "depois de From veio To". Zero é "nunca visto". */
	BATTLESQUARE_API int32 TransitionCount(const TArray<FStyleTransition>& Transitions,
		uint8 From, uint8 To);
}
