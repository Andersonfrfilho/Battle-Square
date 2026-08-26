// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldTraversalMotion.h"
#include "Misc/AutomationTest.h"

namespace
{
	FVector ComputeDirection(const FVector2D& Input, const FRotator& CameraRotation)
	{
		FWorldTraversalMotionParams Params;
		Params.MovementInput = Input;
		Params.CameraRotation = CameraRotation;
		return FWorldTraversalMotion::ComputeMoveDirection(Params);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldTraversalMotionForwardAtZeroYawTest,
	"BattleSquare.World.WorldTraversalMotion.ForwardAtZeroYawIsPositiveX",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTraversalMotionForwardAtZeroYawTest::RunTest(const FString& Parameters)
{
	const FVector Direction = ComputeDirection(FVector2D(1.0, 0.0), FRotator::ZeroRotator);

	TestTrue(TEXT("frente com câmera a 0° aponta para +X"),
		Direction.Equals(FVector(1.0, 0.0, 0.0), UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldTraversalMotionForwardAtNinetyYawTest,
	"BattleSquare.World.WorldTraversalMotion.ForwardAtNinetyYawIsPositiveY",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTraversalMotionForwardAtNinetyYawTest::RunTest(const FString& Parameters)
{
	const FVector Direction = ComputeDirection(FVector2D(1.0, 0.0), FRotator(0.0, 90.0, 0.0));

	TestTrue(TEXT("frente com câmera a 90° aponta para +Y — o movimento é relativo à câmera"),
		Direction.Equals(FVector(0.0, 1.0, 0.0), UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldTraversalMotionStrafeAtZeroYawTest,
	"BattleSquare.World.WorldTraversalMotion.StrafeAtZeroYawIsPositiveY",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTraversalMotionStrafeAtZeroYawTest::RunTest(const FString& Parameters)
{
	const FVector Direction = ComputeDirection(FVector2D(0.0, 1.0), FRotator::ZeroRotator);

	TestTrue(TEXT("lado com câmera a 0° aponta para +Y"),
		Direction.Equals(FVector(0.0, 1.0, 0.0), UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldTraversalMotionDiagonalIsNormalizedTest,
	"BattleSquare.World.WorldTraversalMotion.DiagonalIsNormalized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTraversalMotionDiagonalIsNormalizedTest::RunTest(const FString& Parameters)
{
	const FVector Direction = ComputeDirection(FVector2D(1.0, 1.0), FRotator::ZeroRotator);

	// Sem normalizar, a diagonal teria comprimento √2 e andar na diagonal
	// seria ~41% mais rápido que andar reto.
	TestTrue(TEXT("a diagonal tem comprimento 1, não √2"),
		FMath::IsNearlyEqual(Direction.Size(), 1.0, UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldTraversalMotionZeroInputIsZeroTest,
	"BattleSquare.World.WorldTraversalMotion.ZeroInputProducesNoDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTraversalMotionZeroInputIsZeroTest::RunTest(const FString& Parameters)
{
	const FVector Direction = ComputeDirection(FVector2D::ZeroVector, FRotator(0.0, 37.0, 0.0));

	TestTrue(TEXT("entrada nula devolve vetor nulo — sem deriva"), Direction.IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldTraversalMotionIgnoresPitchAndRollTest,
	"BattleSquare.World.WorldTraversalMotion.IgnoresCameraPitchAndRoll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTraversalMotionIgnoresPitchAndRollTest::RunTest(const FString& Parameters)
{
	const FVector Flat = ComputeDirection(FVector2D(1.0, 0.0), FRotator(0.0, 45.0, 0.0));
	const FVector Tilted = ComputeDirection(FVector2D(1.0, 0.0), FRotator(-60.0, 45.0, 20.0));

	// Com o pitch entrando na conta, olhar para baixo empurraria o
	// personagem para dentro do chão.
	TestTrue(TEXT("pitch e roll da câmera não mudam a direção de movimento"),
		Flat.Equals(Tilted, UE_KINDA_SMALL_NUMBER));
	TestTrue(TEXT("a direção continua horizontal"), FMath::IsNearlyZero(Tilted.Z, UE_KINDA_SMALL_NUMBER));
	return true;
}
