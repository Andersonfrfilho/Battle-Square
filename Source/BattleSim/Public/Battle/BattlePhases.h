// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"

struct FPetState;
struct FBattleEvent;
struct FBattleState;

// As cinco fases de um slot (design.md). Cada uma é uma função livre,
// testável isoladamente — ver T5 a T8 em tasks.md. Nenhuma delas conhece
// UWorld/AActor: tudo aqui é FBattleState mais um TArray<FBattleEvent> de
// saída.

namespace BattlePhases
{
	// F2 — Postura. Aplica Defender/Esquivar no PostureFlags de quem
	// declarou essas ações neste slot. Não toca em posição nem em vida.
	void ApplyPostures(
		FBattleState& State,
		const FBattleAction& LeftAction,
		const FBattleAction& RightAction,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace);

	// F3 — Movimento. Resolve as intenções de MOVER dos dois lados
	// SIMULTANEAMENTE: nenhuma intenção é aplicada antes de todas serem
	// lidas (design.md). Fora da grade e colisão entre aliados são
	// anulados; lados opostos coabitam (DP-02, comportamento padrão);
	// troca de casas é permitida.
	void ApplyMovement(
		FBattleState& State,
		const FBattleAction& LeftAction,
		const FBattleAction& RightAction,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace);

	// F4 — Combate. Ataque e Magia resolvem sobre as posições PÓS-movimento
	// (design.md). O dano é ACUMULADO em FPetState::PendingDamage — ver
	// BattleState.h — nunca aplicado aqui. É F5 (T8) quem aplica tudo de
	// uma vez, o que garante que dois pets que se matam no mesmo slot
	// morrem os dois (BTL-07).
	void ApplyCombat(
		FBattleState& State,
		const FBattleAction& LeftAction,
		const FBattleAction& RightAction,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace);

	// F5 — Encerramento. Aplica TODO o PendingDamage de uma vez, ANTES de
	// checar mortes (BTL-07: dois pets que se matam no mesmo slot morrem
	// os dois). Expira PostureFlags. Emite DanoAplicado, PetMorreu e
	// SlotEncerrado, nessa ordem (design.md).
	void ApplyResolution(
		FBattleState& State,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace);
}
