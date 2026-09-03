// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "World/LandUseLayout.h"

/**
 * O TRABALHO — um só, oferecido em três lugares (decisão 59).
 *
 * A mecânica é uma; o que muda entre Fazenda, Criadouro e Pomar é QUAL skill
 * do pet facilita. E a regra inquebrável da spec: a skill ACELERA ou PAGA
 * MELHOR, nunca tranca — "um trabalho que EXIGE submergir tranca o jogador
 * sem" o pet certo, e é o oposto do que se quer.
 */
namespace GroundWorkRules
{
	/** Aqui se trabalha? Só os três da decisão 59. */
	BATTLESQUARE_API bool IsWorkPlace(EGroundUse Use);

	/**
	 * A skill que FACILITA em cada lugar — a diferença entre os três.
	 *
	 * Fazenda pede terra arada (Escavar); pomar pede fruta no alto (Voar);
	 * criadouro pede se aproximar de bicho arisco (Camuflar).
	 *
	 * Fora de lugar de trabalho devolve `Aguardar` — o neutro universal do
	 * enum, que não tem `Nenhuma`. É deliberado que todo pet o tenha: quem
	 * pagar bônus sem antes checar `IsWorkPlace` tem um defeito maior do que
	 * este retorno poderia causar, e o gate é dele.
	 */
	BATTLESQUARE_API EActionType FacilitatingSkillFor(EGroundUse Use);

	/**
	 * O pagamento de UM ciclo de trabalho.
	 *
	 * O bônus MULTIPLICA a base, nunca a substitui — mesmo desenho do
	 * DP-atr-09 (o estudo do dono multiplica o treino). E o caminho sem pet
	 * paga a base INTEIRA: facilitar é somar por cima, não devolver o que se
	 * tirou de quem não tem.
	 */
	BATTLESQUARE_API int32 PayFor(int32 BasePay, int32 PetBonusPercent, bool bPetHasSkill);
}
