// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TrainerRankingRules.h"

void FTrainerRankingRules::AwardArenaVictory(FTrainerProfile& Profile)
{
	Profile.RankingPoints += PointsPerArenaVictory;
}
