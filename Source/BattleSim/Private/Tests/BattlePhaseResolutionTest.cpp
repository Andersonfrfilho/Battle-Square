// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakeResolutionPet(uint8 PetId, uint8 Side, int32 Health)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Health = Health;
		Pet.MaxHealth = 100;
		return Pet;
	}
}

// T8/BTL-07: TODO dano é aplicado antes de qualquer checagem de morte.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseResolutionAppliesAllDamageBeforeDeathCheckTest,
	"BattleSim.Phase.Resolution.AppliesDamageBeforeDeathCheck",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseResolutionAppliesAllDamageBeforeDeathCheckTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeResolutionPet(1, 0, 50));
	State.Pets[0].PendingDamage = 60; // mais que a vida — deve morrer

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyResolution(State, 0, Trace);

	TestEqual(TEXT("Dano aplicado integralmente"), State.Pets[0].Health, -10);
	TestFalse(TEXT("Pet está morto"), State.Pets[0].IsAlive());
	TestEqual(TEXT("PendingDamage zerado após aplicar"), State.Pets[0].PendingDamage, 0);

	return true;
}

// T8 — o requisito central da fase: dois pets que se matam no MESMO slot
// morrem OS DOIS. É a prova de que o dano é acumulado e aplicado em bloco,
// não sequencialmente (o que faria o primeiro processado "vencer").
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseResolutionMutualKillTest,
	"BattleSim.Phase.Resolution.MutualKillBothDie",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseResolutionMutualKillTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeResolutionPet(1, 0, 10));
	State.Pets.Add(MakeResolutionPet(2, 1, 10));
	State.Pets[0].PendingDamage = 15; // pet 1 recebeu dano fatal de pet 2
	State.Pets[1].PendingDamage = 15; // pet 2 recebeu dano fatal de pet 1

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyResolution(State, 0, Trace);

	TestFalse(TEXT("Pet 1 morreu"), State.Pets[0].IsAlive());
	TestFalse(TEXT("Pet 2 morreu"), State.Pets[1].IsAlive());

	int32 DeathEventCount = 0;
	for (const FBattleEvent& Event : Trace)
	{
		if (Event.Type == EBattleEventType::PetMorreu) { ++DeathEventCount; }
	}
	TestEqual(TEXT("Dois eventos PetMorreu — nenhum escapou por processar primeiro"), DeathEventCount, 2);

	return true;
}

// T8: pet já morto em slot anterior não gera PetMorreu de novo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseResolutionNoDuplicateDeathEventTest,
	"BattleSim.Phase.Resolution.AlreadyDeadPetDoesNotReemitDeathEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseResolutionNoDuplicateDeathEventTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeResolutionPet(1, 0, 0)); // já morto de um slot anterior
	State.Pets.Add(MakeResolutionPet(2, 1, 50));

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyResolution(State, 1, Trace);

	int32 DeathEventCount = 0;
	for (const FBattleEvent& Event : Trace)
	{
		if (Event.Type == EBattleEventType::PetMorreu) { ++DeathEventCount; }
	}
	TestEqual(TEXT("Nenhum PetMorreu para quem já estava morto"), DeathEventCount, 0);

	return true;
}

// T8/BTL-12: postura expira ao fim do slot, incondicionalmente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseResolutionExpiresPostureTest,
	"BattleSim.Phase.Resolution.ExpiresPostureFlags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseResolutionExpiresPostureTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeResolutionPet(1, 0, 50));
	State.Pets[0].PostureFlags = static_cast<uint8>(EBattlePostureFlags::Defending) | static_cast<uint8>(EBattlePostureFlags::Dodging);

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyResolution(State, 0, Trace);

	TestEqual(TEXT("PostureFlags zerado ao fim do slot"), State.Pets[0].PostureFlags, static_cast<uint8>(0));

	return true;
}

// T8: sem dano pendente, nenhum DanoAplicado é emitido — só SlotEncerrado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseResolutionNoOpWhenNoPendingDamageTest,
	"BattleSim.Phase.Resolution.NoDamageEventWhenNothingPending",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseResolutionNoOpWhenNoPendingDamageTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeResolutionPet(1, 0, 50));

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyResolution(State, 0, Trace);

	TestEqual(TEXT("Apenas o evento SlotEncerrado"), Trace.Num(), 1);
	TestTrue(TEXT("É SlotEncerrado"), Trace[0].Type == EBattleEventType::SlotEncerrado);

	return true;
}

// T8: ordem dos eventos é DanoAplicado(s), depois PetMorreu(s), depois SlotEncerrado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseResolutionEventOrderTest,
	"BattleSim.Phase.Resolution.EventsAreOrderedDamageThenDeathThenSlotEnd",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseResolutionEventOrderTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeResolutionPet(1, 0, 5));
	State.Pets[0].PendingDamage = 10; // vai morrer

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyResolution(State, 0, Trace);

	TestEqual(TEXT("3 eventos: DanoAplicado, PetMorreu, SlotEncerrado"), Trace.Num(), 3);
	if (Trace.Num() == 3)
	{
		TestTrue(TEXT("1º: DanoAplicado"), Trace[0].Type == EBattleEventType::DanoAplicado);
		TestTrue(TEXT("2º: PetMorreu"), Trace[1].Type == EBattleEventType::PetMorreu);
		TestTrue(TEXT("3º: SlotEncerrado"), Trace[2].Type == EBattleEventType::SlotEncerrado);
	}

	return true;
}
