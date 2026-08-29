// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakeCombatant(uint8 PetId, uint8 Side, uint8 Column, uint8 Row, int32 Attack, int32 Defense)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Column = Column;
		Pet.Row = Row;
		Pet.Health = 100;
		Pet.MaxHealth = 100;
		Pet.Attack = Attack;
		Pet.Defense = Defense;
		return Pet;
	}

	FBattleAction Attack(EBattleDirection Direction)
	{
		FBattleAction Action;
		Action.Type = EActionType::Atacar;
		Action.Direction = Direction;
		return Action;
	}

	FBattleAction Magic(EBattleDirection Direction)
	{
		FBattleAction Action;
		Action.Type = EActionType::Magia;
		Action.Direction = Direction;
		return Action;
	}

	FBattleAction Wait()
	{
		FBattleAction Action;
		Action.Type = EActionType::Aguardar;
		return Action;
	}
}

// T7/BTL-09: ataque direcional acerta oponente na casa alvo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatDirectionalHitTest,
	"BattleSim.Phase.Combat.DirectionalAttackHits",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatDirectionalHitTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, /*Attack=*/20, /*Defense=*/5));
	State.Pets.Add(MakeCombatant(2, 1, 2, 1, /*Attack=*/10, /*Defense=*/5)); // à direita de 1

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Direita), Wait(), 0, Trace);

	TestEqual(TEXT("Dano acumulado no alvo (20 - 5 = 15)"), State.Pets[1].PendingDamage, 15);
	TestEqual(TEXT("Vida do alvo NÃO muda em F4"), State.Pets[1].Health, 100);
	TestEqual(TEXT("Um evento de acerto"), Trace.Num(), 1);
	TestTrue(TEXT("Evento é AtaqueAcertou"), Trace[0].Type == EBattleEventType::AtaqueAcertou);
	TestEqual(TEXT("Valor do evento é o dano"), Trace[0].Value, 15);

	return true;
}

// T7: ataque numa direção sem alvo emite AtaqueErrou, sem dano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatMissTest,
	"BattleSim.Phase.Combat.AttackWithoutTargetMisses",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatMissTest::RunTest(const FString& Parameters)
{
	// CANTOS OPOSTOS, não centro contra canto.
	//
	// Antes o atacante ficava no centro (1,1) e o alvo em (0,0) — que é
	// ADJACENTE. Ele só errava porque a direção apontava para outro lado. Com
	// o alvo automático (DP-golpe-05, 2026-08-29) isso passou a acertar, e o
	// teste media a direção, não o alcance. Num 3x3, tudo é adjacente ao
	// centro: para estar fora de alcance de verdade, é preciso ir aos cantos.
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 0, 0, 20, 5));
	State.Pets.Add(MakeCombatant(2, 1, 2, 2, 10, 5)); // duas casas: fora de alcance

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Direita), Wait(), 0, Trace);

	TestEqual(TEXT("Nenhum dano acumulado"), State.Pets[1].PendingDamage, 0);
	TestEqual(TEXT("Um evento emitido"), Trace.Num(), 1);
	TestTrue(TEXT("Evento é AtaqueErrou"), Trace[0].Type == EBattleEventType::AtaqueErrou);

	return true;
}

// T7/BTL-09: Esquivar anula ataque FÍSICO.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatDodgeNullifiesAttackTest,
	"BattleSim.Phase.Combat.DodgeNullifiesPhysicalAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatDodgeNullifiesAttackTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, 20, 5));
	FPetState Dodger = MakeCombatant(2, 1, 2, 1, 10, 5);
	Dodger.PostureFlags = static_cast<uint8>(EBattlePostureFlags::Dodging);
	State.Pets.Add(Dodger);

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Direita), Wait(), 0, Trace);

	TestEqual(TEXT("Nenhum dano — esquiva anulou"), State.Pets[1].PendingDamage, 0);
	TestTrue(TEXT("Evento é Esquivou"), Trace[0].Type == EBattleEventType::Esquivou);

	return true;
}

// T7/BTL-10: Magia IGNORA esquiva — fura a esquiva, não a defesa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatMagicIgnoresDodgeTest,
	"BattleSim.Phase.Combat.MagicIgnoresDodge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatMagicIgnoresDodgeTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, 20, 5));
	FPetState Dodger = MakeCombatant(2, 1, 2, 1, 10, 5);
	Dodger.PostureFlags = static_cast<uint8>(EBattlePostureFlags::Dodging);
	State.Pets.Add(Dodger);

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Magic(EBattleDirection::Direita), Wait(), 0, Trace);

	// Magia: Attack(20) * 150% / 100 - Defense(5) = 30 - 5 = 25
	TestEqual(TEXT("Magia acerta apesar da esquiva"), State.Pets[1].PendingDamage, 25);
	TestTrue(TEXT("Evento é AtaqueAcertou, não Esquivou"), Trace[0].Type == EBattleEventType::AtaqueAcertou);

	return true;
}

