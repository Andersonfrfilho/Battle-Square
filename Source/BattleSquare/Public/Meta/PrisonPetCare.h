// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O PET CONFISCADO ENVELHECE NA PRISÃO, E ALGUÉM DECIDE CUIDAR (crime, CR10).
 *
 * O pet que a polícia confiscou (CR9) não sai do tempo: ele envelhece pelo
 * MESMO relógio do mundo que todo pet usa (`PetAging`, MV6) — CR10 não é um
 * segundo envelhecimento, é a prisão consumindo o de sempre. O que a prisão
 * muda é só QUEM cuida: por decisão 34, cuidado é ATIVO, então um pet preso
 * sem cuidador não recebe cuidado passivo nenhum — envelhece a plena
 * velocidade. "Alguém decide como cuidar dele" é literal: sem um cuidador que
 * assuma a tarefa, o cuidado é zero.
 *
 * Esta é a única regra NOVA da CR10; o resto (idade, morte, config) é `PetAging`.
 */
namespace PrisonPetCare
{
	/**
	 * Os atos de cuidado que de fato contam para um pet PRESO.
	 *
	 * Sem cuidador que assuma a tarefa, ZERO — o pet abandonado na cela
	 * envelhece a plena velocidade, e nenhum cuidado "vaza" do tempo (decisão
	 * 34, o passivo rejeitado). Com cuidador, valem os atos que ele deu.
	 */
	BATTLESQUARE_API int32 EffectiveCareActs(bool bHasCaretaker, int32 CareActsGiven);
}
