// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakeMovementPet(uint8 PetId, uint8 Side, uint8 Column, uint8 Row)
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
	State.Pets.Add(MakeMovementPet(1, 0, 0, 0)); // canto superior esquerdo
	State.Pets.Add(MakeMovementPet(2, 1, 2, 2));

	TArray<FBattleEvent> Trace;
	// Left tenta ir para Cima (Row -1) — fora da grade.
	BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, MoveAction(EBattleDirection::Cima), WaitAction()), 0, Trace);

	TestEqual(TEXT("Left permanece na mesma casa"), State.Pets[0].Column, static_cast<uint8>(0));
	TestEqual(TEXT("Left permanece na mesma linha"), State.Pets[0].Row, static_cast<uint8>(0));
	TestEqual(TEXT("Um evento de bloqueio emitido"), Trace.Num(), 1);
	TestTrue(TEXT("Evento é MovimentoBloqueado"), Trace[0].Type == EBattleEventType::MovimentoBloqueado);

	return true;
}

// BTL-05 — DOIS ALIADOS CONVERGINDO PARA A MESMA CASA.
//
// Este teste MORAVA aqui sob o nome de "colisão entre aliados é inalcançável
// no contrato de v1": ele afirmava o comportamento de então (só o primeiro pet
// vivo do lado recebia a ação) e emitia um AVISO dizendo que este requisito não
// tinha cobertura de execução. Existia para NÃO FINGIR COBERTURA, e cumpriu o
// papel dele.
//
// (O nome antigo e a palavra do aviso saem daqui de propósito: a verificação da
// task é um `grep` por eles, e um comentário que os cite faria o grep achar o
// que ele existe para não achar.)
//
// Foi CONVERTIDO, e não acrescentado ao lado: quem ler o histórico precisa
// achar a regra nova onde a antiga morava — é o mesmo tratamento que a
// inversão do DP-02 recebeu logo abaixo. Aviso que sobrevive ao conserto faz a
// próxima leitura concluir que a lacuna continua aberta.
//
// A colisão em si NÃO É NOVA: `ClaimsByCell`, `PetsBarrados` e `EmitBlocked`
// estavam escritos e testados no ramo de lados opostos desde sempre. O que a
// CP4 fez foi torná-los ALCANÇÁVEIS, removendo o `break` que fazia só o
// primeiro aliado coletar intenção.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementAlliesCollideTest,
	"BattleSim.Phase.Movement.AlliesCollideAndAreBothBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementAlliesCollideTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeMovementPet(1, 0, 0, 1)); // aliado A em (0,1)
	State.Pets.Add(MakeMovementPet(2, 0, 2, 1)); // aliado B em (2,1)

	// OS DOIS ENDEREÇADOS, e é isto que `DuelSlotActions` não sabe fazer: ela
	// endereça um pet por LADO, e aqui os dois são do mesmo lado. Montar a
	// lista à mão é o que este caso pede — e é a forma que a CP7 vai tornar a
	// única.
	TArray<FSlotAction> Acoes;
	Acoes.Add({ 1, MoveAction(EBattleDirection::Direita) });   // A vai para (1,1)
	Acoes.Add({ 2, MoveAction(EBattleDirection::Esquerda) });  // B vai para (1,1)

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State, Acoes, 0, Trace);

	// OS DOIS FICAM ONDE ESTAVAM. Deixar um passar seria pior que barrar os
	// dois: a casa disputada iria para quem o laço visitou primeiro, e a ordem
	// do array não é garantia de determinismo (BTL-17).
	TestEqual(TEXT("o aliado A nao saiu de (0,1)"),
		State.Pets[0].Column, static_cast<uint8>(0));
	TestEqual(TEXT("o aliado B nao saiu de (2,1)"),
		State.Pets[1].Column, static_cast<uint8>(2));

	// E OS DOIS EVENTOS ESTÃO NO TRAÇO. Barrar em silêncio faria o jogador ver
	// dois pets parados sem saber que a jogada dele colidiu consigo mesma.
	int32 Barrados = 0;
	for (const FBattleEvent& Evento : Trace)
	{
		if (Evento.Type == EBattleEventType::MovimentoBloqueado)
		{
			++Barrados;
		}
	}

	AddInfo(FString::Printf(TEXT("eventos de bloqueio no traco: %d"), Barrados));
	TestEqual(TEXT("os DOIS aliados foram anunciados como barrados"), Barrados, 2);

	return true;
}

