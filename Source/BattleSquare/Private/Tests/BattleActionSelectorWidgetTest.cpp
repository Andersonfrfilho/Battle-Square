// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleActionSelectorWidget.h"
#include "Battle/BattleActionQueueComponent.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	struct FSelectorFixture
	{
		UWorld* World = nullptr;
		UBattleActionQueueComponent* Queue = nullptr;
		UBattleActionSelectorWidget* Widget = nullptr;
	};

	FSelectorFixture MakeFixture()
	{
		FSelectorFixture Fixture;
		Fixture.World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(Fixture.World);
		Fixture.World->InitializeActorsForPlay(FURL());

		AActor* Owner = Fixture.World->SpawnActor<AActor>();
		Fixture.Queue = NewObject<UBattleActionQueueComponent>(Owner);
		Fixture.Queue->RegisterComponent();

		// O widget é UUserWidget; para teste headless basta o objeto, sem
		// árvore de UMG — é justamente a separação que DP-ui-01 garante.
		Fixture.Widget = NewObject<UBattleActionSelectorWidget>(Owner);
		Fixture.Widget->BindToQueue(Fixture.Queue);
		return Fixture;
	}

	void Destroy(FSelectorFixture& Fixture)
	{
		if (!Fixture.World)
		{
			return;
		}
		GEngine->DestroyWorldContext(Fixture.World);
		Fixture.World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectorMirrorsStepTest,
	"BattleSquare.UI.BattleActionSelector.MirrorsSelectionStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSelectorMirrorsStepTest::RunTest(const FString& Parameters)
{
	FSelectorFixture Fixture = MakeFixture();

	TestEqual(TEXT("começa escolhendo o tipo"),
		Fixture.Widget->CurrentStep, EBattleActionSelectionStep::ChoosingType);

	// Atacar exige direção — o passo tem de avançar.
	TestTrue(TEXT("escolher Atacar é aceito"), Fixture.Widget->BeginSelectingType(EActionType::Atacar));
	TestEqual(TEXT("a tela reflete o passo de direção"),
		Fixture.Widget->CurrentStep, EBattleActionSelectionStep::ChoosingDirection);

	Destroy(Fixture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectorDirectionlessTypeTest,
	"BattleSquare.UI.BattleActionSelector.TypeWithoutDirectionConfirmsImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSelectorDirectionlessTypeTest::RunTest(const FString& Parameters)
{
	FSelectorFixture Fixture = MakeFixture();

	// Defender ignora direção — confirma na hora, sem passar pelo passo 2.
	TestTrue(TEXT("escolher Defender é aceito"), Fixture.Widget->BeginSelectingType(EActionType::Defender));
	TestEqual(TEXT("continua no passo de tipo"),
		Fixture.Widget->CurrentStep, EBattleActionSelectionStep::ChoosingType);
	TestEqual(TEXT("uma ação já foi confirmada"), Fixture.Widget->ConfirmedActionCount, 1);

	Destroy(Fixture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectorCountsAndCommitsTest,
	"BattleSquare.UI.BattleActionSelector.CountsThreeActionsThenCommits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSelectorCountsAndCommitsTest::RunTest(const FString& Parameters)
{
	FSelectorFixture Fixture = MakeFixture();

	for (int32 Index = 0; Index < 3; ++Index)
	{
		Fixture.Widget->BeginSelectingType(EActionType::Atacar);
		Fixture.Widget->ConfirmDirection(EBattleDirection::Cima);
	}

	TestEqual(TEXT("três ações confirmadas"), Fixture.Widget->ConfirmedActionCount, 3);
	TestFalse(TEXT("ainda não commitado"), Fixture.Widget->bIsCommitted);

	TestTrue(TEXT("commit é aceito com o turno cheio"), Fixture.Widget->Commit());
	TestTrue(TEXT("a tela reflete o commit"), Fixture.Widget->bIsCommitted);

	Destroy(Fixture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectorUndoTest,
	"BattleSquare.UI.BattleActionSelector.UndoLastActionBeforeCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSelectorUndoTest::RunTest(const FString& Parameters)
{
	FSelectorFixture Fixture = MakeFixture();

	Fixture.Widget->BeginSelectingType(EActionType::Defender);
	Fixture.Widget->BeginSelectingType(EActionType::Esquivar);
	TestEqual(TEXT("duas ações confirmadas"), Fixture.Widget->ConfirmedActionCount, 2);

	TestTrue(TEXT("desfazer é aceito"), Fixture.Widget->RemoveLastAction());
	TestEqual(TEXT("voltou para uma ação"), Fixture.Widget->ConfirmedActionCount, 1);

	Destroy(Fixture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectorCancelsPendingTest,
	"BattleSquare.UI.BattleActionSelector.CancelReturnsToTypeStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSelectorCancelsPendingTest::RunTest(const FString& Parameters)
{
	FSelectorFixture Fixture = MakeFixture();

	Fixture.Widget->BeginSelectingType(EActionType::Mover);
	TestEqual(TEXT("está escolhendo direção"),
		Fixture.Widget->CurrentStep, EBattleActionSelectionStep::ChoosingDirection);

	TestTrue(TEXT("cancelar é aceito"), Fixture.Widget->CancelPendingSelection());
	TestEqual(TEXT("voltou ao passo de tipo"),
		Fixture.Widget->CurrentStep, EBattleActionSelectionStep::ChoosingType);
	TestEqual(TEXT("nenhuma ação foi confirmada"), Fixture.Widget->ConfirmedActionCount, 0);

	Destroy(Fixture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectorLocksAfterCommitTest,
	"BattleSquare.UI.BattleActionSelector.LockedAfterCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSelectorLocksAfterCommitTest::RunTest(const FString& Parameters)
{
	FSelectorFixture Fixture = MakeFixture();

	for (int32 Index = 0; Index < 3; ++Index)
	{
		Fixture.Widget->BeginSelectingType(EActionType::Aguardar);
	}
	Fixture.Widget->Commit();

	// Depois do commit o turno está fechado às cegas: nada mais entra.
	TestFalse(TEXT("não aceita ação nova depois do commit"),
		Fixture.Widget->BeginSelectingType(EActionType::Atacar));
	TestFalse(TEXT("não aceita desfazer depois do commit"), Fixture.Widget->RemoveLastAction());

	Destroy(Fixture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectorNewTurnReopensQueueTest,
	"BattleSquare.UI.BattleActionSelector.NewTurnReopensTheQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSelectorNewTurnReopensQueueTest::RunTest(const FString& Parameters)
{
	FSelectorFixture Fixture = MakeFixture();

	for (int32 Index = 0; Index < 3; ++Index)
	{
		Fixture.Widget->BeginSelectingType(EActionType::Aguardar);
	}
	Fixture.Widget->Commit();
	TestTrue(TEXT("o turno fecha no commit"), Fixture.Widget->bIsCommitted);

	// Sem isto a batalha é de UMA rodada só: a fila fica commitada para
	// sempre e a tela trava em "aguardando o oponente". Foi o que apareceu
	// no primeiro turno jogado de verdade, em 2026-08-26.
	Fixture.Queue->BeginNewTurn();

	TestFalse(TEXT("o turno novo destrava a fila"), Fixture.Widget->bIsCommitted);
	TestEqual(TEXT("as ações do turno anterior não vazam"), Fixture.Widget->ConfirmedActionCount, 0);
	TestEqual(TEXT("volta ao passo de escolher tipo"),
		Fixture.Widget->CurrentStep, EBattleActionSelectionStep::ChoosingType);
	TestTrue(TEXT("e aceita ação de novo"), Fixture.Widget->BeginSelectingType(EActionType::Defender));

	Destroy(Fixture);
	return true;
}

// Quem está sendo controlado tem que aparecer ANTES da escolha, não depois de
// confirmar. No modo de escolher pelos dois lados, jogar pelo pet errado é um
// turno perdido — e a tela era a única coisa que podia avisar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleActionSelectorShowsWhoIsBeingControlledTest,
	"BattleSquare.UI.BattleActionSelector.ShowsWhoIsBeingControlled",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleActionSelectorShowsWhoIsBeingControlledTest::RunTest(const FString& Parameters)
{
	UBattleActionSelectorWidget* Widget = NewObject<UBattleActionSelectorWidget>();

	// Sem rótulo, nada é acrescentado — o modo desligado não pode poluir a
	// tela com um aviso que só faz sentido ligado.
	Widget->SetChoosingForLabel(FText::GetEmpty());

	const FText Rotulo = FText::FromString(TEXT("escolhendo pelo OPONENTE: Brisa"));
	Widget->SetChoosingForLabel(Rotulo);

	TestEqual(TEXT("O rótulo fica guardado para compor o status"),
		Widget->GetChoosingForLabel().ToString(), Rotulo.ToString());

	return true;
}
