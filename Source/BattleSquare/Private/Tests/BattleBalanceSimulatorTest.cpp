// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/BattleBalanceSimulator.h"
#include "Data/BattleDataTranslator.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakeTemplate(uint8 PetId, uint8 Side, int32 Attack, int32 Defense, int32 MaxHealth)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Column = (Side == 0) ? 1 : 2;
		Pet.Row = 1;
		Pet.Attack = Attack;
		Pet.Defense = Defense;
		Pet.Speed = 5;
		Pet.MaxHealth = MaxHealth;
		Pet.Health = MaxHealth;
		return Pet;
	}
}

// T4: duas execuções com a mesma seed produzem resultado idêntico;
// seeds diferentes produzem resultados que variam.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleBalanceSimulatorDeterministicBySeedTest,
	"BattleSquare.Balance.RunBatchSimulation.DeterministicBySeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleBalanceSimulatorDeterministicBySeedTest::RunTest(const FString& Parameters)
{
	const FPetState Left = MakeTemplate(1, 0, 15, 5, 50);
	const FPetState Right = MakeTemplate(2, 1, 10, 5, 50);

	const FBattleBalanceResult ResultA = FBattleBalanceSimulator::RunBatchSimulation(Left, Right, /*NumSimulations=*/20, /*BaseSeed=*/42);
	const FBattleBalanceResult ResultB = FBattleBalanceSimulator::RunBatchSimulation(Left, Right, /*NumSimulations=*/20, /*BaseSeed=*/42);

	TestEqual(TEXT("Mesma seed: LeftWins idêntico"), ResultA.LeftWins, ResultB.LeftWins);
	TestEqual(TEXT("Mesma seed: RightWins idêntico"), ResultA.RightWins, ResultB.RightWins);
	TestEqual(TEXT("Mesma seed: Draws idêntico"), ResultA.Draws, ResultB.Draws);
	TestTrue(TEXT("Mesma seed: AverageTurns idêntico"), FMath::IsNearlyEqual(ResultA.AverageTurns, ResultB.AverageTurns, 0.0001));
	TestTrue(TEXT("Mesma seed: AverageDamagePerTurn idêntico"), FMath::IsNearlyEqual(ResultA.AverageDamagePerTurn, ResultB.AverageDamagePerTurn, 0.0001));
	TestEqual(TEXT("Todo combate termina em vitória ou empate"), ResultA.LeftWins + ResultA.RightWins + ResultA.Draws, 20);

	const FBattleBalanceResult ResultDifferentSeed = FBattleBalanceSimulator::RunBatchSimulation(Left, Right, 20, /*BaseSeed=*/999);
	const bool bSomethingDiffers = (ResultA.LeftWins != ResultDifferentSeed.LeftWins)
		|| !FMath::IsNearlyEqual(ResultA.AverageTurns, ResultDifferentSeed.AverageTurns, 0.0001)
		|| !FMath::IsNearlyEqual(ResultA.AverageDamagePerTurn, ResultDifferentSeed.AverageDamagePerTurn, 0.0001);
	TestTrue(TEXT("Seed diferente produz algum resultado diferente (não travado no mesmo valor)"), bSomethingDiffers);

	return true;
}

// T5: composição super efetiva vence visivelmente mais que composição
// neutra, com atributos base IDÊNTICOS — prova de que o tipo importa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleBalanceSimulatorSuperEffectiveWinsMoreTest,
	"BattleSquare.Balance.RunBatchSimulation.ReportsAggregateStats",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleBalanceSimulatorSuperEffectiveWinsMoreTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Table;
	Table.SetPercent(TEXT("Fogo"), TEXT("Planta"), 150);

	FLoadedPetRecord FireSource;
	FireSource.Type = TEXT("Fogo"); FireSource.Name = TEXT("FireFixture");
	FireSource.Attack = 20; FireSource.Defense = 10; FireSource.Speed = 5; FireSource.MaxHealth = 50;

	FLoadedPetRecord PlantSource;
	PlantSource.Type = TEXT("Planta"); PlantSource.Name = TEXT("PlantFixture");
	PlantSource.Attack = 20; PlantSource.Defense = 10; PlantSource.Speed = 5; PlantSource.MaxHealth = 50;

	FPetState FireEffective, PlantAsDefender;
	FPetPresentationInfo FirePresentation, PlantPresentation;
	FBattleDataTranslator::TranslateMatchup(FireSource, PlantSource, Table, 1, 2, FireEffective, FirePresentation, PlantAsDefender, PlantPresentation);

	// Composição de controle: os mesmos atributos base, mas SEM
	// vantagem de tipo (tabela vazia) — mede o "chão" da moeda-honesta.
	FTypeEffectivenessTable EmptyTable;
	FPetState FireNeutral, PlantNeutral;
	FPetPresentationInfo FireNeutralPresentation, PlantNeutralPresentation;
	FBattleDataTranslator::TranslateMatchup(FireSource, PlantSource, EmptyTable, 1, 2, FireNeutral, FireNeutralPresentation, PlantNeutral, PlantNeutralPresentation);

	const int32 NumSimulations = 200;
	const FBattleBalanceResult EffectiveResult = FBattleBalanceSimulator::RunBatchSimulation(FireEffective, PlantAsDefender, NumSimulations, /*BaseSeed=*/1000);
	const FBattleBalanceResult NeutralResult = FBattleBalanceSimulator::RunBatchSimulation(FireNeutral, PlantNeutral, NumSimulations, /*BaseSeed=*/1000);

	const double EffectiveWinRate = static_cast<double>(EffectiveResult.LeftWins) / static_cast<double>(NumSimulations);
	const double NeutralWinRate = static_cast<double>(NeutralResult.LeftWins) / static_cast<double>(NumSimulations);

	AddInfo(FString::Printf(TEXT("Relatório — Fogo super efetivo: %.1f%% vitórias, %.1f turnos médios, %.1f dano médio/turno"),
		EffectiveWinRate * 100.0, EffectiveResult.AverageTurns, EffectiveResult.AverageDamagePerTurn));
	AddInfo(FString::Printf(TEXT("Relatório — Fogo neutro (controle): %.1f%% vitórias, %.1f turnos médios, %.1f dano médio/turno"),
		NeutralWinRate * 100.0, NeutralResult.AverageTurns, NeutralResult.AverageDamagePerTurn));

	TestTrue(TEXT("Composição super efetiva vence visivelmente mais que a composição neutra de controle"), EffectiveWinRate > NeutralWinRate + 0.05);

	return true;
}
