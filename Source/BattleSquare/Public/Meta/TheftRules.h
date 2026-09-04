// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetOwnershipQuery.h"

/**
 * O ROUBO como consequência de batalha (crime-e-recompensa, CR2) — a decisão
 * pura: quando vencer PERMITE roubar, e o que muda quando o vencedor escolhe.
 *
 * A spec: "vencendo o dono, o roubo é consequência de uma batalha ganha" — não
 * é furto furtivo, é resultado de vitória, e NÃO é automático: o vencedor
 * decide. E só há o que roubar quando o pet tem OUTRO dono (CR1): selvagem se
 * captura, o próprio é seu, e nenhum dos dois é roubo.
 *
 * ⚠️ O GATE COMPLETO da decisão 22 (o treinador tem de estar desmaiado,
 * hipnotizado ou paralisado) espera o TREINADOR-COMO-ALVO, que é Fatia 3 de
 * `captura-roubo-e-treinador` e não existe ainda. `bTrainerVulnerable` é o
 * lugar dele: hoje quem chama passa `true` (a vitória já é a vulnerabilidade
 * mínima), e quando o estado do treinador existir, ele alimenta este campo
 * sem mudar a regra.
 */
namespace TheftRules
{
	/** O que o vencedor PODE fazer com o pet do perdedor. */
	enum class ETheftOption : uint8
	{
		/** Pode roubar — é de outro, e o treinador está vulnerável. */
		CanSteal,

		/** Não há o que roubar: selvagem (captura-se) ou já é seu. */
		Nothing,

		/** É de outro, mas o treinador não está vulnerável (decisão 22). */
		Protected,
	};

	/** O vencedor pode roubar este pet? */
	BATTLESQUARE_API ETheftOption OptionFor(
		PetOwnershipQuery::EOwnerRelation LoserPetRelation, bool bTrainerVulnerable);

	/**
	 * A escolha do vencedor produz posse nova?
	 *
	 * Só quando PODE roubar E escolheu roubar. É o aceite da task: o MESMO
	 * confronto, uma vez "roubar" e outra "não", termina em posse diferente —
	 * a escolha importa, não só a vitória.
	 */
	BATTLESQUARE_API bool TransfersOwnership(
		PetOwnershipQuery::EOwnerRelation LoserPetRelation,
		bool bTrainerVulnerable, bool bWinnerChoseToSteal);
}
