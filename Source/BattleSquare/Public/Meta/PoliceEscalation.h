// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A POLÍCIA ESCALA COM QUEM A VENCE (crime-e-recompensa, CR9 — decisão 28-b).
 *
 * Cada policial que o suspeito derrota eleva o nível do PRÓXIMO enviado: o
 * calor sobe a cada vitória sobre a lei e NUNCA desce sozinho. Escrever isto
 * como regra pura tem um motivo forte — o calor tem de vir das DERROTAS, não
 * do relógio (regra 5 da geração procedural): dois suspeitos com o mesmo
 * histórico enfrentam exatamente a mesma polícia, e o teste consegue afirmá-lo.
 *
 * O primeiro reforço vem no nível do próprio suspeito (briga justa, degrau 0);
 * a partir daí cada derrota soma um degrau à força do seguinte.
 */
namespace PoliceEscalation
{
	/** O histórico do suspeito contra a lei — só o que escala a polícia. */
	struct BATTLESQUARE_API FPoliceForce
	{
		/** Quantos policiais este suspeito já derrotou. */
		int32 CopsDefeatedBySuspect = 0;
	};

	/** O reforço que a lei vai mandar agora. */
	struct BATTLESQUARE_API FReinforcement
	{
		/** O degrau de dificuldade — 0 é o primeiro, sobe a cada derrota. */
		int32 Tier = 0;

		/** O nível dos pets do policial enviado. */
		int32 PetLevel = 0;
	};

	/**
	 * O próximo reforço, dado o histórico do suspeito e o nível dele.
	 *
	 * Determinístico e monótono: mais derrotas nunca produzem um reforço mais
	 * fraco. O nível do pet do policial é o do suspeito somado ao degrau — o
	 * primeiro é par a par, cada vitória sobre a lei torna o próximo um degrau
	 * mais duro.
	 */
	BATTLESQUARE_API FReinforcement NextReinforcement(
		const FPoliceForce& Force, int32 SuspectLevel);
}
