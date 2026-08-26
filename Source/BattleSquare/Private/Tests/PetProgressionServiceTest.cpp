// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetProgressionService.h"
#include "Misc/AutomationTest.h"

namespace
{
	FOwnedPetInstance MakeInstanceWithExperience(int32 Experience)
	{
		FOwnedPetInstance Instance;
		Instance.CatalogId = TEXT("id-teste");
		Instance.Experience = Experience;
		return Instance;
	}
}

// T1: nível derivado corretamente no limiar exato e entre limiares;
// nunca acima do teto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetProgressionServiceLevelDerivedFromExperienceTest,
	"BattleSquare.Meta.PetProgressionService.LevelDerivedFromExperience",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetProgressionServiceLevelDerivedFromExperienceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("0 XP = nível 1"), FPetProgressionService::GetLevel(MakeInstanceWithExperience(0)), 1);
	TestEqual(TEXT("XP entre limiares fica no nível de baixo"), FPetProgressionService::GetLevel(MakeInstanceWithExperience(BattlePetProgressionConstants::ExperiencePerLevel - 1)), 1);
	TestEqual(TEXT("XP exatamente no limiar do nível 2 sobe para nível 2"), FPetProgressionService::GetLevel(MakeInstanceWithExperience(BattlePetProgressionConstants::ExperiencePerLevel)), 2);

	const int32 ExperienceForMaxLevel = FPetProgressionService::ExperienceRequiredForLevel(BattlePetProgressionConstants::MaxLevel);
	TestEqual(TEXT("XP exato do teto atinge o nível máximo"), FPetProgressionService::GetLevel(MakeInstanceWithExperience(ExperienceForMaxLevel)), BattlePetProgressionConstants::MaxLevel);
	TestEqual(TEXT("XP muito além do teto nunca ultrapassa o nível máximo"), FPetProgressionService::GetLevel(MakeInstanceWithExperience(ExperienceForMaxLevel * 100)), BattlePetProgressionConstants::MaxLevel);

	return true;
}

// T2 🧠: múltiplos níveis de uma vez processados corretamente; teto
// nunca ultrapassado; XP bruto continua acumulando além do teto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetProgressionServiceGrantExperienceProcessesMultipleLevelsTest,
	"BattleSquare.Meta.PetProgressionService.GrantExperienceProcessesMultipleLevels",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetProgressionServiceGrantExperienceProcessesMultipleLevelsTest::RunTest(const FString& Parameters)
{
	// 1 nível.
	FOwnedPetInstance OneLevelUp = MakeInstanceWithExperience(0);
	FPetProgressionService::GrantExperience(OneLevelUp, BattlePetProgressionConstants::ExperiencePerLevel);
	TestEqual(TEXT("XP suficiente para 1 nível sobe exatamente 1"), FPetProgressionService::GetLevel(OneLevelUp), 2);

	// 3 níveis de uma vez.
	FOwnedPetInstance ThreeLevelsUp = MakeInstanceWithExperience(0);
	FPetProgressionService::GrantExperience(ThreeLevelsUp, BattlePetProgressionConstants::ExperiencePerLevel * 3);
	TestEqual(TEXT("XP suficiente para 3 níveis de uma vez sobe exatamente 3, não 1"), FPetProgressionService::GetLevel(ThreeLevelsUp), 4);

	// Muito além do teto.
	FOwnedPetInstance BeyondCap = MakeInstanceWithExperience(0);
	FPetProgressionService::GrantExperience(BeyondCap, BattlePetProgressionConstants::ExperiencePerLevel * BattlePetProgressionConstants::MaxLevel * 100);
	TestEqual(TEXT("XP muito além do teto nunca resulta em nível acima do máximo"), FPetProgressionService::GetLevel(BeyondCap), BattlePetProgressionConstants::MaxLevel);
	TestTrue(TEXT("Experience bruto continua acumulando, nunca é truncado"), BeyondCap.Experience > FPetProgressionService::ExperienceRequiredForLevel(BattlePetProgressionConstants::MaxLevel));

	return true;
}

// T3: nível 1 não altera atributos; nível > 1 aumenta proporcionalmente;
// nível máximo produz o bônus máximo esperado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetProgressionServiceApplyLevelBonusScalesAttributesTest,
	"BattleSquare.Meta.PetProgressionService.ApplyLevelBonusScalesAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetProgressionServiceApplyLevelBonusScalesAttributesTest::RunTest(const FString& Parameters)
{
	auto MakeBaseState = []()
	{
		FPetState State;
		State.Attack = 20;
		State.Defense = 10;
		State.Speed = 8;
		State.MaxHealth = 50;
		State.Health = 50;
		return State;
	};

	FPetState LevelOneState = MakeBaseState();
	FPetProgressionService::ApplyLevelBonus(LevelOneState, /*Level=*/1);
	TestEqual(TEXT("Nível 1: Attack inalterado"), LevelOneState.Attack, 20);
	TestEqual(TEXT("Nível 1: Defense inalterado"), LevelOneState.Defense, 10);
	TestEqual(TEXT("Nível 1: Speed inalterado"), LevelOneState.Speed, 8);
	TestEqual(TEXT("Nível 1: MaxHealth inalterado"), LevelOneState.MaxHealth, 50);

	FPetState LevelFiveState = MakeBaseState();
	FPetProgressionService::ApplyLevelBonus(LevelFiveState, /*Level=*/5);
	// BonusPercent = 100 + (5-1)*5 = 120.
	TestEqual(TEXT("Nível 5: Attack com bônus de 20%"), LevelFiveState.Attack, 24);
	TestEqual(TEXT("Nível 5: Defense com bônus de 20%"), LevelFiveState.Defense, 12);
	TestTrue(TEXT("Nível 5: Attack maior que nível 1"), LevelFiveState.Attack > LevelOneState.Attack);

	FPetState MaxLevelState = MakeBaseState();
	FPetProgressionService::ApplyLevelBonus(MaxLevelState, BattlePetProgressionConstants::MaxLevel);
	// BonusPercent = 100 + (10-1)*5 = 145.
	TestEqual(TEXT("Nível máximo: Attack com bônus de 45%"), MaxLevelState.Attack, 29);
	TestEqual(TEXT("Nível máximo: MaxHealth com bônus de 45%"), MaxLevelState.MaxHealth, 72);
	TestEqual(TEXT("Nível máximo: Health acompanha o novo MaxHealth"), MaxLevelState.Health, MaxLevelState.MaxHealth);

	return true;
}
