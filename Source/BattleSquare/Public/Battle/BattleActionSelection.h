// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "BattleActionSelection.generated.h"

// T1 (tasks.md, PRES-01): estado da seleção em 2 passos — resolve o
// problema de "ação = 12 botões" sinalizado em AD-009. Prefixo "Battle"
// nos tipos refletidos por disciplina (L-006/L-008): a engine já colidiu
// com nomes genéricos como EDirection e HashCombine nesta sessão.
UENUM()
enum class EBattleActionSelectionStep : uint8
{
	ChoosingType = 0,      // passo 1: qual dos 6 tipos
	ChoosingDirection,     // passo 2: só se o tipo escolhido precisar (Mover/Atacar/Magia)
};

USTRUCT()
struct FBattlePendingActionSelection
{
	GENERATED_BODY()

	UPROPERTY()
	EBattleActionSelectionStep Step = EBattleActionSelectionStep::ChoosingType;

	UPROPERTY()
	EActionType SelectedType = EActionType::Aguardar;
};

// Mover/Atacar/Magia precisam de direção; Defender/Esquivar/Aguardar
// confirmam de imediato (PRES-01, critérios 1 e 2). Classificação de UI
// — o núcleo (BattleSim) não tem esse conceito, cada fase só olha o
// campo Direction quando faz sentido para ela.
/**
 * Só MOVER precisa de direção.
 *
 * Atacar e Magia precisavam, e deixaram de precisar em 2026-08-29
 * (DP-golpe-05): o alvo é o adjacente, escolhido sozinho. O que elas pedem
 * agora é QUAL GOLPE — outro passo, com outra pergunta.
 */
FORCEINLINE bool BattleActionRequiresDirection(EActionType Type)
{
	return Type == EActionType::Mover;
}

/** Atacar e Magia pedem o golpe (DP-golpe-04). */
FORCEINLINE bool BattleActionRequiresMove(EActionType Type)
{
	return Type == EActionType::Atacar || Type == EActionType::Magia;
}
