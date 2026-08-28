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

// A tradução do desfecho vive numa função só.
//
// A tela de resultado a tinha, e a arena ganhou uma cópia inline quando a
// mensagem de "VOCÊ VENCEU" foi escrita — duas verdades sobre quem venceu do
// ponto de vista do jogador. Este teste guarda a função compartilhada, que é
// onde a regra passou a morar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleOutcomeForLocalPlayerIsSharedTest,
	"BattleSquare.UI.BattleResult.OutcomeTranslationIsShared",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleOutcomeForLocalPlayerIsSharedTest::RunTest(const FString& Parameters)
{
	FBattleEvent Fim;
	Fim.Type = EBattleEventType::BatalhaEncerrada;

	// O mesmo evento significa coisas OPOSTAS para cada lado — é justamente
	// isso que uma cópia divergente estragaria em silêncio.
	Fim.Value = 0;
	TestEqual(TEXT("Lado 0 venceu, do ponto de vista do lado 0"),
		static_cast<uint8>(BattleOutcomeForLocalPlayer(Fim, 0)),
		static_cast<uint8>(EBattleResultOutcome::Vitoria));
	TestEqual(TEXT("E o mesmo evento é derrota para o lado 1"),
		static_cast<uint8>(BattleOutcomeForLocalPlayer(Fim, 1)),
		static_cast<uint8>(EBattleResultOutcome::Derrota));

	// 0xFF é empate por convenção de BattleOutcome.cpp, para os dois lados.
	Fim.Value = 0xFF;
	TestEqual(TEXT("Empate para o lado 0"),
		static_cast<uint8>(BattleOutcomeForLocalPlayer(Fim, 0)),
		static_cast<uint8>(EBattleResultOutcome::Empate));
	TestEqual(TEXT("Empate também para o lado 1"),
		static_cast<uint8>(BattleOutcomeForLocalPlayer(Fim, 1)),
		static_cast<uint8>(EBattleResultOutcome::Empate));

	// E o widget usa a MESMA função — se alguém reintroduzir uma cópia lá
	// dentro, o valor deixa de acompanhar e isto reprova.
	UBattleResultWidget* Tela = NewObject<UBattleResultWidget>();
	Fim.Value = 1;
	Tela->ApplyBattleEndedEvent(Fim, /*LocalPlayerSide=*/1);
	TestEqual(TEXT("A tela concorda com a função"),
		static_cast<uint8>(Tela->Outcome),
		static_cast<uint8>(BattleOutcomeForLocalPlayer(Fim, 1)));

	return true;
}
