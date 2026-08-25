// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleRandom.h"
#include "Misc/AutomationTest.h"

// T2 (tasks.md): mesma seed produz a mesma sequência entre execuções —
// é a base do determinismo exigido por BTL-16.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRandomDeterminismTest,
	"BattleSim.Random.SameSeedProducesSameSequence",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRandomDeterminismTest::RunTest(const FString& Parameters)
{
	FBattleRandom RandomA;
	RandomA.State = 12345ULL;

	FBattleRandom RandomB;
	RandomB.State = 12345ULL;

	bool bAllMatch = true;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		const uint32 ValueA = RandomA.NextUInt32();
		const uint32 ValueB = RandomB.NextUInt32();
		if (ValueA != ValueB)
		{
			AddError(FString::Printf(TEXT("Divergência no índice %d: %u != %u"), Index, ValueA, ValueB));
			bAllMatch = false;
		}
	}

	return bAllMatch;
}

// T2: NextRange é inclusivo nos dois extremos e sem viés de módulo
// perceptível — verificado por distribuição em amostra grande.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRandomRangeDistributionTest,
	"BattleSim.Random.RangeIsUnbiasedAndInclusive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRandomRangeDistributionTest::RunTest(const FString& Parameters)
{
	FBattleRandom Random;
	Random.State = 42ULL;

	constexpr int32 Min = 0;
	constexpr int32 Max = 5;
	constexpr int32 BucketCount = Max - Min + 1;
	constexpr int32 SampleCount = 100000;

	int32 Buckets[BucketCount] = {};
	bool bAllInRange = true;

	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const int32 Value = Random.NextRange(Min, Max);
		if (Value < Min || Value > Max)
		{
			AddError(FString::Printf(TEXT("Valor %d fora do intervalo [%d, %d]"), Value, Min, Max));
			bAllInRange = false;
			continue;
		}
		Buckets[Value - Min]++;
	}

	const double Expected = static_cast<double>(SampleCount) / BucketCount;
	for (int32 Bucket = 0; Bucket < BucketCount; ++Bucket)
	{
		const double Deviation = FMath::Abs(Buckets[Bucket] - Expected) / Expected;
		TestTrue(FString::Printf(TEXT("Bucket %d desvia menos de 2%% do uniforme (real: %.4f)"), Bucket, Deviation), Deviation < 0.02);
	}

	return bAllInRange;
}
