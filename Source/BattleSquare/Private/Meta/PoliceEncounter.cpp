// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PoliceEncounter.h"

EPoliceAction PoliceEncounter::Decide(const FEncounter& Encontro)
{
	// Fora do alcance da ronda o policial nem observa: a maioria dos cruzamentos
	// para aqui, e é o que impede a lei de ser onisciente sobre o mapa inteiro.
	if (!Encontro.bWithinPatrolRange)
	{
		return EPoliceAction::Ignore;
	}

	// A polícia observa, não consulta o banco: reconhece pela aparência (CR7)
	// E pelo pet. As DUAS, nunca uma — é o que torna o erro da decisão 26 raro
	// em vez de constante.
	const bool bAppearanceMatches = CharacterRecognition::IsRecognized(
		Encontro.PosterAppearance, Encontro.SuspectAppearance);

	if (bAppearanceMatches && Encontro.bPetMatchesWanted)
	{
		return EPoliceAction::Arrest;
	}

	return EPoliceAction::Ignore;
}