// T7/BTL-11: Defender reduz dano de ataque físico.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatDefendReducesPhysicalDamageTest,
	"BattleSim.Phase.Combat.DefendReducesPhysicalDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatDefendReducesPhysicalDamageTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, 20, 10));
	FPetState Defender = MakeCombatant(2, 1, 2, 1, 10, 10);
	Defender.PostureFlags = static_cast<uint8>(EBattlePostureFlags::Defending);
	State.Pets.Add(Defender);

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Direita), Wait(), 0, Trace);

	// DefesaEfetiva = 10 * 150 / 100 = 15. Dano = max(1, 20 - 15) = 5.
	TestEqual(TEXT("Dano reduzido pela defesa ativa"), State.Pets[1].PendingDamage, 5);

	return true;
}

// T7/BTL-11: Defender reduz dano de MAGIA também — "todo dano", sem exceção.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatDefendReducesMagicDamageTest,
	"BattleSim.Phase.Combat.DefendReducesMagicDamageToo",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatDefendReducesMagicDamageTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, 20, 10));
	FPetState Defender = MakeCombatant(2, 1, 2, 1, 10, 10);
	Defender.PostureFlags = static_cast<uint8>(EBattlePostureFlags::Defending);
	State.Pets.Add(Defender);

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Magic(EBattleDirection::Direita), Wait(), 0, Trace);

	// DefesaEfetiva = 10 * 150 / 100 = 15. Magia = 20*150/100 - 15 = 30 - 15 = 15.
	TestEqual(TEXT("Defesa reduz dano de magia também"), State.Pets[1].PendingDamage, 15);

	return true;
}

// T7/BTL-13: dano mínimo garantido — nunca zero nem negativo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatMinimumDamageTest,
	"BattleSim.Phase.Combat.MinimumDamageIsGuaranteed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatMinimumDamageTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	// Ataque fraco contra defesa muito alta — dano bruto seria negativo.
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, /*Attack=*/5, 10));
	State.Pets.Add(MakeCombatant(2, 1, 2, 1, /*Attack=*/10, /*Defense=*/999));

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Direita), Wait(), 0, Trace);

	TestEqual(TEXT("Dano nunca cai abaixo do mínimo (1)"), State.Pets[1].PendingDamage, 1);
	TestTrue(TEXT("Dano mínimo nunca é zero ou negativo"), State.Pets[1].PendingDamage > 0);

	return true;
}

// Este teste já foi convertido DUAS vezes, e o histórico importa:
//
// 1. Nasceu afirmando que o oponente coabitando era alvo válido.
// 2. Em 2026-08-27, DP-02 aboliu a coabitação, e ele passou a guardar a
//    REMOÇÃO daquele caso especial — atacar com direção errada errava.
// 3. Em 2026-08-29, DP-golpe-05 tirou da direção o poder de decidir o alvo. A
//    direção deixou de importar para ataque, então "direção errada" deixou de
//    ser um conceito.
//
// Ele agora guarda a regra vigente: o alvo é o ADJACENTE, e a direção é
// ignorada. Convertido no lugar de novo — apagar transformaria duas inversões
// de regra em ausência silenciosa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatCoabitingOpponentIsValidTargetTest,
	"BattleSim.Phase.Combat.DirectionNoLongerDecidesTheTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatCoabitingOpponentIsValidTargetTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, 20, 5));
	State.Pets.Add(MakeCombatant(2, 1, 1, 1, 10, 5)); // MESMA célula — coabitando

	TArray<FBattleEvent> Trace;
	// Direção que aponta para o vazio: o golpe acerta assim mesmo, porque o
	// alvo é escolhido pela ADJACÊNCIA, não pela mira.
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Cima), Wait(), 0, Trace);

	TestTrue(TEXT("A direção não impede o acerto"),
		Trace[0].Type == EBattleEventType::AtaqueAcertou);
	TestTrue(TEXT("E o dano vai para o adjacente"), State.Pets[1].PendingDamage > 0);

	return true;
}

// T7/BTL-07: dano é acumulado, NUNCA aplicado à vida dentro de F4.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatDamageIsAccumulatedNotAppliedTest,
	"BattleSim.Phase.Combat.DamageIsAccumulatedNotApplied",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatDamageIsAccumulatedNotAppliedTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, 999, 0)); // dano altíssimo
	State.Pets.Add(MakeCombatant(2, 1, 2, 1, 10, 0));

	const int32 HealthBefore = State.Pets[1].Health;

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Direita), Wait(), 0, Trace);

	TestEqual(TEXT("Health não mudou mesmo com dano altíssimo acumulado"), State.Pets[1].Health, HealthBefore);
	TestTrue(TEXT("PendingDamage registrou o dano de verdade"), State.Pets[1].PendingDamage > 0);

	return true;
}

// T7: pet morto não é alvo válido — a busca de oponente já filtra por IsAlive().
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseCombatSkipsDeadTargetTest,
	"BattleSim.Phase.Combat.SkipsDeadTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseCombatSkipsDeadTargetTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeCombatant(1, 0, 1, 1, 20, 5));
	FPetState DeadTarget = MakeCombatant(2, 1, 2, 1, 10, 5);
	DeadTarget.Health = 0;
	State.Pets.Add(DeadTarget);

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyCombat(State, Attack(EBattleDirection::Direita), Wait(), 0, Trace);

	TestTrue(TEXT("Sem alvo vivo, o ataque erra"), Trace[0].Type == EBattleEventType::AtaqueErrou);
	TestEqual(TEXT("Nenhum dano ao pet morto"), State.Pets[1].PendingDamage, 0);

	return true;
}
