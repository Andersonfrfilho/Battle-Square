// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleResultWidget.h"

EBattleResultOutcome BattleOutcomeForLocalPlayer(const FBattleEvent& Event, uint8 LocalPlayerSide)
{
	// 0xFF é empate por convenção de BattleOutcome.cpp — lido do evento,
	// nunca recalculado a partir do estado.
	const uint8 WinningSide = static_cast<uint8>(Event.Value);

	if (WinningSide == 0xFF)
	{
		return EBattleResultOutcome::Empate;
	}
	return (WinningSide == LocalPlayerSide)
		? EBattleResultOutcome::Vitoria
		: EBattleResultOutcome::Derrota;
}

void UBattleResultWidget::ApplyBattleEndedEvent(const FBattleEvent& Event, uint8 LocalPlayerSide)
{
	Outcome = BattleOutcomeForLocalPlayer(Event, LocalPlayerSide);
}
