// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterDetector.h"

int32 FEncounterDetector::FindTriggeredEncounter(const FEncounterDetectionParams& Params)
{
	int32 ClosestIndex = INDEX_NONE;
	double ClosestDistanceSquared = 0.0;

	for (int32 Index = 0; Index < Params.Candidates.Num(); ++Index)
	{
		const FEncounterCandidate& Candidate = Params.Candidates[Index];
		if (Candidate.bIsResolved)
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared(Params.PawnWorldLocation, Candidate.WorldLocation);
		const double RadiusSquared = static_cast<double>(Candidate.EncounterRadiusUnits) * Candidate.EncounterRadiusUnits;
		if (DistanceSquared > RadiusSquared)
		{
			continue;
		}

		// Estritamente menor mantém o menor índice no empate exato — sem isso,
		// a ordem de iteração do mundo decidiria o desempate.
		if (ClosestIndex == INDEX_NONE || DistanceSquared < ClosestDistanceSquared)
		{
			ClosestIndex = Index;
			ClosestDistanceSquared = DistanceSquared;
		}
	}

	return ClosestIndex;
}
