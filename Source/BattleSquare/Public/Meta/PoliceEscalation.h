// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A PATENTE da força policial — o que o calor sobe (decisão 28-c).
 *
 * Declarado em escopo GLOBAL, com base explícita: enum encolhido para dentro de
 * um namespace ou de um parâmetro vira um novo tipo incompleto (a armadilha de
 * C++ que já mordeu este projeto três vezes).
 *
 * A ordem É a escada: cada valor é um degrau acima do anterior, e a comparação
 * numérica entre eles é a própria progressão do calor.
 */
enum class EPoliceForceType : uint8
{
	/** A dupla de ronda — o primeiro destacamento, par a par com o suspeito. */
	Ronda = 0,

	/** A ostensiva — uniformizada, um degrau acima. */
	Ostensiva = 1,

	/** A investigadora — sobe de novo. */
	Investigadora = 2,

	/** A tropa de elite. */
	Elite = 3,

	/** A tropa federal — o topo da escada; dali só o nível dos pets sobe. */
	Federal = 4
};

/**
 * A POLÍCIA ESCALA COM QUEM A VENCE (crime-e-recompensa, CR9 — decisões 28-b/28-c).
 *
 * Cada policial que o suspeito derrota eleva o PRÓXIMO destacamento: o calor
 * sobe a cada vitória sobre a lei e NUNCA desce sozinho. Duas coisas escalam
 * juntas — a PATENTE (a escada de cinco tipos) e o nível dos pets. A patente
 * termina na federal; o nível continua subindo além dela, para o topo nunca
 * virar briga fácil.
 *
 * Regra pura porque o calor tem de vir das DERROTAS, não do relógio (regra 5 da
 * geração procedural): dois suspeitos com o mesmo histórico enfrentam
 * exatamente a mesma polícia, e o teste consegue afirmá-lo.
 */
namespace PoliceEscalation
{
	/** Todo destacamento é uma dupla — nunca um policial solitário. */
	static constexpr int32 POLICE_PATROL_PAIR = 2;

	/** O histórico do suspeito contra a lei — só o que escala a polícia. */
	struct BATTLESQUARE_API FPoliceForce
	{
		/** Quantos policiais este suspeito já derrotou. */
		int32 CopsDefeatedBySuspect = 0;
	};

	/** O destacamento que a lei vai mandar agora. */
	struct BATTLESQUARE_API FReinforcement
	{
		/** O degrau de dificuldade — 0 é o primeiro, sobe a cada derrota. */
		int32 Tier = 0;

		/** A patente da força enviada — a escada da decisão 28-c. */
		EPoliceForceType Type = EPoliceForceType::Ronda;

		/** O nível dos pets do destacamento. */
		int32 PetLevel = 0;

		/** Quantos policiais vêm — sempre a dupla. */
		int32 OfficerCount = POLICE_PATROL_PAIR;
	};

	/**
	 * O próximo destacamento, dado o histórico do suspeito e o nível dele.
	 *
	 * Determinístico e monótono: mais derrotas nunca produzem um destacamento
	 * mais fraco nem uma patente mais baixa.
	 */
	BATTLESQUARE_API FReinforcement NextReinforcement(
		const FPoliceForce& Force, int32 SuspectLevel);

	/** O nome da patente, para o jogador ler na tela (o cartaz da força). */
	BATTLESQUARE_API FText NameOf(EPoliceForceType Type);
}
