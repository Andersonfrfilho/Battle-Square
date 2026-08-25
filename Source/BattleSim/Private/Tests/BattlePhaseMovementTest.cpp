// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakePet(uint8 PetId, uint8 Side, uint8 Column, uint8 Row)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Column = Column;
		Pet.Row = Row;
		Pet.Health = 50;
		Pet.MaxHealth = 50;
		return Pet;
	}

	FBattleAction MoveAction(EBattleDirection Direction)
	{
		FBattleAction Action;
		Action.Type = EActionType::Mover;
		Action.Direction = Direction;
		return Action;
	}

	FBattleAction WaitAction()
	{
		FBattleAction Action;
		Action.Type = EActionType::Aguardar;
		return Action;
	}
}

// T6/BTL-04: mover para fora da grade é anulado e emite MovimentoBloqueado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementOutOfBoundsBlockedTest,
	"BattleSim.Phase.Movement.OutOfBoundsIsBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementOutOfBoundsBlockedTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 0, 0)); // canto superior esquerdo
	State.Pets.Add(MakePet(2, 1, 2, 2));

	TArray<FBattleEvent> Trace;
	// Left tenta ir para Cima (Row -1) — fora da grade.
	BattlePhases::ApplyMovement(State, MoveAction(EBattleDirection::Cima), WaitAction(), 0, Trace);

	TestEqual(TEXT("Left permanece na mesma casa"), State.Pets[0].Column, static_cast<uint8>(0));
	TestEqual(TEXT("Left permanece na mesma linha"), State.Pets[0].Row, static_cast<uint8>(0));
	TestEqual(TEXT("Um evento de bloqueio emitido"), Trace.Num(), 1);
	TestTrue(TEXT("Evento é MovimentoBloqueado"), Trace[0].Type == EBattleEventType::MovimentoBloqueado);

	return true;
}

// T6/BTL-05: dois aliados disputando a mesma casa são AMBOS anulados.
//
// LACUNA DE COBERTURA REGISTRADA EXPLICITAMENTE (ver STATE.md):
// a assinatura pública de ApplyMovement recebe UMA ação por LADO, não por
// PET — reflexo direto do contrato de rede de v1 (FTurnCommit é por lado,
// design.md). Com uma única ação por lado, dois aliados SEMPRE recebem a
// MESMA direção a partir de posições DIFERENTES, e portanto NUNCA convergem
// para o mesmo destino — colisão entre aliados é matematicamente
// inatingível pela interface pública enquanto v1 for 1 pet por lado
// (spec.md, Out of Scope: "mais de 1 pet por lado").
//
// O código de detecção de colisão em BattlePhaseMovement.cpp já existe e
// está correto (chave de agrupamento por lado+destino, ver DestinationClaimsBySide),
// mas fica INALCANÇÁVEL por este teste até M3 expandir o commit para
// per-pet. Este teste registra esse fato em vez de fingir cobertura —
// ver L-004 (afirmação sem teste é opinião): um teste que passa sem
// exercitar o caminho que afirma testar é o mesmo problema, com roupagem
// de verde.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementAllyCollisionIsUnreachableInV1Test,
	"BattleSim.Phase.Movement.AllyCollisionIsUnreachableUnderV1Contract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementAllyCollisionIsUnreachableInV1Test::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 0, 1)); // aliado A
	State.Pets.Add(MakePet(2, 0, 2, 1)); // aliado B

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State, MoveAction(EBattleDirection::Direita), WaitAction(), 0, Trace);

	// Confirma o comportamento real de v1: só o primeiro pet vivo do lado
	// recebe a ação (CollectIntent para na primeira correspondência).
	// Isto NÃO é uma limitação escondida — é o contrato de v1 documentado.
	TestEqual(TEXT("Apenas o primeiro pet do lado recebe a ação em v1"), State.Pets[0].Column, static_cast<uint8>(1));
	TestEqual(TEXT("Segundo pet do lado não é afetado em v1"), State.Pets[1].Column, static_cast<uint8>(2));

	AddWarning(TEXT("BTL-05 (colisão entre aliados) não tem cobertura de execução sob o contrato de v1 — ver comentário deste teste e STATE.md."));

	return true;
}

// T6/BTL-06: lados opostos podem coabitar a mesma casa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementOpposingSidesCoexistTest,
	"BattleSim.Phase.Movement.OpposingSidesCanCoexist",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementOpposingSidesCoexistTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 0, 1)); // Left em (0,1), vai para (1,1)
	State.Pets.Add(MakePet(2, 1, 2, 1)); // Right em (2,1), vai para (1,1)

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State, MoveAction(EBattleDirection::Direita), MoveAction(EBattleDirection::Esquerda), 0, Trace);

	TestEqual(TEXT("Left chegou em (1,1)"), State.Pets[0].Column, static_cast<uint8>(1));
	TestEqual(TEXT("Right chegou em (1,1)"), State.Pets[1].Column, static_cast<uint8>(1));
	TestEqual(TEXT("Mesma linha para os dois"), State.Pets[0].Row, State.Pets[1].Row);

	int32 MovedCount = 0;
	for (const FBattleEvent& Event : Trace)
	{
		if (Event.Type == EBattleEventType::Moveu) { ++MovedCount; }
	}
	TestEqual(TEXT("Ambos os movimentos foram bem-sucedidos"), MovedCount, 2);

	return true;
}

// T6: troca de casas entre dois pets — sem colisão, mesmo eles se cruzando.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementSwapIsAllowedTest,
	"BattleSim.Phase.Movement.SwapPositionsIsAllowed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementSwapIsAllowedTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakePet(1, 0, 0, 1)); // Left em (0,1)
	State.Pets.Add(MakePet(2, 1, 1, 1)); // Right em (1,1)

	TArray<FBattleEvent> Trace;
	// Left vai para a direita (entra em 1,1); Right vai para a esquerda (entra em 0,1) — troca.
	BattlePhases::ApplyMovement(State, MoveAction(EBattleDirection::Direita), MoveAction(EBattleDirection::Esquerda), 0, Trace);

	TestEqual(TEXT("Left terminou onde Right estava"), State.Pets[0].Column, static_cast<uint8>(1));
	TestEqual(TEXT("Right terminou onde Left estava"), State.Pets[1].Column, static_cast<uint8>(0));

	return true;
}

// T6: pet morto não gera intenção de movimento nem evento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementSkipsDeadPetTest,
	"BattleSim.Phase.Movement.SkipsDeadPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementSkipsDeadPetTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	FPetState DeadPet = MakePet(1, 0, 1, 1);
	DeadPet.Health = 0;
	State.Pets.Add(DeadPet);
	State.Pets.Add(MakePet(2, 1, 2, 2));

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State, MoveAction(EBattleDirection::Direita), WaitAction(), 0, Trace);

	TestEqual(TEXT("Pet morto não se move"), State.Pets[0].Column, static_cast<uint8>(1));
	TestEqual(TEXT("Nenhum evento para pet morto"), Trace.Num(), 0);

	return true;
}
