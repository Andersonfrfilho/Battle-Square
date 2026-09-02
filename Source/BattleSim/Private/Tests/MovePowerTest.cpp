// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeMoveDuel(int32 PoderGolpe0, int32 PoderGolpe1)
	{
		FBattleState State;

		FPetState Atacante;
		Atacante.PetId = 1; Atacante.Side = 0;
		Atacante.Column = 1; Atacante.Row = 1;
		Atacante.Health = 500; Atacante.MaxHealth = 500;
		Atacante.Attack = 100; Atacante.Defense = 10; Atacante.Speed = 5;
		Atacante.MovePowers[0] = PoderGolpe0;
		Atacante.MovePowers[1] = PoderGolpe1;

		FPetState Alvo = Atacante;
		Alvo.PetId = 2; Alvo.Side = 1; Alvo.Column = 2;
		Alvo.MovePowers[0] = 0;
		Alvo.MovePowers[1] = 0;

		State.Pets.Add(Atacante);
		State.Pets.Add(Alvo);
		return State;
	}

	int32 DanoDoGolpe(int32 PoderGolpe0, int32 PoderGolpe1, uint8 IndiceUsado)
	{
		FBattleState State = MakeMoveDuel(PoderGolpe0, PoderGolpe1);
		TArray<FBattleEvent> Trace;

		FBattleAction Aguardar;
		BattlePhases::ApplyCombat(State,
		BattlePhases::DuelSlotActions(State, MakeMoveAction(EActionType::Atacar, IndiceUsado), Aguardar), 0, Trace);

		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == 1) { return Pet.PendingDamage; }
		}
		return 0;
	}
}

// Golpe mais forte dói mais. Sem isto, escolher entre quatro golpes seria
// escolher entre quatro nomes — a decisão que a fatia 2 abriu não teria
// consequência nenhuma.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMovePowerChangesDamageTest,
	"BattleSim.Moves.Power.ChangesDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMovePowerChangesDamageTest::RunTest(const FString& Parameters)
{
	const int32 Fraco = DanoDoGolpe(/*Golpe0=*/50, /*Golpe1=*/200, /*Usado=*/0);
	const int32 Forte = DanoDoGolpe(/*Golpe0=*/50, /*Golpe1=*/200, /*Usado=*/1);

	TestTrue(TEXT("Os dois causam dano"), Fraco > 0 && Forte > 0);
	TestTrue(TEXT("O golpe de poder 200 dói mais que o de 50"), Forte > Fraco);

	return true;
}

// Pet SEM golpe cadastrado continua lutando como antes.
//
// Poder 0 significa "não tem golpe", não "golpe que não machuca": tratá-lo
// como poder faria todo pet legado bater sem dano nenhum, e o espelho está
// cheio deles enquanto a migração não termina.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetWithoutMovesStillFightsTest,
	"BattleSim.Moves.Power.PetWithoutMovesStillFights",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetWithoutMovesStillFightsTest::RunTest(const FString& Parameters)
{
	const int32 SemGolpe = DanoDoGolpe(/*Golpe0=*/0, /*Golpe1=*/0, /*Usado=*/0);

	TestTrue(TEXT("Pet sem golpe ainda fere"), SemGolpe > 0);
	return true;
}

// O poder entra no HASH do estado: ele decide dano, e estado que decide
// resultado precisa estar na assinatura, senão duas partidas com golpes
// diferentes teriam a mesma e a divergência passaria calada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMovePowerIsInTheHashTest,
	"BattleSim.Moves.Power.IsInTheHash",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMovePowerIsInTheHashTest::RunTest(const FString& Parameters)
{
	const FBattleState Um = MakeMoveDuel(50, 200);
	const FBattleState Outro = MakeMoveDuel(60, 200);

	TestNotEqual(TEXT("Poderes diferentes, hashes diferentes"),
		Um.ComputeHash(), Outro.ComputeHash());

	const FBattleState Igual = MakeMoveDuel(50, 200);
	TestEqual(TEXT("Poderes iguais, hashes iguais"), Um.ComputeHash(), Igual.ComputeHash());

	return true;
}