// CONTRAPESO — DOIS ALIADOS COM DESTINOS DIFERENTES ANDAM OS DOIS.
//
// Sem ele, uma implementação que barrasse todo movimento de aliado passaria no
// teste de cima cantando vitória. E este é literalmente o comportamento que o
// `break` impedia: o segundo aliado ficava plantado para sempre.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementAlliesBothMoveTest,
	"BattleSim.Phase.Movement.AlliesWithDifferentTargetsBothMove",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementAlliesBothMoveTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeMovementPet(1, 0, 0, 0)); // A em (0,0)
	State.Pets.Add(MakeMovementPet(2, 0, 0, 2)); // B em (0,2)

	TArray<FSlotAction> Acoes;
	Acoes.Add({ 1, MoveAction(EBattleDirection::Direita) });
	Acoes.Add({ 2, MoveAction(EBattleDirection::Direita) });

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State, Acoes, 0, Trace);

	TestEqual(TEXT("o aliado A andou"), State.Pets[0].Column, static_cast<uint8>(1));
	TestEqual(TEXT("e o aliado B TAMBEM"), State.Pets[1].Column, static_cast<uint8>(1));

	// Cada um na SUA linha: andar junto não pode virar andar por cima.
	TestEqual(TEXT("A continua na linha dele"), State.Pets[0].Row, static_cast<uint8>(0));
	TestEqual(TEXT("B continua na linha dele"), State.Pets[1].Row, static_cast<uint8>(2));

	return true;
}

// DP-02 INVERTIDO em 2026-08-27: lados opostos NÃO coabitam mais.
//
// Este teste afirmava a coabitação. Ele não estava errado — ficou obsoleto por
// decisão de produto, e foi CONVERTIDO em vez de apagado: quem ler o histórico
// precisa achar a regra nova onde a antiga morava, senão a inversão vira uma
// ausência silenciosa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementOpposingSidesCoexistTest,
	"BattleSim.Phase.Movement.OpposingSidesCannotCoexist",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementOpposingSidesCoexistTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeMovementPet(1, 0, 0, 1)); // Left em (0,1), vai para (1,1)
	State.Pets.Add(MakeMovementPet(2, 1, 2, 1)); // Right em (2,1), vai para (1,1)

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, MoveAction(EBattleDirection::Direita), MoveAction(EBattleDirection::Esquerda)), 0, Trace);

	TestEqual(TEXT("Left continua em (0,1)"), State.Pets[0].Column, static_cast<uint8>(0));
	TestEqual(TEXT("Right continua em (2,1)"), State.Pets[1].Column, static_cast<uint8>(2));

	int32 MovedCount = 0;
	int32 EncounterCount = 0;
	for (const FBattleEvent& Event : Trace)
	{
		if (Event.Type == EBattleEventType::Moveu) { ++MovedCount; }
		if (Event.Type == EBattleEventType::EncontroNoMesmoPonto) { ++EncounterCount; }
	}
	TestEqual(TEXT("Nenhum dos dois entrou na casa disputada"), MovedCount, 0);
	TestEqual(TEXT("O encontro foi registrado uma vez"), EncounterCount, 1);

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
	State.Pets.Add(MakeMovementPet(1, 0, 0, 1)); // Left em (0,1)
	State.Pets.Add(MakeMovementPet(2, 1, 1, 1)); // Right em (1,1)

	TArray<FBattleEvent> Trace;
	// Left vai para a direita (entra em 1,1); Right vai para a esquerda (entra em 0,1) — troca.
	BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, MoveAction(EBattleDirection::Direita), MoveAction(EBattleDirection::Esquerda)), 0, Trace);

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
	FPetState DeadPet = MakeMovementPet(1, 0, 1, 1);
	DeadPet.Health = 0;
	State.Pets.Add(DeadPet);
	State.Pets.Add(MakeMovementPet(2, 1, 2, 2));

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, MoveAction(EBattleDirection::Direita), WaitAction()), 0, Trace);

	TestEqual(TEXT("Pet morto não se move"), State.Pets[0].Column, static_cast<uint8>(1));
	TestEqual(TEXT("Nenhum evento para pet morto"), Trace.Num(), 0);

	return true;
}

