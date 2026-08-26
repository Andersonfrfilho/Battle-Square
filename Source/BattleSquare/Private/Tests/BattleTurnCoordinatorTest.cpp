// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleTurnCoordinator.h"
#include "Net/BattleNetConstants.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeCoordinatorDuelState()
	{
		FBattleState State;
		FPetState Left;
		Left.PetId = 1; Left.Side = 0; Left.Column = 1; Left.Row = 1;
		Left.Health = 50; Left.MaxHealth = 50; Left.Attack = 20; Left.Defense = 5;
		FPetState Right;
		Right.PetId = 2; Right.Side = 1; Right.Column = 2; Right.Row = 1;
		Right.Health = 50; Right.MaxHealth = 50; Right.Attack = 10; Right.Defense = 5;
		State.Pets.Add(Left);
		State.Pets.Add(Right);
		return State;
	}

	FTurnCommit MakeAttackCommit()
	{
		FTurnCommit Commit;
		Commit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
		return Commit;
	}
}

// T4: SubmitCommit de um lado não revela nada do outro — não há getter
// público nenhum que exponha o commit antes da resolução. Este teste
// confirma o comportamento observável: só depois dos dois lados
// presentes é que o delegate dispara.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorHidesUntilBothPresentTest,
	"BattleSquare.Net.TurnCoordinatorSubmit.HidesUntilBothPresent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorHidesUntilBothPresentTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	bool bResolvedFired = false;
	Coordinator->OnTurnResolved.AddLambda([&bResolvedFired](const FBattleState&, const TArray<FBattleEvent>&) { bResolvedFired = true; });

	Coordinator->BeginTurn(MakeCoordinatorDuelState(), 0.0);

	const bool bAcceptedFirst = Coordinator->SubmitCommit(0, MakeAttackCommit());
	TestTrue(TEXT("Primeiro commit (lado 0) é aceito"), bAcceptedFirst);
	TestFalse(TEXT("Nenhuma resolução dispara com só um lado presente"), bResolvedFired);
	TestFalse(TEXT("IsTurnResolved é false com só um lado presente"), Coordinator->IsTurnResolved());

	return true;
}

// T4: segundo SubmitCommit do MESMO lado no mesmo turno é rejeitado,
// nunca sobrescreve o primeiro.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorRejectsDuplicateCommitTest,
	"BattleSquare.Net.TurnCoordinatorSubmit.RejectsDuplicateFromSameSide",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorRejectsDuplicateCommitTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(MakeCoordinatorDuelState(), 0.0);

	TestTrue(TEXT("Primeiro commit do lado 0 é aceito"), Coordinator->SubmitCommit(0, MakeAttackCommit()));
	TestFalse(TEXT("Segundo commit do lado 0, mesmo turno, é rejeitado"), Coordinator->SubmitCommit(0, MakeAttackCommit()));

	return true;
}

// T5: com os dois commits presentes, dispara o resolvedor real e emite
// o resultado via delegate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorResolvesWithBothCommitsTest,
	"BattleSquare.Net.TurnCoordinatorResolvesAndTimeout.ResolvesWithBothCommits",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorResolvesWithBothCommitsTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();

	bool bResolvedFired = false;
	int32 ReceivedTraceCount = 0;
	Coordinator->OnTurnResolved.AddLambda([&bResolvedFired, &ReceivedTraceCount](const FBattleState&, const TArray<FBattleEvent>& Trace)
	{
		bResolvedFired = true;
		ReceivedTraceCount = Trace.Num();
	});

	Coordinator->BeginTurn(MakeCoordinatorDuelState(), 0.0);
	Coordinator->SubmitCommit(0, MakeAttackCommit());
	Coordinator->SubmitCommit(1, MakeAttackCommit());

	TestTrue(TEXT("Resolução dispara assim que os dois lados estão presentes"), bResolvedFired);
	TestTrue(TEXT("Trace real recebido não é vazio"), ReceivedTraceCount > 0);
	TestTrue(TEXT("IsTurnResolved fica true"), Coordinator->IsTurnResolved());

	return true;
}

