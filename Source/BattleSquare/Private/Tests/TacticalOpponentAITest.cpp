// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/TacticalOpponentAI.h"
#include "Battle/BattleRandom.h"
#include "Battle/BattleState.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeState(uint8 SelfColumn, uint8 SelfRow, uint8 EnemyColumn, uint8 EnemyRow,
		int32 SelfHealth = 100)
	{
		FBattleState State;

		FPetState Enemy;
		Enemy.PetId = 1;
		Enemy.Side = 0;
		Enemy.Column = EnemyColumn;
		Enemy.Row = EnemyRow;
		Enemy.Health = 100;
		Enemy.MaxHealth = 100;
		State.Pets.Add(Enemy);

		FPetState Self;
		Self.PetId = 2;
		Self.Side = 1;
		Self.Column = SelfColumn;
		Self.Row = SelfRow;
		Self.Health = SelfHealth;
		Self.MaxHealth = 100;
		State.Pets.Add(Self);

		return State;
	}

	bool IsAttack(const FBattleAction& Action)
	{
		return Action.Type == EActionType::Atacar || Action.Type == EActionType::Magia;
	}
}

// Este teste nasceu de uma queixa concreta: o oponente atacava sem olhar onde
// o jogador estava, e ataque em direção vazia era erro garantido.
//
// Em 2026-08-29, DP-golpe-05 tirou da direção o poder de decidir o alvo — o
// golpe acerta o adjacente, e "direção errada" deixou de existir. A queixa
// original não pode mais acontecer, e o teste passou a guardar o que a
// substituiu: o ÍNDICE DO GOLPE precisa ser válido, senão a IA escolheria um
// golpe que o pet não tem.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentNeverAttacksEmptyAirTest,
	"BattleSquare.Battle.TacticalOpponentAI.PicksValidMoveIndex",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentNeverAttacksEmptyAirTest::RunTest(const FString& Parameters)
{
	// Inimigo adjacente à esquerda: todo ataque tem que apontar para lá.
	const FBattleState State = MakeState(/*Self*/2, 1, /*Enemy*/1, 1);

	for (uint64 Seed = 1; Seed <= 60; ++Seed)
	{
		FBattleRandom Random;
		Random.State = Seed;

		const FTurnCommit Commit = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, Random);

		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			const FBattleAction& Acao = Commit.Actions[Slot];
			if (!IsAttack(Acao))
			{
				continue;
			}

			// O byte da direção carrega o índice do golpe (DP-golpe-04): fora
			// da faixa, o resolvedor usaria um golpe que não existe.
			TestTrue(TEXT("Índice de golpe dentro da faixa"),
				static_cast<uint8>(Acao.Direction) < BattleMovesPerPet);
		}
	}

	return true;
}

// Longe do inimigo, atacar é sempre desperdício: não há alcance. A IA tem que
// FECHAR a distância antes, que é a diferença entre ter intenção e sortear.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentClosesDistanceTest,
	"BattleSquare.Battle.TacticalOpponentAI.ClosesDistance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentClosesDistanceTest::RunTest(const FString& Parameters)
{
	// Cantos opostos: distância 2, fora de alcance.
	const FBattleState State = MakeState(/*Self*/2, 2, /*Enemy*/0, 0);

	for (uint64 Seed = 1; Seed <= 60; ++Seed)
	{
		FBattleRandom Random;
		Random.State = Seed;

		const FTurnCommit Commit = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, Random);
		const FBattleAction& First = Commit.Actions[0];

		TestFalse(TEXT("Não ataca de longe"), IsAttack(First));

		if (First.Type == EActionType::Mover)
		{
			TestEqual(TEXT("Move na direção do inimigo"),
				static_cast<uint8>(First.Direction),
				static_cast<uint8>(EBattleDirection::CimaEsquerda));
		}
	}

	return true;
}

