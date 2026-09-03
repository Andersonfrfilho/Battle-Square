// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TrainerWalletRules.h"

bool FTrainerWalletRules::GrantStartingMoneyOnce(FTrainerProfile& Profile, int32 StartingMoney)
{
	// A marca decide, não o saldo. Um jogador que gastou tudo tem saldo de
	// treinador novo — e não é um. Conceder pelo saldo faria falir virar renda.
	if (Profile.bWalletGranted)
	{
		return false;
	}

	Profile.bWalletGranted = true;
	Profile.Money += FMath::Max(0, StartingMoney);
	return true;
}

bool FTrainerWalletRules::TrySpend(FTrainerProfile& Profile, int32 Amount)
{
	if (Amount <= 0 || Profile.Money < Amount)
	{
		return false;
	}

	Profile.Money -= Amount;
	return true;
}

void FTrainerWalletRules::Earn(FTrainerProfile& Profile, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Profile.Money += Amount;
}
