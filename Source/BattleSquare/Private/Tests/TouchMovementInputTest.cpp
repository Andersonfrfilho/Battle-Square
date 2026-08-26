// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TouchMovementInput.h"
#include "World/WorldTraversalMotion.h"
#include "Misc/AutomationTest.h"

namespace
{
	FVector2D ComputeAxis(const FVector2D& Origin, const FVector2D& Current)
	{
		FTouchMovementParams Params;
		Params.TouchOriginScreen = Origin;
		Params.TouchCurrentScreen = Current;
		return FTouchMovementInput::ComputeMovementAxis(Params);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTouchMovementDragUpIsForwardTest,
	"BattleSquare.World.TouchMovementInput.DragUpOnScreenIsForward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTouchMovementDragUpIsForwardTest::RunTest(const FString& Parameters)
{
	// Em tela, "para cima" é Y decrescente.
	const FVector2D Axis = ComputeAxis(FVector2D(200.0, 400.0), FVector2D(200.0, 280.0));

	TestTrue(TEXT("arrastar para cima na tela é andar para frente (X positivo)"), Axis.X > 0.0);
	TestTrue(TEXT("sem componente lateral"), FMath::IsNearlyZero(Axis.Y, UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTouchMovementDeadZoneTest,
	"BattleSquare.World.TouchMovementInput.InsideDeadZoneProducesNoDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTouchMovementDeadZoneTest::RunTest(const FString& Parameters)
{
	const FVector2D Origin(200.0, 400.0);
	const FVector2D BarelyMoved(Origin.X + TouchMovement::TouchDeadZoneScreenUnits - 1.0, Origin.Y);

	TestTrue(TEXT("dedo parado dentro da zona morta não gera movimento"),
		ComputeAxis(Origin, BarelyMoved).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTouchMovementSaturatesTest,
	"BattleSquare.World.TouchMovementInput.BeyondMaxRadiusSaturatesAtOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTouchMovementSaturatesTest::RunTest(const FString& Parameters)
{
	const FVector2D Origin(200.0, 400.0);
	const FVector2D FarAway(Origin.X, Origin.Y - TouchMovement::TouchMaxRadiusScreenUnits * 5.0);

	// Sem saturar, quanto mais longe o dedo, mais rápido o personagem.
	TestTrue(TEXT("além do raio máximo o comprimento é exatamente 1"),
		FMath::IsNearlyEqual(ComputeAxis(Origin, FarAway).Size(), 1.0, UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTouchMovementPartialDragScalesTest,
	"BattleSquare.World.TouchMovementInput.PartialDragScalesProportionally",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTouchMovementPartialDragScalesTest::RunTest(const FString& Parameters)
{
	const FVector2D Origin(200.0, 400.0);
	const double HalfRadius = TouchMovement::TouchMaxRadiusScreenUnits * 0.5;
	const FVector2D HalfWay(Origin.X, Origin.Y - HalfRadius);

	TestTrue(TEXT("meio caminho até o raio máximo dá meia intensidade"),
		FMath::IsNearlyEqual(ComputeAxis(Origin, HalfWay).Size(), 0.5, UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTouchMovementDiagonalNormalizedTest,
	"BattleSquare.World.TouchMovementInput.DiagonalIsNormalized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTouchMovementDiagonalNormalizedTest::RunTest(const FString& Parameters)
{
	const FVector2D Origin(200.0, 400.0);
	const double Reach = TouchMovement::TouchMaxRadiusScreenUnits * 2.0;
	const FVector2D Diagonal(Origin.X + Reach, Origin.Y - Reach);

	TestTrue(TEXT("a diagonal saturada tem comprimento 1, não √2"),
		FMath::IsNearlyEqual(ComputeAxis(Origin, Diagonal).Size(), 1.0, UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTouchMovementMatchesKeyboardPathTest,
	"BattleSquare.World.TouchMovementInput.TouchAxisProducesSameDirectionAsKeyboard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTouchMovementMatchesKeyboardPathTest::RunTest(const FString& Parameters)
{
	const FVector2D Origin(200.0, 400.0);
	const FVector2D FullyForward(Origin.X, Origin.Y - TouchMovement::TouchMaxRadiusScreenUnits);

	FWorldTraversalMotionParams TouchDriven;
	TouchDriven.MovementInput = ComputeAxis(Origin, FullyForward);
	TouchDriven.CameraRotation = FRotator(0.0, 45.0, 0.0);

	FWorldTraversalMotionParams KeyboardDriven;
	KeyboardDriven.MovementInput = FVector2D(1.0, 0.0);
	KeyboardDriven.CameraRotation = FRotator(0.0, 45.0, 0.0);

	// DP-mobile-02: toque não é um segundo caminho de movimento. O eixo entra
    // na MESMA função pura, e a direção resultante tem de ser a mesma.
	TestTrue(TEXT("toque e teclado produzem a mesma direção no mundo"),
		FWorldTraversalMotion::ComputeMoveDirection(TouchDriven)
			.Equals(FWorldTraversalMotion::ComputeMoveDirection(KeyboardDriven), UE_KINDA_SMALL_NUMBER));
	return true;
}
