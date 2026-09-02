// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

// Resultado de resolver um turno inteiro: o estado seguinte e o trace
// completo dos 3 slots. Ver design.md — é o par (estado, trace) que
// atravessa a fronteira do núcleo.
struct FBattleResolveResult
{
	FBattleState NextState;
	TArray<FBattleEvent> Trace;
};

// O coração do núcleo de simulação. FUNÇÃO ESTÁTICA E PURA — sem membro,
// sem singleton, sem estado interno (design.md, T9 em tasks.md). É o que
// torna o determinismo (BTL-16) testável em vez de presumido, e o que
// permite rodar milhares de combates headless para balanceamento.
class BATTLESIM_API FBattleResolver
{
public:
	/**
	 * Resolve o turno a partir dos commits DE CADA PET.
	 *
	 * Recebe uma lista, e não um commit por lado: era o commit por lado que
	 * impedia dois aliados de agirem no mesmo turno. Commit sem dono
	 * reconhecível é IGNORADO — inventar um dono para ele faria uma ação
	 * chegar a quem ninguém escolheu.
	 */
	static FBattleResolveResult ResolveTurn(
		const FBattleState& InState,
		TArrayView<const FTurnCommit> Commits);

	/**
	 * Monta a lista de um DUELO, endereçando cada commit ao pet vivo do lado.
	 *
	 * Existe porque o fio ainda carrega dois commits, um por lado (CP7 muda
	 * isso). É a tradução do contrato antigo para o novo, num lugar só — feita
	 * em cada chamador, ela seria a mesma regra escrita cinco vezes, e as cinco
	 * concordariam até a primeira edição.
	 */
	static TArray<FTurnCommit> DuelCommits(
		const FBattleState& InState,
		const FTurnCommit& LeftCommit,
		const FTurnCommit& RightCommit);
};
