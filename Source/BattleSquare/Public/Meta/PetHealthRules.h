// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A VIDA PERSISTENTE do pet — as três contas, num lugar só (decisão 61).
 *
 * Puras, como a carteira e o ranking. A batalha continua decidindo a vida
 * DENTRO dela (BattleSim); aqui só se traduz o que entra e o que sai — e as
 * duas traduções moram juntas porque separadas elas divergem na primeira
 * edição.
 */
class BATTLESQUARE_API FPetHealthRules
{
public:
	static constexpr int32 FullHealthPercent = 100;

	/** Está machucado? Negativo é "não informado", e não informado é cheio. */
	static bool IsHurt(int32 HealthPercent)
	{
		return HealthPercent >= 0 && HealthPercent < FullHealthPercent;
	}

	/**
	 * O que FICA depois da batalha, em porcentagem do teto.
	 *
	 * Teto inválido devolve CHEIO, nunca zero: um pet que não se consegue
	 * medir não pode sair morto da medição.
	 */
	static int32 PercentAfterBattle(int32 Health, int32 MaxHealth);

	/**
	 * Com quanto se ENTRA na batalha, dado o teto de agora.
	 *
	 * Piso de UM: batalha que começa com zero é luta perdida antes do
	 * primeiro turno, e isso não é luta — é uma tela de derrota com passos.
	 */
	static int32 StartingHealthFor(int32 HealthPercent, int32 MaxHealth);
};
