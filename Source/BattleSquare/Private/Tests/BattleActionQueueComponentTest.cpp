// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleActionQueueComponent.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	UBattleActionQueueComponent* MakeQueue()
	{
		// UActorComponent sem Outer de Actor real — válido para uso isolado
		// em teste, já que a lógica não depende de nada do ciclo de vida
		// do ator (design.md: fila isolada de qualquer coisa visual).
		return NewObject<UBattleActionQueueComponent>();
	}
}

// T2: tipos sem direção confirmam na hora; tipos com direção avançam
// para o passo 2 sem entrar na fila ainda.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueBeginSelectingTypeTest,
	"BattleSquare.ActionQueue.BeginSelectingType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueBeginSelectingTypeTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();

	TestTrue(TEXT("Defender confirma na hora"), Queue->BeginSelectingType(EActionType::Defender));
	TestEqual(TEXT("1 ação confirmada"), Queue->GetConfirmedActionCount(), 1);
	TestTrue(TEXT("Continua no passo ChoosingType"), Queue->GetCurrentStep() == EBattleActionSelectionStep::ChoosingType);

	TestTrue(TEXT("Atacar avança para ChoosingDirection"), Queue->BeginSelectingType(EActionType::Atacar));
	TestEqual(TEXT("Ainda 1 ação confirmada — Atacar não entrou sem direção"), Queue->GetConfirmedActionCount(), 1);
	TestTrue(TEXT("Passo agora é ChoosingDirection"), Queue->GetCurrentStep() == EBattleActionSelectionStep::ChoosingDirection);

	return true;
}

// T2: fila em 3/3 recusa nova seleção.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueRefusesAtLimitTest,
	"BattleSquare.ActionQueue.RefusesSelectionAtLimit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueRefusesAtLimitTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->BeginSelectingType(EActionType::Aguardar);
	Queue->BeginSelectingType(EActionType::Aguardar);
	Queue->BeginSelectingType(EActionType::Aguardar);

	TestEqual(TEXT("3 ações confirmadas"), Queue->GetConfirmedActionCount(), 3);
	TestFalse(TEXT("4ª seleção é recusada"), Queue->BeginSelectingType(EActionType::Defender));
	TestEqual(TEXT("Continua em 3"), Queue->GetConfirmedActionCount(), 3);

	return true;
}

// T3: confirmar direção fora do passo 2 é ignorado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueConfirmDirectionOutOfStepTest,
	"BattleSquare.ActionQueue.ConfirmDirectionIgnoredOutOfStep",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueConfirmDirectionOutOfStepTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	TestFalse(TEXT("ConfirmDirection sem seleção pendente é ignorado"), Queue->ConfirmDirection(EBattleDirection::Direita));
	TestEqual(TEXT("Nenhuma ação adicionada"), Queue->GetConfirmedActionCount(), 0);
	return true;
}

// T3: fluxo completo do passo 2 — confirma e adiciona (Tipo, Direção) à fila.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueConfirmDirectionAddsActionTest,
	"BattleSquare.ActionQueue.ConfirmDirectionAddsAction",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueConfirmDirectionAddsActionTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->BeginSelectingType(EActionType::Atacar);
	TestTrue(TEXT("ConfirmDirection no passo certo funciona"), Queue->ConfirmDirection(EBattleDirection::Direita));
	TestEqual(TEXT("1 ação confirmada"), Queue->GetConfirmedActionCount(), 1);
	TestTrue(TEXT("Volta para ChoosingType"), Queue->GetCurrentStep() == EBattleActionSelectionStep::ChoosingType);

	const FTurnCommit Commit = Queue->BuildCommit();
	TestTrue(TEXT("Ação 0 é Atacar"), Commit.Actions[0].Type == EActionType::Atacar);
	TestTrue(TEXT("Direção é Direita"), Commit.Actions[0].Direction == EBattleDirection::Direita);

	return true;
}

// T3: cancelar no passo 2 volta ao passo 1 sem afetar ações já confirmadas.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueCancelPreservesConfirmedTest,
	"BattleSquare.ActionQueue.CancelPreservesConfirmedActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueCancelPreservesConfirmedTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->BeginSelectingType(EActionType::Defender); // confirmada
	Queue->BeginSelectingType(EActionType::Magia);     // pendente, passo 2

	TestTrue(TEXT("Cancelar no passo 2 funciona"), Queue->CancelPendingSelection());
	TestTrue(TEXT("Volta para ChoosingType"), Queue->GetCurrentStep() == EBattleActionSelectionStep::ChoosingType);
	TestEqual(TEXT("A ação confirmada antes continua lá — só 1"), Queue->GetConfirmedActionCount(), 1);

	return true;
}

