// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleGridNavigation.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleAction Mover(EBattleDirection Direction)
	{
		FBattleAction Action;
		Action.Type = EActionType::Mover;
		Action.Direction = Direction;
		return Action;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridNavEdgeBlocksTest,
	"BattleSquare.GridNavigation.EdgeDirectionsWouldLeaveGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridNavEdgeBlocksTest::RunTest(const FString& Parameters)
{
	// Canto superior esquerdo (0,0): cima, esquerda e as diagonais para fora.
	TestTrue(TEXT("de (0,0), Cima sai"), FBattleGridNavigation::WouldLeaveGrid(0, 0, EBattleDirection::Cima));
	TestTrue(TEXT("de (0,0), Esquerda sai"), FBattleGridNavigation::WouldLeaveGrid(0, 0, EBattleDirection::Esquerda));
	TestTrue(TEXT("de (0,0), CimaEsquerda sai"), FBattleGridNavigation::WouldLeaveGrid(0, 0, EBattleDirection::CimaEsquerda));
	TestFalse(TEXT("de (0,0), Direita fica"), FBattleGridNavigation::WouldLeaveGrid(0, 0, EBattleDirection::Direita));
	TestFalse(TEXT("de (0,0), Baixo fica"), FBattleGridNavigation::WouldLeaveGrid(0, 0, EBattleDirection::Baixo));

	// Do centro, nenhuma direção sai.
	for (int32 Dir = 1; Dir <= 8; ++Dir)
	{
		TestFalse(TEXT("do centro nenhuma direção sai"),
			FBattleGridNavigation::WouldLeaveGrid(1, 1, static_cast<EBattleDirection>(Dir)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridNavProjectsQueuedMovesTest,
	"BattleSquare.GridNavigation.ProjectsQueuedMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridNavProjectsQueuedMovesTest::RunTest(const FString& Parameters)
{
	uint8 Column = 0;
	uint8 Row = 0;

	// Duas ações de mover: a projeção precisa ACUMULAR, senão a 2ª direção
	// seria julgada a partir da casa errada.
	FBattleGridNavigation::ProjectCell(1, 1,
		{ Mover(EBattleDirection::Direita), Mover(EBattleDirection::Direita) }, Column, Row);
	TestEqual(TEXT("dois passos à direita saem de (1,1) para a coluna 2 — o segundo é recusado pela borda"),
		static_cast<int32>(Column), 2);
	TestEqual(TEXT("a linha não muda"), static_cast<int32>(Row), 1);

	// Ações que não movem são ignoradas pela projeção.
	FBattleAction Defender;
	Defender.Type = EActionType::Defender;
	FBattleGridNavigation::ProjectCell(1, 1, { Defender }, Column, Row);
	TestEqual(TEXT("Defender não move"), static_cast<int32>(Column), 1);
	TestEqual(TEXT("Defender não move (linha)"), static_cast<int32>(Row), 1);

	return true;
}