// Aguardar é estritamente dominado: não aproxima, não fere, não protege. A IA
// antiga o escolhia em 1 de 6 turnos, e era isso que parecia "não fez nada".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentNeverIdlesTest,
	"BattleSquare.Battle.TacticalOpponentAI.NeverIdles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentNeverIdlesTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakeState(/*Self*/2, 1, /*Enemy*/1, 1);

	for (uint64 Seed = 1; Seed <= 40; ++Seed)
	{
		FBattleRandom Random;
		Random.State = Seed;

		const FTurnCommit Commit = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, Random);
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			TestNotEqual(TEXT("Nunca aguarda à toa"),
				static_cast<uint8>(Commit.Actions[Slot].Type),
				static_cast<uint8>(EActionType::Aguardar));
		}
	}

	return true;
}

// Intenção NÃO pode virar previsibilidade: o commit é às cegas e simultâneo,
// então um oponente que sempre repete a mesma sequência é exploração de cor.
// Ele precisa variar sem deixar de fazer sentido.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentStaysUnpredictableTest,
	"BattleSquare.Battle.TacticalOpponentAI.StaysUnpredictable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentStaysUnpredictableTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakeState(/*Self*/2, 1, /*Enemy*/1, 1);

	TSet<FString> Sequencias;
	for (uint64 Seed = 1; Seed <= 40; ++Seed)
	{
		FBattleRandom Random;
		Random.State = Seed;

		const FTurnCommit Commit = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, Random);

		FString Assinatura;
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			Assinatura += FString::Printf(TEXT("%d:%d|"),
				static_cast<int32>(Commit.Actions[Slot].Type),
				static_cast<int32>(Commit.Actions[Slot].Direction));
		}
		Sequencias.Add(Assinatura);
	}

	TestTrue(TEXT("Varia entre sementes diferentes"), Sequencias.Num() > 3);
	return true;
}

// Determinismo (AD-004): a mesma semente tem que dar o mesmo commit, ou a
// repetição de partida e o replay deixam de valer.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentIsDeterministicTest,
	"BattleSquare.Battle.TacticalOpponentAI.IsDeterministic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentIsDeterministicTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakeState(/*Self*/2, 2, /*Enemy*/0, 1);

	FBattleRandom PrimeiraTirada;
	PrimeiraTirada.State = 12345;
	const FTurnCommit Primeiro = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, PrimeiraTirada);

	FBattleRandom SegundaTirada;
	SegundaTirada.State = 12345;
	const FTurnCommit Segundo = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, SegundaTirada);

	for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
	{
		TestEqual(TEXT("Mesma semente, mesmo tipo"),
			static_cast<uint8>(Primeiro.Actions[Slot].Type),
			static_cast<uint8>(Segundo.Actions[Slot].Type));
		TestEqual(TEXT("Mesma semente, mesma direção"),
			static_cast<uint8>(Primeiro.Actions[Slot].Direction),
			static_cast<uint8>(Segundo.Actions[Slot].Direction));
	}

	return true;
}

// Nunca gerar Mover para fora da grade — o resolvedor emitiria
// MovimentoBloqueado, que agora o jogador LÊ no feed como se a IA tivesse se
// atrapalhado. A regra já existia; o feed tornou a violação visível.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentStaysInsideGridTest,
	"BattleSquare.Battle.TacticalOpponentAI.StaysInsideGrid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentStaysInsideGridTest::RunTest(const FString& Parameters)
{
	// Canto, com o inimigo do outro lado: a tentação de sair da grade é máxima.
	const FBattleState State = MakeState(/*Self*/0, 0, /*Enemy*/2, 2);

	for (uint64 Seed = 1; Seed <= 40; ++Seed)
	{
		FBattleRandom Random;
		Random.State = Seed;

		const FTurnCommit Commit = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, Random);

		int32 Column = 0;
		int32 Row = 0;
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			if (Commit.Actions[Slot].Type != EActionType::Mover)
			{
				continue;
			}

			int8 DeltaColumn = 0;
			int8 DeltaRow = 0;
			GetDirectionDelta(Commit.Actions[Slot].Direction, DeltaColumn, DeltaRow);
			Column += DeltaColumn;
			Row += DeltaRow;

			TestTrue(TEXT("Movimento planejado cai dentro da grade"), State.IsInside(Column, Row));
		}
	}

	return true;
}

