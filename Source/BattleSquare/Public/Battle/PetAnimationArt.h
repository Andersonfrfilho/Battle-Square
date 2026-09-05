// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A ANIMAÇÃO DO PET, POR CONVENÇÃO DE NOME (adocao-de-arte, AR7).
 *
 * O import nomeia cada animação a partir da malha que a trouxe:
 *
 *     SK_Dragon  ->  SK_Dragon_Anim_CharacterArmature_Flying_Idle
 *     SK_Cat     ->  SK_Cat_Anim_CharacterArmature_Idle
 *
 * ou seja `<malha>_Anim_<armadura>_<ação>`. Isso é dado suficiente para achar a
 * animação de um pet SEM Animation Blueprint nenhum: o `SkeletalMeshComponent`
 * toca uma `AnimSequence` direto (modo de nó único).
 *
 * POR QUE SEM AnimBP, que era o texto original da caixa: um AnimBP é amarrado a
 * UM esqueleto, e o import criou um esqueleto por malha (128). Seriam 128
 * AnimBPs — e cada um teria de nascer à MÃO, porque a fábrica de AnimBlueprint
 * exige `TargetSkeleton`, que o MCP não passa: a tentativa abre diálogo modal e
 * TRAVA o editor (medido duas vezes em 04/09). Convenção de nome dá o mesmo
 * resultado visível — o bicho respira na tela — sem asset novo e sem janela.
 *
 * O que se perde é máquina de estados e blend. Para a vista de batalha, onde o
 * pet fica parado e age em turnos, o nó único basta; quando o jogo pedir
 * transição de verdade, o AnimBP entra por cima disto sem desfazer nada.
 */
namespace PetAnimationArt
{
	/**
	 * Os caminhos CANDIDATOS da animação parada deste pet, em ordem de
	 * tentativa. Quem chama tenta carregar cada um e fica no primeiro que vier.
	 *
	 * São vários porque as famílias de rig nomeiam a ação de jeitos diferentes
	 * — medido no import: a família voadora tem `Flying_Idle`, as de chão têm
	 * `Idle` — e a armadura também varia (`CharacterArmature` no Quaternius,
	 * `Rig_Medium` no KayKit). Tentar em ordem é honesto: nenhuma das formas é
	 * "a" forma, e adivinhar uma só deixaria famílias inteiras paradas.
	 *
	 * Caminho de malha vazio devolve lista vazia — sem malha não há animação a
	 * procurar.
	 */
	BATTLESQUARE_API TArray<FString> IdleCandidatesFor(const FString& MeshPath);
}
