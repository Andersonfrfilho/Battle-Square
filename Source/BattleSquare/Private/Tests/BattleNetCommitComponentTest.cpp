// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleNetCommitComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeCommitComponentDuelState()
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

	FNetTurnCommit MakeValidNetCommit()
	{
		FNetTurnCommit Commit;
		Commit.ActionA = { EActionType::Atacar, EBattleDirection::Direita };
		return Commit;
	}
}

// T7: payload malformado (enum fabricado fora de range) nunca chega a
// UBattleTurnCoordinator::SubmitCommit — ValidateIncomingCommit barra
// antes de ProcessValidatedCommit ser chamado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNetCommitComponentRejectsMalformedTest,
	"BattleSquare.Net.CommitComponentValidatesBeforeForwarding.RejectsMalformed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNetCommitComponentRejectsMalformedTest::RunTest(const FString& Parameters)
{
	UBattleNetCommitComponent* Component = NewObject<UBattleNetCommitComponent>();
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(MakeCommitComponentDuelState(), 0.0);
	Component->SetServerCoordinator(Coordinator, /*Side=*/0);

	FNetTurnCommit Malformed;
	Malformed.ActionA = { static_cast<EActionType>(200), EBattleDirection::Nenhuma };

	TestFalse(TEXT("ValidateIncomingCommit rejeita payload malformado"), Component->ValidateIncomingCommit(Malformed));

	// O coordenador não revalida forma — essa é responsabilidade do
	// componente. Server_SubmitCommit_Validate chama ValidateIncomingCommit,
	// e só quando ela retorna true a engine chama _Implementation (que
	// chama ProcessValidatedCommit). Provado aqui: a validação barra o
	// payload ruim antes de qualquer encaminhamento. O encadeamento real
	// com a engine (RPC atravessando uma conexão de verdade) exige rede
	// real — ver design.md, Limite de Ferramenta.

	return true;
}

// T7: payload válido é validado, convertido e encaminhado ao coordenador
// — confirmado indiretamente pelo turno resolver quando os dois lados
// processam um commit válido.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNetCommitComponentForwardsValidTest,
	"BattleSquare.Net.CommitComponentValidatesBeforeForwarding.ForwardsValid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNetCommitComponentForwardsValidTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(MakeCommitComponentDuelState(), 0.0);

	UBattleNetCommitComponent* ComponentSide0 = NewObject<UBattleNetCommitComponent>();
	ComponentSide0->SetServerCoordinator(Coordinator, /*Side=*/0);
	UBattleNetCommitComponent* ComponentSide1 = NewObject<UBattleNetCommitComponent>();
	ComponentSide1->SetServerCoordinator(Coordinator, /*Side=*/1);

	TestTrue(TEXT("Payload válido passa na validação"), ComponentSide0->ValidateIncomingCommit(MakeValidNetCommit()));

	ComponentSide0->ProcessValidatedCommit(MakeValidNetCommit());
	TestFalse(TEXT("Turno não resolve com só um lado processado"), Coordinator->IsTurnResolved());

	ComponentSide1->ProcessValidatedCommit(MakeValidNetCommit());
	TestTrue(TEXT("Turno resolve depois dos dois componentes encaminharem seus commits válidos"), Coordinator->IsTurnResolved());

	return true;
}

// T7: o componente do lado 0 recebe o resultado via
// HandleCoordinatorResolved/Client_ReceiveTurnResult_Implementation e o
// expõe por OnResultReceived — o caminho que ABattleArena vai consumir.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNetCommitComponentExposesResultTest,
	"BattleSquare.Net.CommitComponentValidatesBeforeForwarding.ExposesResultToClient",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNetCommitComponentExposesResultTest::RunTest(const FString& Parameters)
{
	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>();
	Coordinator->BeginTurn(MakeCommitComponentDuelState(), 0.0);

	UBattleNetCommitComponent* Component = NewObject<UBattleNetCommitComponent>();
	Component->SetServerCoordinator(Coordinator, /*Side=*/0);

	bool bResultReceived = false;
	int32 ReceivedTraceCount = 0;
	Component->OnResultReceived.AddLambda([&bResultReceived, &ReceivedTraceCount](const FBattleState&, const TArray<FBattleEvent>& Trace)
	{
		bResultReceived = true;
		ReceivedTraceCount = Trace.Num();
	});

	// Sem NetConnection real, chamar a implementação diretamente é o
	// caminho headless-testável (ver design.md, Limite de Ferramenta) —
	// prova a lógica de exposição do resultado, não o transporte por RPC.
	Component->Client_ReceiveTurnResult_Implementation(MakeCommitComponentDuelState(), TArray<FBattleEvent>{ FBattleEvent{} });

	TestTrue(TEXT("OnResultReceived dispara ao processar o resultado"), bResultReceived);
	TestEqual(TEXT("Trace recebido chega intacto ao delegate"), ReceivedTraceCount, 1);

	return true;
}
