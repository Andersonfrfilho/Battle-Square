// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * NEM TODO PET PODE SER MONTADO (montaria-e-trilhas, MT4).
 *
 * A regra é uma linha, mas a decisão que ela protege é grande: montar exige que
 * o DADO diga explicitamente que sim. Dado antigo, sem o campo, chega como
 * não-montável (o default seguro de `FLoadedPetRecord::bMountable`) — nunca vira
 * montável por acidente. Esta função é o ponto único que decide, para a tela e a
 * montaria concordarem.
 */
namespace MountEligibility
{
	/** Este pet pode ser montado, dado o campo do registro? */
	BATTLESQUARE_API bool CanMount(bool bRecordSaysMountable);

	/** O motivo curto da recusa, para a tela dizer por quê. */
	BATTLESQUARE_API FText RefusalReason();
}
