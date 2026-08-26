// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FEncounterCandidate
{
	FVector WorldLocation = FVector::ZeroVector;
	float EncounterRadiusUnits = 0.0f;
	FName CatalogId = NAME_None;
	bool bIsResolved = false;
};

struct FEncounterDetectionParams
{
	FVector PawnWorldLocation = FVector::ZeroVector;
	TArray<FEncounterCandidate> Candidates;
};

class BATTLESQUARE_API FEncounterDetector
{
public:
	/** Índice do encontro disparado, ou INDEX_NONE. Mais próximo vence; empate exato resolve pelo menor índice. */
	static int32 FindTriggeredEncounter(const FEncounterDetectionParams& Params);
};
