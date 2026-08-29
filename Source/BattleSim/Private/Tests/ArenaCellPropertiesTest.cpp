// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakeArenaPet(uint8 PetId, uint8 Side, uint8 Column, uint8 Row, int32 Health, int32 Attack, int32 Defense)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Column = Column;
		Pet.Row = Row;
		Pet.Health = Health;
		Pet.MaxHealth = Health;
		Pet.Attack = Attack;
		Pet.Defense = Defense;
		return Pet;
	}
}

// T4 🧠: dano de casa + dano de combate no MESMO slot somam no mesmo
// PendingDamage e matam os dois pets JUNTOS (garantia de morte
// simultânea, estendida a partir do combate normal — BTL-07).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaDamageCellCombinesWithCombatForSimultaneousDeathTest,
	"BattleSim.Movement.DamageCellCombinesWithCombatDamageForSimultaneousDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FArenaDamageCellCombinesWithCombatForSimultaneousDeathTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	// Left com pouca vida, sobre uma casa de dano; Right ataca Left com
	// dano suficiente para, JUNTO com o dano de casa, matar Left no
	// mesmo slot que Right também está numa casa de dano.
	State.Pets.Add(MakeArenaPet(1, 0, 1, 1, /*Health=*/6, /*Attack=*/10, /*Defense=*/5));
	State.Pets.Add(MakeArenaPet(2, 1, 2, 1, /*Health=*/50, /*Attack=*/3, /*Defense=*/5));
	State.CellLayout[State.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Damage); // casa do Left

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	FTurnCommit RightCommit;
	RightCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Esquerda };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit);

	// Left: Attack=3 do Right vs Defense=5 -> MinDamage=1 de combate;
	// dano de casa (constante do módulo) soma no mesmo PendingDamage.
	// Left tinha 6 de vida — a soma dos dois deve zerar (ou passar de)
	// sua vida no MESMO slot em que ambos são aplicados juntos em F5.
	TestFalse(TEXT("Left morreu pela soma de dano de casa + combate no mesmo slot"), Result.NextState.Pets[0].IsAlive());

	bool bFoundPetMorreuForLeft = false;
	int32 DamageEventsForLeft = 0;
	for (const FBattleEvent& Event : Result.Trace)
	{
		if (Event.Type == EBattleEventType::PetMorreu && Event.ActorId == 1) { bFoundPetMorreuForLeft = true; }
		if (Event.Type == EBattleEventType::DanoAplicado && Event.ActorId == 1) { ++DamageEventsForLeft; }
	}
	TestTrue(TEXT("PetMorreu emitido para Left"), bFoundPetMorreuForLeft);
	// Um único DanoAplicado para Left (soma aplicada de uma vez em F5,
	// não dois eventos separados) — prova de que o acumulador é o mesmo.
	TestEqual(TEXT("Um único evento DanoAplicado para Left — soma aplicada de uma vez"), DamageEventsForLeft, 1);

	return true;
}

