// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleResultWidget.h"

void UBattleResultWidget::ApplyBattleEndedEvent(const FBattleEvent& Event, uint8 LocalPlayerSide)
{
	const uint8 WinningSide = static_cast<uint8>(Event.Value);

	if (WinningSide == 0xFF)
	{
		Outcome = EBattleResultOutcome::Empate;
	}
	else if (WinningSide == LocalPlayerSide)
	{
		Outcome = EBattleResultOutcome::Vitoria;
	}
	else
	{
		Outcome = EBattleResultOutcome::Derrota;
	}
}
