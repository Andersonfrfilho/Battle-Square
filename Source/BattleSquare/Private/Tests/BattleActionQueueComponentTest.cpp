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
	// MOVER confirma por direção — este é o caso que sobrou depois de
	// DP-golpe-05, e o teste passou a exercitá-lo em vez de Atacar.
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->BeginSelectingType(EActionType::Mover);
	TestTrue(TEXT("ConfirmDirection no passo certo funciona"), Queue->ConfirmDirection(EBattleDirection::Direita));
	TestEqual(TEXT("1 ação confirmada"), Queue->GetConfirmedActionCount(), 1);
	TestTrue(TEXT("Volta para ChoosingType"), Queue->GetCurrentStep() == EBattleActionSelectionStep::ChoosingType);

	const FTurnCommit Commit = Queue->BuildCommit();
	TestTrue(TEXT("Ação 0 é Mover"), Commit.Actions[0].Type == EActionType::Mover);
	TestTrue(TEXT("Direção é Direita"), Commit.Actions[0].Direction == EBattleDirection::Direita);

	// E ATACAR recusa direção: aceitar gravaria uma direção no byte que o
	// resolvedor lê como índice de golpe, e o jogador executaria um golpe que
	// não escolheu.
	UBattleActionQueueComponent* Ataque = MakeQueue();
	Ataque->BeginSelectingType(EActionType::Atacar);
	TestFalse(TEXT("Atacar não confirma por direção"),
		Ataque->ConfirmDirection(EBattleDirection::Direita));
	TestTrue(TEXT("Mas confirma por golpe"), Ataque->ConfirmMove(2));

	const FTurnCommit AtaqueCommit = Ataque->BuildCommit();
	TestEqual(TEXT("O índice do golpe viaja no commit"),
		static_cast<int32>(GetMoveIndexFromAction(AtaqueCommit.Actions[0])), 2);

	// Fora da faixa é RECUSADO, não corrigido para zero.
	UBattleActionQueueComponent* ForaDaFaixa = MakeQueue();
	ForaDaFaixa->BeginSelectingType(EActionType::Atacar);
	TestFalse(TEXT("Golpe 4 não existe"), ForaDaFaixa->ConfirmMove(4));
	TestFalse(TEXT("Golpe negativo também não"), ForaDaFaixa->ConfirmMove(-1));
	TestEqual(TEXT("E nada foi confirmado"), ForaDaFaixa->GetConfirmedActionCount(), 0);

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
	LeftQueue->ConfirmMove(0);
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

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(State,
		FBattleResolver::DuelCommits(State,
			LeftQueue->BuildCommit(), RightQueue->BuildCommit()));

	TestTrue(TEXT("Resolvedor real aceitou o commit construído pela fila"), Result.Trace.Num() > 0);
	TestTrue(TEXT("Right recebeu dano do ataque de Left"), Result.NextState.Pets[1].Health < RightPet.Health);

	return true;
}

// Skill que o pet não tem é RECUSADA pela fila.
//
// DP-skill-02: a tela esconde, mas quem recusa é o componente. Tela que só
// esconde é tela que um cliente adulterado ignora — e a regra precisa existir
// num lugar só.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueRefusesUnavailableSkillTest,
	"BattleSquare.Battle.ActionQueue.RefusesUnavailableSkill",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueRefusesUnavailableSkillTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Fila = NewObject<UBattleActionQueueComponent>();

	// Sem configuração, nada muda: é o comportamento de antes desta feature, e
	// toda batalha que não declara skills continua funcionando.
	TestTrue(TEXT("Sem restrição, voar é aceito"),
		Fila->BeginSelectingType(EActionType::Voar));

	Fila->BeginNewTurn();
	Fila->SetAvailableActions({ EActionType::Atacar, EActionType::Defender, EActionType::Camuflar });

	TestTrue(TEXT("O que o pet tem é aceito"),
		Fila->BeginSelectingType(EActionType::Defender));

	TestFalse(TEXT("O que ele NÃO tem é recusado"),
		Fila->BeginSelectingType(EActionType::Voar));

	TestEqual(TEXT("E a recusa não consome slot"),
		Fila->GetConfirmedActionCount(), 1);

	return true;
}

// A recusa do golpe trancado mora na FILA, não na tela.
//
// DP-skill-02, aplicado aos golpes: esconder o botão basta para quem joga
// pela interface e não basta para um cliente adulterado. Um teste que só
// verificasse o botão sumido aprovaria exatamente o caso que a regra existe
// para impedir.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQueueRefusesLockedMoveTest,
	"BattleSquare.Battle.ActionQueue.RefusesLockedMove",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQueueRefusesLockedMoveTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->SetUnlockedMoves({ true, false, true, true });

	Queue->BeginSelectingType(EActionType::Atacar);
	TestFalse(TEXT("Golpe 1 está trancado e é recusado"), Queue->ConfirmMove(1));
	TestEqual(TEXT("E nada foi confirmado"), Queue->GetConfirmedActionCount(), 0);

	// A seleção continua ABERTA depois da recusa: cancelar por conta própria
	// obrigaria o jogador a escolher o tipo de novo por ter clicado errado.
	TestTrue(TEXT("O golpe 2, destrancado, ainda entra"), Queue->ConfirmMove(2));
	TestEqual(TEXT("O índice preservado é o 2"),
		static_cast<int32>(GetMoveIndexFromAction(Queue->BuildCommit().Actions[0])), 2);

	return true;
}