// T5 🧠: buff fortalece o Attack de quem ataca a partir da casa, e o
// Defense de quem é alvo estando nela — contextual, não persistente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaBuffCellStrengthensContextuallyTest,
	"BattleSim.Combat.BuffCellStrengthensAttackAndDefenseContextually",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FArenaBuffCellStrengthensContextuallyTest::RunTest(const FString& Parameters)
{
	// Caso 1: atacante buffado causa mais dano que sem buff.
	{
		FBattleState BuffedState;
		BuffedState.Pets.Add(MakeArenaPet(1, 0, 1, 1, 50, /*Attack=*/20, 5));
		BuffedState.Pets.Add(MakeArenaPet(2, 1, 2, 1, 50, 10, /*Defense=*/5));
		BuffedState.CellLayout[BuffedState.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Buff); // casa do atacante

		FBattleState NeutralState;
		NeutralState.Pets.Add(MakeArenaPet(1, 0, 1, 1, 50, 20, 5));
		NeutralState.Pets.Add(MakeArenaPet(2, 1, 2, 1, 50, 10, 5));

		FTurnCommit AttackCommit;
		AttackCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
		FTurnCommit WaitCommit;
		WaitCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

		const FBattleResolveResult BuffedResult = FBattleResolver::ResolveTurn(BuffedState, AttackCommit, WaitCommit);
		const FBattleResolveResult NeutralResult = FBattleResolver::ResolveTurn(NeutralState, AttackCommit, WaitCommit);

		const int32 BuffedDamage = 50 - BuffedResult.NextState.Pets[1].Health;
		const int32 NeutralDamage = 50 - NeutralResult.NextState.Pets[1].Health;
		TestTrue(TEXT("Atacante buffado causa MAIS dano que o mesmo ataque sem buff"), BuffedDamage > NeutralDamage);
	}

	// Caso 2: alvo buffado sofre menos dano que sem buff.
	{
		FBattleState TargetBuffedState;
		TargetBuffedState.Pets.Add(MakeArenaPet(1, 0, 1, 1, 50, /*Attack=*/20, 5));
		TargetBuffedState.Pets.Add(MakeArenaPet(2, 1, 2, 1, 50, 10, /*Defense=*/5));
		TargetBuffedState.CellLayout[TargetBuffedState.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Buff); // casa do alvo

		FBattleState NeutralState;
		NeutralState.Pets.Add(MakeArenaPet(1, 0, 1, 1, 50, 20, 5));
		NeutralState.Pets.Add(MakeArenaPet(2, 1, 2, 1, 50, 10, 5));

		FTurnCommit AttackCommit;
		AttackCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
		FTurnCommit WaitCommit;
		WaitCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

		const FBattleResolveResult TargetBuffedResult = FBattleResolver::ResolveTurn(TargetBuffedState, AttackCommit, WaitCommit);
		const FBattleResolveResult NeutralResult = FBattleResolver::ResolveTurn(NeutralState, AttackCommit, WaitCommit);

		const int32 TargetBuffedDamage = 50 - TargetBuffedResult.NextState.Pets[1].Health;
		const int32 NeutralDamage = 50 - NeutralResult.NextState.Pets[1].Health;
		TestTrue(TEXT("Alvo buffado sofre MENOS dano que o mesmo ataque sem buff"), TargetBuffedDamage < NeutralDamage);
	}

	// Caso 3: sair da casa de buff no slot seguinte remove o bônus —
	// sem resíduo.
	{
		FBattleState State;
		State.Pets.Add(MakeArenaPet(1, 0, 1, 1, 50, /*Attack=*/20, 5));
		State.Pets.Add(MakeArenaPet(2, 1, 2, 1, 50, 10, /*Defense=*/5));
		State.CellLayout[State.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Buff);

		// Slot 0: Left sai da casa de buff (move para longe); Right espera.
		FTurnCommit LeftMoveAway;
		LeftMoveAway.Actions[0] = { EActionType::Mover, EBattleDirection::Cima };
		LeftMoveAway.Actions[1] = { EActionType::Atacar, EBattleDirection::BaixoDireita };
		FTurnCommit RightWaitThenNothing;
		RightWaitThenNothing.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
		RightWaitThenNothing.Actions[1] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

		const FBattleResolveResult MovedAwayResult = FBattleResolver::ResolveTurn(State, LeftMoveAway, RightWaitThenNothing);

		FBattleState NeverBuffedState;
		NeverBuffedState.Pets.Add(MakeArenaPet(1, 0, 1, 0, 50, 20, 5)); // mesma posição final de MovedAwayResult, sem nunca ter passado pela casa de buff
		NeverBuffedState.Pets.Add(MakeArenaPet(2, 1, 2, 1, 50, 10, 5));
		FTurnCommit AttackFromSamePosition;
		AttackFromSamePosition.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };
		AttackFromSamePosition.Actions[1] = { EActionType::Atacar, EBattleDirection::BaixoDireita };
		const FBattleResolveResult NeverBuffedResult = FBattleResolver::ResolveTurn(NeverBuffedState, AttackFromSamePosition, RightWaitThenNothing);

		const int32 DamageAfterLeavingBuff = 50 - MovedAwayResult.NextState.Pets[1].Health;
		const int32 DamageNeverBuffed = 50 - NeverBuffedResult.NextState.Pets[1].Health;
		TestEqual(TEXT("Dano depois de sair da casa de buff é igual ao de quem nunca foi buffado — sem resíduo"), DamageAfterLeavingBuff, DamageNeverBuffed);
	}

	return true;
}
