// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldCellKey.h"
#include "Misc/AutomationTest.h"

/**
 * MV3 — a identidade da árvore é a posição quantizada, e ela é ESTÁVEL: a mesma
 * posição dá sempre a mesma célula, dois pontos da mesma casa colidem, casas
 * diferentes não.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldCellKeyStableTest,
	"BattleSquare.World.ChaveDeCelula.Estavel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldCellKeyStableTest::RunTest(const FString&)
{
	using namespace WorldCellKey;

	const float Quantum = 100.0f;

	// DETERMINÍSTICO: a mesma posição dá sempre a mesma célula — é o que faz a
	// marca reencontrar a árvore depois de replantada.
	const FVector2D Ponto(1234.0f, -567.0f);
	TestEqual(TEXT("mesma posicao, mesma celula"),
		CellKeyOf(Ponto, Quantum), CellKeyOf(Ponto, Quantum));

	// Dois pontos da MESMA casa colidem (a árvore replantada um tico ao lado
	// ainda é a mesma marca).
	TestEqual(TEXT("pontos da mesma casa colidem"),
		CellKeyOf(FVector2D(1210.0f, 30.0f), Quantum),
		CellKeyOf(FVector2D(1290.0f, 70.0f), Quantum));

	// Casas diferentes NÃO colidem.
	TestNotEqual(TEXT("casas diferentes nao colidem"),
		CellKeyOf(FVector2D(10.0f, 10.0f), Quantum),
		CellKeyOf(FVector2D(310.0f, 10.0f), Quantum));

	// A chave do pedaço é estável.
	TestEqual(TEXT("chave de pedaco"), ChunkKeyOf(FIntPoint(3, -4)), FString(TEXT("3:-4")));

	// Quantum degenerado não quebra.
	TestFalse(TEXT("quantum zero nao produz chave vazia"),
		CellKeyOf(Ponto, 0.0f).IsEmpty());

	return true;
}
