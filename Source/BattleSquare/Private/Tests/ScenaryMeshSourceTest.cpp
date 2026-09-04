// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ScenaryPalette.h"
#include "Misc/AutomationTest.h"

/**
 * a-malha-vem-de-fora MV2/MV3 — a malha vem de UM lugar: ScenaryPalette da o
 * caminho de cada primitiva, e ele aponta para /Engine/BasicShapes (invariante
 * 20: dado, nao estilo; verde ainda em primitiva da engine).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenaryMeshSourceTest,
	"BattleSquare.Environment.MalhaDeFora.CaminhoVemDaFonteUnica",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenaryMeshSourceTest::RunTest(const FString&)
{
	using namespace ScenaryPalette;

	// Cada primitiva tem caminho, e ele aponta para /Engine/BasicShapes/*.
	for (EScenaryPrimitive P : { EScenaryPrimitive::Cube, EScenaryPrimitive::Cylinder,
			EScenaryPrimitive::Sphere, EScenaryPrimitive::Cone })
	{
		const FString Caminho = PrimitiveMeshPath(P);
		TestTrue(TEXT("o caminho nao e vazio"), !Caminho.IsEmpty());
		TestTrue(TEXT("aponta para /Engine/BasicShapes/ (invariante 20)"),
			Caminho.StartsWith(TEXT("/Engine/BasicShapes/")));
	}

	// Primitivas diferentes, caminhos diferentes — nao colapsam.
	TestNotEqual(TEXT("cubo e esfera sao caminhos distintos"),
		FString(PrimitiveMeshPath(EScenaryPrimitive::Cube)),
		FString(PrimitiveMeshPath(EScenaryPrimitive::Sphere)));

	return true;
}
