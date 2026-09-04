// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PoliceEscalation.h"

PoliceEscalation::FReinforcement PoliceEscalation::NextReinforcement(
	const FPoliceForce& Force, int32 SuspectLevel)
{
	// O degrau É a contagem de derrotas: nenhuma derrota, degrau 0 (briga
	// justa); cada policial vencido soma um. Piso em zero para um histórico
	// nunca-negativo não rebaixar o nível abaixo do suspeito.
	const int32 Tier = FMath::Max(0, Force.CopsDefeatedBySuspect);

	FReinforcement Reforco;
	Reforco.Tier = Tier;
	Reforco.PetLevel = FMath::Max(1, SuspectLevel) + Tier;
	return Reforco;
}