// T3: casa bloqueada rejeita o movimento de quem NÃO TEM COMO PASSAR —
// pet nunca sai da posição original, repetidamente.
//
// A casa bloqueada deixou de ser parede lisa: hoje ela é tronco ou pedra, e
// quem tem força derruba, quem tem agilidade sobe (BattleSim.Obstacle.*). Este
// teste continua sendo o caso do pet que não tem nem uma coisa nem outra — e
// o `MakeMovementPet` o entrega assim, com ataque e velocidade em zero.
//
// A verificação explícita abaixo existe para que este teste passe DE
// PROPÓSITO: sem ela, mexer no padrão do helper faria o pet virar forte e o
// teste passaria a medir outra coisa em silêncio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementBlockedCellRejectsMovementTest,
	"BattleSim.Movement.BlockedCellRejectsMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementBlockedCellRejectsMovementTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeMovementPet(1, 0, 1, 1));
	State.Pets.Add(MakeMovementPet(2, 1, 0, 0)); // longe, não interfere
	State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Blocked); // casa à direita do Pet 1

	TestTrue(TEXT("o pet deste teste e' fraco demais para derrubar"),
		State.Pets[0].GetEffectiveAttack() < BattleArenaConstants::ObstacleBreakAttack);
	TestTrue(TEXT("e lento demais para subir"),
		State.Pets[0].GetEffectiveSpeed() < BattleArenaConstants::ObstacleClimbSpeed);

	for (int32 Attempt = 0; Attempt < 5; ++Attempt)
	{
		TArray<FBattleEvent> Trace;
		BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, MoveAction(EBattleDirection::Direita), WaitAction()), 0, Trace);

		TestEqual(TEXT("Pet nunca sai da posição original (casa bloqueada)"), State.Pets[0].Column, static_cast<uint8>(1));
		TestEqual(TEXT("Pet nunca sai da posição original (casa bloqueada)"), State.Pets[0].Row, static_cast<uint8>(1));

		bool bFoundBlockedEvent = false;
		for (const FBattleEvent& Event : Trace)
		{
			if (Event.Type == EBattleEventType::MovimentoBloqueado) { bFoundBlockedEvent = true; }
			TestFalse(TEXT("Nenhum evento novo — só MovimentoBloqueado já existente"), Event.Type == EBattleEventType::Moveu);
		}
		TestTrue(TEXT("MovimentoBloqueado emitido, mesmo vocabulário de sempre"), bFoundBlockedEvent);
	}

	return true;
}

// T3: arena neutra (CellLayout todo None) produz resultado idêntico ao
// comportamento de antes da feature — zero regressão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementNeutralArenaNoRegressionTest,
	"BattleSim.Movement.NeutralArenaProducesNoRegression",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementNeutralArenaNoRegressionTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeMovementPet(1, 0, 1, 1));
	State.Pets.Add(MakeMovementPet(2, 1, 2, 2));

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, MoveAction(EBattleDirection::Direita), WaitAction()), 0, Trace);

	TestEqual(TEXT("Pet se move normalmente em arena neutra"), State.Pets[0].Column, static_cast<uint8>(2));

	return true;
}

// T4: pet parado numa casa de dano perde vida ao longo de vários
// turnos, sem nenhum ataque do oponente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementDamageCellAccumulatesPendingDamageTest,
	"BattleSim.Movement.DamageCellAccumulatesPendingDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementDamageCellAccumulatesPendingDamageTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeMovementPet(1, 0, 1, 1));
	State.CellLayout[State.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Damage);

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, WaitAction(), WaitAction()), 0, Trace);

	TestTrue(TEXT("PendingDamage acumulado por permanecer na casa de dano"), State.Pets[0].PendingDamage > 0);
	TestEqual(TEXT("Nenhum evento próprio de dano de casa — F5 é quem emite"), Trace.Num(), 0);

	return true;
}

// T4: arena sem casa de dano não acumula PendingDamage — zero regressão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattlePhaseMovementNoDamageCellNoPendingDamageTest,
	"BattleSim.Movement.NoDamageCellProducesNoPendingDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattlePhaseMovementNoDamageCellNoPendingDamageTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	State.Pets.Add(MakeMovementPet(1, 0, 1, 1));

	TArray<FBattleEvent> Trace;
	BattlePhases::ApplyMovement(State,
		BattlePhases::DuelSlotActions(State, WaitAction(), WaitAction()), 0, Trace);

	TestEqual(TEXT("Sem casa de dano, PendingDamage continua zero"), State.Pets[0].PendingDamage, 0);

	return true;
}