// T5: timeout preenche o lado ausente com 3x Aguardar e resolve —
// tempo é injetado, nunca lido de relógio real.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorTimeoutFillsMissingSideTest,
	"BattleSquare.Net.TurnCoordinatorResolvesAndTimeout.TimeoutFillsMissingSide",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorTimeoutFillsMissingSideTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	bool bResolvedFired = false;
	Coordinator->OnTurnResolved.AddLambda([&bResolvedFired](const FBattleState&, const TArray<FBattleEvent>&) { bResolvedFired = true; });

	Coordinator->BeginTurn(MakeCoordinatorDuelState(), /*StartTimeSeconds=*/100.0);
	Coordinator->SubmitCommit(0, MakeAttackCommit());

	// Antes do timeout: nada acontece.
	Coordinator->CheckTimeout(100.0 + BattleNetConstants::CommitTimeoutSeconds - 1.0);
	TestFalse(TEXT("Antes do timeout, turno não resolve"), bResolvedFired);

	// No/depois do timeout: lado 1 é preenchido com Aguardar e resolve.
	Coordinator->CheckTimeout(100.0 + BattleNetConstants::CommitTimeoutSeconds);
	TestTrue(TEXT("No timeout, turno resolve com o lado ausente preenchido"), bResolvedFired);
	TestTrue(TEXT("IsTurnResolved fica true após timeout"), Coordinator->IsTurnResolved());

	return true;
}

// T6 🧠: commit do lado 1 chegando ANTES do timeout (mesmo que muito
// perto) resolve com os DOIS commits reais — nunca com o preenchimento
// automático, mesmo que CheckTimeout nunca tenha sido chamado antes.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorLateCommitBeatsTimeoutTest,
	"BattleSquare.Net.TurnCoordinatorRaceOrdering.LateCommitBeatsTimeout",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorLateCommitBeatsTimeoutTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();

	int32 ResolveCallCount = 0;
	Coordinator->OnTurnResolved.AddLambda([&ResolveCallCount](const FBattleState&, const TArray<FBattleEvent>&) { ++ResolveCallCount; });

	Coordinator->BeginTurn(MakeCoordinatorDuelState(), 0.0);
	Coordinator->SubmitCommit(0, MakeAttackCommit());

	// Lado 1 commita 0.1s antes do timeout — sem CheckTimeout ter sido
	// chamado ainda. Deve resolver imediatamente com os dois commits
	// reais, e resolver só UMA vez.
	const double JustBeforeTimeout = static_cast<double>(BattleNetConstants::CommitTimeoutSeconds) - 0.1;
	const bool bAccepted = Coordinator->SubmitCommit(1, MakeAttackCommit());
	TestTrue(TEXT("Commit tardio-mas-real do lado 1 é aceito"), bAccepted);
	TestEqual(TEXT("Resolução dispara exatamente uma vez, com os dois commits reais"), ResolveCallCount, 1);

	// CheckTimeout chamado depois, no instante em que o timeout teria
	// estourado — não deve resolver de novo nem crashar.
	Coordinator->CheckTimeout(JustBeforeTimeout + 1.0);
	TestEqual(TEXT("CheckTimeout depois da resolução real não dispara de novo"), ResolveCallCount, 1);

	return true;
}

// T6 🧠: depois que o timeout já resolveu o turno (preenchendo o lado
// ausente), um SubmitCommit tardio daquele lado é rejeitado sem crash —
// o turno já acabou.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorCommitAfterTimeoutRejectedTest,
	"BattleSquare.Net.TurnCoordinatorRaceOrdering.CommitAfterTimeoutIsRejected",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorCommitAfterTimeoutRejectedTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	int32 ResolveCallCount = 0;
	Coordinator->OnTurnResolved.AddLambda([&ResolveCallCount](const FBattleState&, const TArray<FBattleEvent>&) { ++ResolveCallCount; });

	Coordinator->BeginTurn(MakeCoordinatorDuelState(), 0.0);
	Coordinator->SubmitCommit(0, MakeAttackCommit());

	// Só o lado 0 commitou; tempo avança além do timeout.
	Coordinator->CheckTimeout(static_cast<double>(BattleNetConstants::CommitTimeoutSeconds) + 1.0);
	TestEqual(TEXT("Timeout resolveu o turno uma vez"), ResolveCallCount, 1);

	// Lado 1 tenta commitar SÓ DEPOIS — o turno já resolveu.
	const bool bAcceptedTooLate = Coordinator->SubmitCommit(1, MakeAttackCommit());
	TestFalse(TEXT("Commit do lado 1 depois do timeout é rejeitado, sem crash"), bAcceptedTooLate);
	TestEqual(TEXT("Nenhuma resolução extra dispara"), ResolveCallCount, 1);

	return true;
}