// T4: remover a última ação preserva as anteriores; fila vazia não crasha.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueRemoveLastActionTest,
	"BattleSquare.ActionQueue.RemoveLastActionPreservesOthers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueRemoveLastActionTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();

	TestFalse(TEXT("Remover de fila vazia não crasha, retorna false"), Queue->RemoveLastAction());

	Queue->BeginSelectingType(EActionType::Defender);
	Queue->BeginSelectingType(EActionType::Esquivar);
	TestEqual(TEXT("2 ações confirmadas"), Queue->GetConfirmedActionCount(), 2);

	TestTrue(TEXT("Remove a última"), Queue->RemoveLastAction());
	TestEqual(TEXT("1 ação restante"), Queue->GetConfirmedActionCount(), 1);

	const FTurnCommit Commit = Queue->BuildCommit();
	TestTrue(TEXT("A que restou é Defender (a primeira)"), Commit.Actions[0].Type == EActionType::Defender);

	return true;
}

// T4: commit com fila incompleta preenche com Aguardar antes de travar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueCommitFillsIncompleteQueueTest,
	"BattleSquare.ActionQueue.CommitFillsIncompleteQueueWithWait",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueCommitFillsIncompleteQueueTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->BeginSelectingType(EActionType::Defender); // só 1 de 3

	TestTrue(TEXT("Commit funciona mesmo incompleto"), Queue->Commit());
	TestTrue(TEXT("Fila está commitada"), Queue->IsCommitted());

	const FTurnCommit Commit = Queue->BuildCommit();
	TestTrue(TEXT("Slot 0 é a ação real"), Commit.Actions[0].Type == EActionType::Defender);
	TestTrue(TEXT("Slot 1 preenchido com Aguardar"), Commit.Actions[1].Type == EActionType::Aguardar);
	TestTrue(TEXT("Slot 2 preenchido com Aguardar"), Commit.Actions[2].Type == EActionType::Aguardar);

	return true;
}

// T4: fila travada não aceita mais nenhuma operação.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueLockedAfterCommitTest,
	"BattleSquare.ActionQueue.LockedAfterCommit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueLockedAfterCommitTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->BeginSelectingType(EActionType::Defender);
	Queue->Commit();

	TestFalse(TEXT("BeginSelectingType recusado após commit"), Queue->BeginSelectingType(EActionType::Atacar));
	TestFalse(TEXT("RemoveLastAction recusado após commit"), Queue->RemoveLastAction());
	TestFalse(TEXT("Commit de novo é recusado (já commitado)"), Queue->Commit());

	return true;
}

// T4: o FTurnCommit construído é consumível de verdade pelo resolvedor
// real do núcleo — não é só um formato compatível na aparência.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueBuildCommitFeedsRealResolverTest,
	"BattleSquare.ActionQueue.BuildCommitFeedsRealResolver",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueBuildCommitFeedsRealResolverTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* LeftQueue = MakeQueue();
	LeftQueue->BeginSelectingType(EActionType::Atacar);
	LeftQueue->ConfirmDirection(EBattleDirection::Direita);
	LeftQueue->Commit();

	UBattleActionQueueComponent* RightQueue = MakeQueue();
	RightQueue->Commit(); // vazio -> 3x Aguardar

	FBattleState State;
	FPetState LeftPet;
	LeftPet.PetId = 1; LeftPet.Side = 0; LeftPet.Column = 1; LeftPet.Row = 1;
	LeftPet.Health = 50; LeftPet.MaxHealth = 50; LeftPet.Attack = 20; LeftPet.Defense = 5;
	FPetState RightPet;
	RightPet.PetId = 2; RightPet.Side = 1; RightPet.Column = 2; RightPet.Row = 1;
	RightPet.Health = 50; RightPet.MaxHealth = 50; RightPet.Attack = 10; RightPet.Defense = 5;
	State.Pets.Add(LeftPet);
	State.Pets.Add(RightPet);

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftQueue->BuildCommit(), RightQueue->BuildCommit());

	TestTrue(TEXT("Resolvedor real aceitou o commit construído pela fila"), Result.Trace.Num() > 0);
	TestTrue(TEXT("Right recebeu dano do ataque de Left"), Result.NextState.Pets[1].Health < RightPet.Health);

	return true;
}
