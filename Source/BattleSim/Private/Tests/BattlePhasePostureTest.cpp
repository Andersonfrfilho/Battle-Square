// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeTwoPetState()
	{
		FBattleState State;

		FPetState Left;
		Left.PetId = 1;
		Left.Side = 0;
		Left.Health = 50;
		Left.MaxHealth = 50;
		State.Pets.Add(Left);

		FPetState Right;
		Right.PetId = 2;
		Right.Side = 1;
		Right.Health = 50;
		Right.MaxHealth = 50;
		State.Pets.Add(Right);

		return State;
	}

	FBattleAction MakeAction(EActionType Type)
	{
		FBattleAction Action;
		Action.Type = Type;
		return Action;
	}
}

// T5: Defender e Esquivar assumem postura e emitem PosturaAssumida.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhasePostureAssumesFlagsTest,
	"BattleSim.Phase.Posture.AssumesFlagsAndEmitsEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhasePostureAssumesFlagsTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTwoPetState();
	TArray<FBattleEvent> Trace;

	BattlePhases::ApplyPostures(State, MakeAction(EActionType::Defender), MakeAction(EActionType::Esquivar), 0, Trace);

	TestEqual(TEXT("Left assumiu Defending"), State.Pets[0].PostureFlags, static_cast<uint8>(EBattlePostureFlags::Defending));
	TestEqual(TEXT("Right assumiu Dodging"), State.Pets[1].PostureFlags, static_cast<uint8>(EBattlePostureFlags::Dodging));
	TestEqual(TEXT("Dois eventos PosturaAssumida emitidos"), Trace.Num(), 2);

	for (const FBattleEvent& Event : Trace)
	{
		TestTrue(TEXT("Evento é do tipo PosturaAssumida"), Event.Type == EBattleEventType::PosturaAssumida);
	}

	return true;
}

// T5: ação que não é de postura não altera PostureFlags nem emite evento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhasePostureIgnoresNonPostureActionsTest,
	"BattleSim.Phase.Posture.IgnoresNonPostureActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhasePostureIgnoresNonPostureActionsTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTwoPetState();
	TArray<FBattleEvent> Trace;

	BattlePhases::ApplyPostures(State, MakeAction(EActionType::Atacar), MakeAction(EActionType::Aguardar), 0, Trace);

	TestEqual(TEXT("Left sem postura"), State.Pets[0].PostureFlags, static_cast<uint8>(0));
	TestEqual(TEXT("Right sem postura"), State.Pets[1].PostureFlags, static_cast<uint8>(0));
	TestEqual(TEXT("Nenhum evento emitido"), Trace.Num(), 0);

	return true;
}

// T5 — o requisito mais importante da fase: postura de um slot NÃO pode
// vazar para o slot seguinte. Simula dois slots consecutivos; no segundo,
// ninguém declara postura, e o flag do primeiro slot deve ter sido limpo
// por quem chama a fase (F5, T8) antes de F2 do slot seguinte rodar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhasePostureDoesNotLeakAcrossSlotsTest,
	"BattleSim.Phase.Posture.DoesNotLeakAcrossSlots",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhasePostureDoesNotLeakAcrossSlotsTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTwoPetState();
	TArray<FBattleEvent> TraceSlot0;

	// Slot 0: Left defende.
	BattlePhases::ApplyPostures(State, MakeAction(EActionType::Defender), MakeAction(EActionType::Aguardar), 0, TraceSlot0);
	TestEqual(TEXT("Left defendendo no slot 0"), State.Pets[0].PostureFlags, static_cast<uint8>(EBattlePostureFlags::Defending));

	// F5 do slot 0 expira a postura — é responsabilidade de T8, simulada
	// aqui para isolar o contrato: F2 nunca reseta sozinha, quem reseta é F5.
	State.Pets[0].PostureFlags = 0;

	// Slot 1: ninguém declara postura.
	TArray<FBattleEvent> TraceSlot1;
	BattlePhases::ApplyPostures(State, MakeAction(EActionType::Aguardar), MakeAction(EActionType::Aguardar), 1, TraceSlot1);

	TestEqual(TEXT("Left não está mais defendendo no slot 1"), State.Pets[0].PostureFlags, static_cast<uint8>(0));
	TestEqual(TEXT("F2 sozinha não emite evento se ninguém assume postura"), TraceSlot1.Num(), 0);

	return true;
}

// T5: pet morto não recebe postura (evita side effect fantasma).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhasePostureSkipsDeadPetTest,
	"BattleSim.Phase.Posture.SkipsDeadPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhasePostureSkipsDeadPetTest::RunTest(const FString& Parameters)
{
	FBattleState State = MakeTwoPetState();
	State.Pets[0].Health = 0; // Left morto

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyPostures(State, MakeAction(EActionType::Defender), MakeAction(EActionType::Aguardar), 0, Trace);

	TestEqual(TEXT("Pet morto não assume postura"), State.Pets[0].PostureFlags, static_cast<uint8>(0));
	TestEqual(TEXT("Nenhum evento para pet morto"), Trace.Num(), 0);

	return true;
}
