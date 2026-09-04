// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PoliceEscalation.h"

#define LOCTEXT_NAMESPACE "PoliceEscalation"

PoliceEscalation::FReinforcement PoliceEscalation::NextReinforcement(
	const FPoliceForce& Force, int32 SuspectLevel)
{
	// O degrau É a contagem de derrotas: nenhuma, degrau 0 (dupla de ronda par
	// a par); cada policial vencido soma um. Piso em zero para um histórico
	// nunca-negativo não rebaixar nada.
	const int32 Tier = FMath::Max(0, Force.CopsDefeatedBySuspect);

	// A patente sobe com o degrau e TRAVA na federal (topo da escada). Além do
	// topo, só o nível dos pets continua subindo.
	const int32 TopType = static_cast<int32>(EPoliceForceType::Federal);
	const EPoliceForceType Type =
		static_cast<EPoliceForceType>(FMath::Min(Tier, TopType));

	FReinforcement Reforco;
	Reforco.Tier = Tier;
	Reforco.Type = Type;
	Reforco.PetLevel = FMath::Max(1, SuspectLevel) + Tier;
	Reforco.OfficerCount = POLICE_PATROL_PAIR;
	return Reforco;
}

FText PoliceEscalation::NameOf(EPoliceForceType Type)
{
	switch (Type)
	{
	case EPoliceForceType::Ronda:
		return LOCTEXT("ForcaRonda", "Ronda");
	case EPoliceForceType::Ostensiva:
		return LOCTEXT("ForcaOstensiva", "Polícia Ostensiva");
	case EPoliceForceType::Investigadora:
		return LOCTEXT("ForcaInvestigadora", "Investigação");
	case EPoliceForceType::Elite:
		return LOCTEXT("ForcaElite", "Tropa de Elite");
	case EPoliceForceType::Federal:
		return LOCTEXT("ForcaFederal", "Tropa Federal");
	}
	return LOCTEXT("ForcaRonda", "Ronda");
}

#undef LOCTEXT_NAMESPACE
