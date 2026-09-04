// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PrisonPetCare.h"

int32 PrisonPetCare::EffectiveCareActs(bool bHasCaretaker, int32 CareActsGiven)
{
	// Sem cuidador, nenhum cuidado — nem os atos que porventura tenham sido
	// registrados contam, porque nao ha quem os esteja dando. Com cuidador,
	// so os atos reais (nunca negativos).
	if (!bHasCaretaker)
	{
		return 0;
	}
	return FMath::Max(0, CareActsGiven);
}