// O outro lado da mesma moeda: adjacente, ele ATACA em vez de andar. Sair de
// perto estando ao alcance é jogar pior, e foi isto que fez o teste antigo de
// "o inimigo sai do lugar" reprovar — ele media inércia por movimento, o que
// só funcionava com uma IA que sorteava.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentAttacksInsteadOfWanderingTest,
	"BattleSquare.Battle.TacticalOpponentAI.AttacksInsteadOfWandering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentAttacksInsteadOfWanderingTest::RunTest(const FString& Parameters)
{
	// As mesmas casas iniciais da batalha de verdade.
	const FBattleState State = MakeState(/*Self*/2, 1, /*Enemy*/1, 1);

	int32 Ataques = 0;
	for (uint64 Seed = 1; Seed <= 30; ++Seed)
	{
		FBattleRandom Random;
		Random.State = Seed;

		const FTurnCommit Commit = FTacticalOpponentAI::GenerateCommit(State, /*Side=*/1, Random);
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			TestNotEqual(TEXT("Não se afasta estando ao alcance"),
				static_cast<uint8>(Commit.Actions[Slot].Type),
				static_cast<uint8>(EActionType::Mover));

			if (IsAttack(Commit.Actions[Slot]))
			{
				++Ataques;
			}
		}
	}

	TestTrue(TEXT("Ataca de fato, e não só se protege"), Ataques > 0);
	return true;
}

// A IA só escolhe o que o pet DELA sabe fazer.
//
// Sem isto ela escolheria voar com um pet que não voa, a fila recusaria, e o
// turno viraria um slot perdido — o oponente pareceria travado sem motivo
// visível na tela.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalOpponentRespectsItsOwnSkillsTest,
	"BattleSquare.Battle.TacticalOpponentAI.RespectsItsOwnSkills",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTacticalOpponentRespectsItsOwnSkillsTest::RunTest(const FString& Parameters)
{
	// Ferido e ao alcance: é o estado que faz a IA escolher proteção, que é
	// onde as skills entram.
	const FBattleState State = MakeState(/*Self*/2, 1, /*Enemy*/1, 1, /*SelfHealth=*/10);

	// Um pet que só sabe camuflar, entre as protetoras.
	const TArray<EActionType> SoCamufla = {
		EActionType::Aguardar, EActionType::Mover, EActionType::Atacar,
		EActionType::Magia, EActionType::Camuflar
	};

	bool bUsouCamuflar = false;
	for (uint64 Seed = 1; Seed <= 60; ++Seed)
	{
		FBattleRandom Random;
		Random.State = Seed;

		const FTurnCommit Commit = FTacticalOpponentAI::GenerateCommit(
			State, /*Side=*/1, Random, SoCamufla);

		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			const EActionType Tipo = Commit.Actions[Slot].Type;

			TestTrue(TEXT("Nunca escolhe skill que o pet não tem"),
				SoCamufla.Contains(Tipo) || Tipo == EActionType::Defender);

			TestNotEqual(TEXT("Nunca voa sem saber voar"),
				static_cast<uint8>(Tipo), static_cast<uint8>(EActionType::Voar));
			TestNotEqual(TEXT("Nunca submerge sem saber submergir"),
				static_cast<uint8>(Tipo), static_cast<uint8>(EActionType::Submergir));

			if (Tipo == EActionType::Camuflar)
			{
				bUsouCamuflar = true;
			}
		}
	}

	TestTrue(TEXT("E usa de fato a skill que tem"), bUsouCamuflar);
	return true;
}