// T13 (NET-07/DP-online-03): reconexão simulada — chamar
// GetCurrentBattleState diretamente entrega o FBattleState atual, sem
// exigir replay de trace nenhum.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorReconnectionReturnsCurrentStateTest,
	"BattleSquare.Net.ReconnectionAndAbandon.ReconnectionReturnsCurrentState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorReconnectionReturnsCurrentStateTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	const FBattleState StateAtBeginTurn = MakeCoordinatorDuelState();
	Coordinator->BeginTurn(StateAtBeginTurn, 0.0);

	const FBattleState& Reconnected = Coordinator->GetCurrentBattleState();
	TestEqual(TEXT("Reconexão recebe o mesmo número de pets do estado atual"), Reconnected.Pets.Num(), StateAtBeginTurn.Pets.Num());
	TestEqual(TEXT("Reconexão recebe a vida real do pet, sem recalcular"), Reconnected.Pets[0].Health, StateAtBeginTurn.Pets[0].Health);
	TestEqual(TEXT("Reconexão recebe a posição real do pet"), Reconnected.Pets[1].Column, StateAtBeginTurn.Pets[1].Column);

	return true;
}

// T13 (NET-08): jogador desconectado continua sujeito ao MESMO
// CommitTimeoutSeconds — nenhum tratamento especial que prenda o outro
// jogador além do timeout normal de commit (a ausência por queda de
// rede não é distinguível de AFK do ponto de vista do turno).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorDisconnectedStillSubjectToTimeoutTest,
	"BattleSquare.Net.ReconnectionAndAbandon.DisconnectedStillSubjectToCommitTimeout",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorDisconnectedStillSubjectToTimeoutTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	bool bResolvedFired = false;
	Coordinator->OnTurnResolved.AddLambda([&bResolvedFired](const FBattleState&, const TArray<FBattleEvent>&) { bResolvedFired = true; });

	Coordinator->BeginTurn(MakeCoordinatorDuelState(), 0.0);
	Coordinator->SubmitCommit(0, MakeAttackCommit());
	// Lado 1 "desconectado" — nunca commita. Mesmo caminho de CheckTimeout
	// de sempre, sem exceção nem prazo estendido por causa da queda.

	Coordinator->CheckTimeout(static_cast<double>(BattleNetConstants::CommitTimeoutSeconds) - 1.0);
	TestFalse(TEXT("Antes do CommitTimeoutSeconds normal, não resolve — mesma regra de sempre"), bResolvedFired);

	Coordinator->CheckTimeout(static_cast<double>(BattleNetConstants::CommitTimeoutSeconds));
	TestTrue(TEXT("No CommitTimeoutSeconds normal, resolve preenchendo o lado desconectado — nenhum prazo especial"), bResolvedFired);

	return true;
}

// T13 (NET-11): abandono declara vitória ao jogador presente via o
// evento BatalhaEncerrada já existente — nenhum EBattleEventType novo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleTurnCoordinatorAbandonmentDeclaresVictoryTest,
	"BattleSquare.Net.ReconnectionAndAbandon.AbandonmentDeclaresVictoryForPresentSide",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleTurnCoordinatorAbandonmentDeclaresVictoryTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();

	bool bAbandonmentFired = false;
	FBattleEvent ReceivedEvent;
	Coordinator->OnAbandonment.AddLambda([&bAbandonmentFired, &ReceivedEvent](const FBattleEvent& Event)
	{
		bAbandonmentFired = true;
		ReceivedEvent = Event;
	});

	// Lado 1 abandonou (nunca reconectou dentro de AbandonTimeoutSeconds)
	// — lado 0 é o presente, e vence.
	Coordinator->DeclareAbandonment(/*PresentSide=*/0);

	TestTrue(TEXT("Abandono dispara o delegate"), bAbandonmentFired);
	TestTrue(TEXT("Evento é o BatalhaEncerrada já existente, não um tipo novo"), ReceivedEvent.Type == EBattleEventType::BatalhaEncerrada);
	TestEqual(TEXT("Value do evento é o lado presente (vencedor)"), ReceivedEvent.Value, 0);

	return true;
}
