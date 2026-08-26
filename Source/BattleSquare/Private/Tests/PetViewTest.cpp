// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetView.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakePetViewDuelState()
	{
		FBattleState State;
		FPetState Left;
		Left.PetId = 1; Left.Side = 0; Left.Column = 1; Left.Row = 1;
		Left.Health = 50; Left.MaxHealth = 50; Left.Attack = 20; Left.Defense = 0;
		FPetState Right;
		Right.PetId = 2; Right.Side = 1; Right.Column = 2; Right.Row = 1;
		Right.Health = 50; Right.MaxHealth = 50; Right.Attack = 10; Right.Defense = 5;
		State.Pets.Add(Left);
		State.Pets.Add(Right);
		return State;
	}
}

// T8: SetInitialState + ApplyEvent reagem a um trace REAL de uma
// resolução completa — nunca recalculam dano/posição, só leem os campos
// que o núcleo já preencheu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetViewAppliesEventsFromRealTraceTest,
	"BattleSquare.PetView.AppliesEventsFromRealTrace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetViewAppliesEventsFromRealTraceTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakePetViewDuelState();

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
	FTurnCommit RightCommit;
	RightCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit);

	bool bFoundDamageEvent = false;
	int32 ExpectedDamage = 0;
	for (const FBattleEvent& Event : Result.Trace)
	{
		// DanoAplicado (F5) carrega quem SOFREU o dano em ActorId — não em
		// TargetId (esse campo é usado por eventos de ataque, F4).
		if (Event.Type == EBattleEventType::DanoAplicado && Event.ActorId == State.Pets[1].PetId)
		{
			bFoundDamageEvent = true;
			ExpectedDamage = Event.Value;
			break;
		}
	}
	if (!TestTrue(TEXT("Trace real contém DanoAplicado contra o alvo"), bFoundDamageEvent))
	{
		return false;
	}

	APetView* View = NewObject<APetView>();

	FPetPresentationInfo Presentation;
	Presentation.PetId = State.Pets[1].PetId;
	Presentation.Name = TEXT("AlvoTeste");

	View->SetInitialState(State.Pets[1], Presentation);
	TestEqual(TEXT("Vida cheia após SetInitialState"), View->GetHealthRatio(), 1.0f);
	TestFalse(TEXT("Não derrotado após SetInitialState"), View->IsDefeated());

	for (const FBattleEvent& Event : Result.Trace)
	{
		if (Event.TargetId == State.Pets[1].PetId || Event.ActorId == State.Pets[1].PetId)
		{
			View->ApplyEvent(Event);
		}
	}

	const float ExpectedRatio = 1.0f - (static_cast<float>(ExpectedDamage) / static_cast<float>(State.Pets[1].MaxHealth));
	TestTrue(TEXT("HealthRatio reduzido exatamente pelo Value do evento, sem recalcular"), FMath::IsNearlyEqual(View->GetHealthRatio(), ExpectedRatio, 0.001f));

	return true;
}
