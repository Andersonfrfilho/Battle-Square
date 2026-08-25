// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleResultWidget.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleEvent MakeBattleEndedEvent(int32 WinningSideValue)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::BatalhaEncerrada;
		Event.ActorId = BattleEventNoActor;
		Event.TargetId = BattleEventNoActor;
		Event.Value = WinningSideValue;
		return Event;
	}
}

// T12: os 3 casos (vitória, derrota, empate) lidos direto do Value do
// evento BatalhaEncerrada, do ponto de vista do jogador local — nunca
// recalculados.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleResultWidgetReadsThreeOutcomesTest,
	"BattleSquare.BattleResultWidget.ReadsAllThreeOutcomes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleResultWidgetReadsThreeOutcomesTest::RunTest(const FString& Parameters)
{
	UBattleResultWidget* Widget = NewObject<UBattleResultWidget>();

	// Jogador local é Side 0. WinningSide == 0 -> vitória.
	Widget->ApplyBattleEndedEvent(MakeBattleEndedEvent(0), /*LocalPlayerSide=*/0);
	TestEqual(TEXT("WinningSide igual ao lado local => Vitória"), Widget->Outcome, EBattleResultOutcome::Vitoria);

	// WinningSide == 1, jogador local é Side 0 -> derrota.
	Widget->ApplyBattleEndedEvent(MakeBattleEndedEvent(1), /*LocalPlayerSide=*/0);
	TestEqual(TEXT("WinningSide diferente do lado local => Derrota"), Widget->Outcome, EBattleResultOutcome::Derrota);

	// WinningSide == 0xFF -> empate, independente do lado local.
	Widget->ApplyBattleEndedEvent(MakeBattleEndedEvent(0xFF), /*LocalPlayerSide=*/0);
	TestEqual(TEXT("WinningSide 0xFF => Empate"), Widget->Outcome, EBattleResultOutcome::Empate);

	return true;
}