// Lista vazia é "sem restrição" — o comportamento de antes desta feature.
// Sem isto, toda batalha que não configurasse os golpes ficaria sem golpe
// nenhum, que é a regressão que a feature inteira não pode causar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQueueWithoutUnlockListAllowsEveryMoveTest,
	"BattleSquare.Battle.ActionQueue.WithoutUnlockListAllowsEveryMove",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQueueWithoutUnlockListAllowsEveryMoveTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Queue = MakeQueue();
	Queue->BeginSelectingType(EActionType::Atacar);
	TestTrue(TEXT("Sem lista, o golpe 3 passa"), Queue->ConfirmMove(3));

	// Lista MENOR que o número de golpes não tranca o que ela não menciona:
	// o silêncio ali é ausência de informação, não proibição.
	UBattleActionQueueComponent* Curta = MakeQueue();
	Curta->SetUnlockedMoves({ false });
	Curta->BeginSelectingType(EActionType::Atacar);
	TestFalse(TEXT("O golpe 0, mencionado e trancado, é recusado"), Curta->ConfirmMove(0));
	TestTrue(TEXT("O golpe 3, não mencionado, passa"), Curta->ConfirmMove(3));

	return true;
}

// ---------------------------------------------------------------------------
// CP8 — A FILA TEM DONO.
//
// Ela montava um commit SEM dono, e isso bastava enquanto "o commit da
// esquerda" identificasse alguém. Com dois aliados não identifica mais.
//
// Escolher para qual pet é a ação é escolher DONO, não regra: a tela continua
// sem decidir nada (DP-ui-01).
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueCarriesTheOwnerTest,
	"BattleSquare.ActionQueue.CarriesTheOwner",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueCarriesTheOwnerTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Fila = NewObject<UBattleActionQueueComponent>();
	Fila->SetOwningPetId(3);

	TestEqual(TEXT("a fila sabe de quem e"), Fila->GetOwningPetId(), 3);
	TestEqual(TEXT("e o commit leva o dono"),
		static_cast<int32>(Fila->BuildCommit().PetId), 3);

	return true;
}

// CONTRAPESO — FILA SEM DONO NÃO VIRA O PET 0.
//
// Zero é "ainda não atribuído", e traduzi-lo para o pet 0 faria uma fila que
// ninguém endereçou jogar pelo primeiro pet do tabuleiro. O jogador veria o pet
// dele fazer algo que ele não pediu, e nada apontaria a fila anônima.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueWithoutOwnerIsNotPetZeroTest,
	"BattleSquare.ActionQueue.WithoutOwnerIsNotPetZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueWithoutOwnerIsNotPetZeroTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* Fila = NewObject<UBattleActionQueueComponent>();

	TestEqual(TEXT("sem dono, o commit sai com o SENTINELA"),
		static_cast<int32>(Fila->BuildCommit().PetId), 0xFF);

	return true;
}

// DUAS FILAS, DOIS DONOS, DUAS ESCOLHAS — e é o aceite da task.
//
// O defeito PARECERIA: os dois aliados andam sempre juntos, como antes desta
// feature. E sem a linha no painel ninguém saberia se foi escolha ou limitação.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionQueueTwoAlliesTwoChoicesTest,
	"BattleSquare.ActionQueue.TwoAlliesTwoChoices",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionQueueTwoAlliesTwoChoicesTest::RunTest(const FString& Parameters)
{
	UBattleActionQueueComponent* FilaA = NewObject<UBattleActionQueueComponent>();
	UBattleActionQueueComponent* FilaB = NewObject<UBattleActionQueueComponent>();
	FilaA->SetOwningPetId(1);
	FilaB->SetOwningPetId(2);

	// `BeginSelectingType` + `ConfirmDirection` — a API real da fila. A primeira
	// versão deste teste inventou `SelectActionType`/`SelectDirection`, que não
	// existem, e ele NUNCA RODOU: não compilava, e eu contei os sucessos de uma
	// bateria que nem o tinha dentro.
	FilaA->BeginSelectingType(EActionType::Mover);
	FilaA->ConfirmDirection(EBattleDirection::Esquerda);
	FilaB->BeginSelectingType(EActionType::Mover);
	FilaB->ConfirmDirection(EBattleDirection::Direita);

	const FTurnCommit CommitA = FilaA->BuildCommit();
	const FTurnCommit CommitB = FilaB->BuildCommit();

	TestEqual(TEXT("o aliado A vai para a ESQUERDA"),
		static_cast<int32>(CommitA.Actions[0].Direction),
		static_cast<int32>(EBattleDirection::Esquerda));
	TestEqual(TEXT("e o aliado B para a DIREITA, no mesmo turno"),
		static_cast<int32>(CommitB.Actions[0].Direction),
		static_cast<int32>(EBattleDirection::Direita));

	// E CADA COMMIT SABE DE QUEM É. Duas escolhas diferentes sem dono seriam
	// duas ações que o resolvedor não saberia a quem aplicar.
	TestEqual(TEXT("com o dono A"), static_cast<int32>(CommitA.PetId), 1);
	TestEqual(TEXT("e o dono B"), static_cast<int32>(CommitB.PetId), 2);

	return true;
}
