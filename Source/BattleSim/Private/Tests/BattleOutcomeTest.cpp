// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleOutcome.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakePet(uint8 PetId, uint8 Side, int32 Health, int32 MaxHealth)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Health = Health;
		Pet.MaxHealth = MaxHealth;
		return Pet;
	}
}

// T10/BTL-14: os dois lados zeram no mesmo F5 → empate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleOutcomeBothDeadIsDrawTest,
	"BattleSim.Outcome.BothSidesDeadIsDraw",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleOutcomeBothDeadIsDrawTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 0, 50));
	State.Pets.Add(MakePet(2, 1, 0, 50));

	TArray<FBattleEvent> Trace;
	BattleOutcome::EvaluateOutcome(State, Trace);

	TestTrue(TEXT("Batalha encerrada"), State.bBattleEnded);
	TestEqual(TEXT("Empate — WinningSide é sentinela"), State.WinningSide, static_cast<uint8>(0xFF));
	TestEqual(TEXT("Um evento BatalhaEncerrada"), Trace.Num(), 1);
	TestTrue(TEXT("Evento é BatalhaEncerrada"), Trace[0].Type == EBattleEventType::BatalhaEncerrada);

	return true;
}

// T10/BTL-14: só um lado morto → o outro vence.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleOutcomeOneSideDeadTheOtherWinsTest,
	"BattleSim.Outcome.OneSideDeadTheOtherWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleOutcomeOneSideDeadTheOtherWinsTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 0, 50)); // Left morto
	State.Pets.Add(MakePet(2, 1, 30, 50)); // Right vivo

	TArray<FBattleEvent> Trace;
	BattleOutcome::EvaluateOutcome(State, Trace);

	TestTrue(TEXT("Batalha encerrada"), State.bBattleEnded);
	TestEqual(TEXT("Right (lado 1) venceu"), State.WinningSide, static_cast<uint8>(1));

	return true;
}

// T10/BTL-14: os dois lados vivos, sem limite de turnos atingido → continua.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleOutcomeBothAliveContinuesTest,
	"BattleSim.Outcome.BothAliveBelowTurnLimitContinues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleOutcomeBothAliveContinuesTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 40, 50));
	State.Pets.Add(MakePet(2, 1, 40, 50));
	State.TurnNumber = 3; // bem abaixo do limite

	TArray<FBattleEvent> Trace;
	BattleOutcome::EvaluateOutcome(State, Trace);

	TestFalse(TEXT("Batalha NÃO encerrada"), State.bBattleEnded);
	TestEqual(TEXT("Nenhum evento emitido"), Trace.Num(), 0);

	return true;
}

// T10/BTL-15/DP-05: limite de turnos atingido, vence quem tem mais % de vida.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleOutcomeTurnLimitHigherHealthPercentWinsTest,
	"BattleSim.Outcome.TurnLimitHigherHealthPercentWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleOutcomeTurnLimitHigherHealthPercentWinsTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 40, 50)); // Left: 80%
	State.Pets.Add(MakePet(2, 1, 10, 50)); // Right: 20%
	State.TurnNumber = BattleOutcome::MaxTurns;

	TArray<FBattleEvent> Trace;
	BattleOutcome::EvaluateOutcome(State, Trace);

	TestTrue(TEXT("Batalha encerrada por limite de turnos"), State.bBattleEnded);
	TestEqual(TEXT("Left (maior %) venceu"), State.WinningSide, static_cast<uint8>(0));

	return true;
}

// T10/BTL-15: limite de turnos, percentuais EXATAMENTE iguais → empate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleOutcomeTurnLimitEqualPercentIsDrawTest,
	"BattleSim.Outcome.TurnLimitEqualHealthPercentIsDraw",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleOutcomeTurnLimitEqualPercentIsDrawTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 25, 50)); // Left: 50%
	State.Pets.Add(MakePet(2, 1, 20, 40)); // Right: 50% (denominadores diferentes, mesmo %)
	State.TurnNumber = BattleOutcome::MaxTurns;

	TArray<FBattleEvent> Trace;
	BattleOutcome::EvaluateOutcome(State, Trace);

	TestTrue(TEXT("Batalha encerrada"), State.bBattleEnded);
	TestEqual(TEXT("Percentuais iguais — empate"), State.WinningSide, static_cast<uint8>(0xFF));

	return true;
}

// T10: idempotência — batalha já encerrada não é reavaliada nem reemite evento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleOutcomeIsIdempotentTest,
	"BattleSim.Outcome.AlreadyEndedBattleIsNotReevaluated",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleOutcomeIsIdempotentTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 0, 50));
	State.Pets.Add(MakePet(2, 1, 30, 50));
	State.bBattleEnded = true;
	State.WinningSide = 1; // já decidido por uma chamada anterior

	TArray<FBattleEvent> Trace;
	BattleOutcome::EvaluateOutcome(State, Trace);

	TestEqual(TEXT("WinningSide não muda"), State.WinningSide, static_cast<uint8>(1));
	TestEqual(TEXT("Nenhum evento novo — já estava encerrada"), Trace.Num(), 0);

	return true;
}
