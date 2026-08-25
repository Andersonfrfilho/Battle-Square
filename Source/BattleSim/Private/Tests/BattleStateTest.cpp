// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakeTestPet(uint8 PetId, int32 Health)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = PetId % 2;
		Pet.Column = 1;
		Pet.Row = 1;
		Pet.Health = Health;
		Pet.MaxHealth = 100;
		Pet.Attack = 10;
		Pet.Defense = 5;
		Pet.Speed = 8;
		return Pet;
	}
}

// T3: dois estados com os mesmos dados produzem o mesmo hash (base de BTL-16).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleStateHashDeterminismTest,
	"BattleSim.State.HashIsDeterministic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleStateHashDeterminismTest::RunTest(const FString& Parameters)
{
	FBattleState StateA;
	StateA.Pets.Add(MakeTestPet(1, 50));
	StateA.Pets.Add(MakeTestPet(2, 80));
	StateA.TurnNumber = 3;
	StateA.Random.State = 999ULL;

	FBattleState StateB;
	StateB.Pets.Add(MakeTestPet(1, 50));
	StateB.Pets.Add(MakeTestPet(2, 80));
	StateB.TurnNumber = 3;
	StateB.Random.State = 999ULL;

	TestEqual(TEXT("Hashes idênticos para estados idênticos"), StateA.ComputeHash(), StateB.ComputeHash());
	return true;
}

// T3: ordem de inserção em Pets NÃO pode afetar o hash — é o requisito
// que sustenta BTL-17 (desempate por chave estável, nunca por contêiner).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleStateHashOrderIndependenceTest,
	"BattleSim.State.HashIsOrderIndependent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleStateHashOrderIndependenceTest::RunTest(const FString& Parameters)
{
	FBattleState StateForward;
	StateForward.Pets.Add(MakeTestPet(1, 50));
	StateForward.Pets.Add(MakeTestPet(2, 80));
	StateForward.Pets.Add(MakeTestPet(3, 20));

	FBattleState StateReversed;
	StateReversed.Pets.Add(MakeTestPet(3, 20));
	StateReversed.Pets.Add(MakeTestPet(2, 80));
	StateReversed.Pets.Add(MakeTestPet(1, 50));

	TestEqual(TEXT("Hash não muda com a ordem de inserção dos pets"), StateForward.ComputeHash(), StateReversed.ComputeHash());
	return true;
}

// T3: um único campo diferente (1 de HP) precisa produzir hash diferente —
// prova de que o hash está de fato lendo o estado, não retornando constante.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleStateHashSensitivityTest,
	"BattleSim.State.HashDetectsSingleFieldChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleStateHashSensitivityTest::RunTest(const FString& Parameters)
{
	FBattleState StateA;
	StateA.Pets.Add(MakeTestPet(1, 50));

	FBattleState StateB;
	StateB.Pets.Add(MakeTestPet(1, 49)); // 1 HP de diferença

	TestNotEqual(TEXT("1 HP de diferença muda o hash"), StateA.ComputeHash(), StateB.ComputeHash());
	return true;
}

// T3: MaxHealth separado de Health — resolve o "HP: X/X" do protótipo antigo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetStateMaxHealthTest,
	"BattleSim.State.MaxHealthIsIndependentOfHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetStateMaxHealthTest::RunTest(const FString& Parameters)
{
	FPetState Pet = MakeTestPet(1, 30);
	Pet.MaxHealth = 100;

	TestEqual(TEXT("Health reflete o dano recebido"), Pet.Health, 30);
	TestEqual(TEXT("MaxHealth permanece o teto, independente do dano"), Pet.MaxHealth, 100);
	TestTrue(TEXT("Pet com Health > 0 está vivo"), Pet.IsAlive());

	Pet.Health = 0;
	TestFalse(TEXT("Pet com Health == 0 não está vivo"), Pet.IsAlive());

	return true;
}
