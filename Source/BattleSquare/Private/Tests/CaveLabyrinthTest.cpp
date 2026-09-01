// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/CaveLabyrinth.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr uint32 SementeDoLabirinto = 20260901u;

	/** Uma caverna grande, do tamanho que o usuário pediu. */
	constexpr int32 ColunasDaCavernaGrande = 11;
	constexpr int32 LinhasDaCavernaGrande = 11;

	/** Quantas aberturas existem na BORDA — a entrada deveria ser a única. */
	int32 AberturasNaBordaDaCaverna(const CaveLabyrinth::FCaveGrid& Planta)
	{
		using namespace CaveLabyrinth;

		int32 Aberturas = 0;
		for (int32 Coluna = 0; Coluna < Planta.Columns; ++Coluna)
		{
			if (!Planta.HasWall(Coluna, 0, WallSouth)) { ++Aberturas; }
			if (!Planta.HasWall(Coluna, Planta.Rows - 1, WallNorth)) { ++Aberturas; }
		}
		for (int32 Linha = 0; Linha < Planta.Rows; ++Linha)
		{
			if (!Planta.HasWall(0, Linha, WallWest)) { ++Aberturas; }
			if (!Planta.HasWall(Planta.Columns - 1, Linha, WallEast)) { ++Aberturas; }
		}

		return Aberturas;
	}

	/** As paredes de duas plantas são as mesmas? */
	bool PlantasDaCavernaIguais(const CaveLabyrinth::FCaveGrid& Uma,
		const CaveLabyrinth::FCaveGrid& Outra)
	{
		return Uma.Columns == Outra.Columns
			&& Uma.Rows == Outra.Rows
			&& Uma.EntranceColumn == Outra.EntranceColumn
			&& Uma.Walls == Outra.Walls;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveLabyrinthIsSpanningTreeTest,
	"BattleSquare.CaveLabyrinth.IsSpanningTree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveLabyrinthIsSpanningTreeTest::RunTest(const FString& Parameters)
{
	const CaveLabyrinth::FCaveGrid Planta = CaveLabyrinth::Carve(
		ColunasDaCavernaGrande, LinhasDaCavernaGrande, SementeDoLabirinto);

	TestTrue(TEXT("A planta é válida"), Planta.IsValid());

	// Árvore geradora: exatamente uma passagem a menos que o número de casas.
	// Uma a mais é ciclo (atalho que o labirinto não deveria ter); uma a menos
	// é sala isolada.
	TestEqual(TEXT("Passagens internas = casas - 1"),
		CaveLabyrinth::OpenPassageCount(Planta), Planta.CellCount() - 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveLabyrinthEveryCellReachableTest,
	"BattleSquare.CaveLabyrinth.EveryCellReachable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveLabyrinthEveryCellReachableTest::RunTest(const FString& Parameters)
{
	// Cinco cavernas de tamanhos diferentes, porque o defeito de conectividade
	// costuma aparecer numa forma só.
	const int32 Tamanhos[5][2] = { {5, 5}, {7, 4}, {4, 9}, {11, 11}, {13, 8} };

	for (int32 Qual = 0; Qual < 5; ++Qual)
	{
		const int32 Colunas = Tamanhos[Qual][0];
		const int32 Linhas = Tamanhos[Qual][1];
		const CaveLabyrinth::FCaveGrid Planta =
			CaveLabyrinth::Carve(Colunas, Linhas, SementeDoLabirinto + Qual);

		for (int32 Linha = 0; Linha < Linhas; ++Linha)
		{
			for (int32 Coluna = 0; Coluna < Colunas; ++Coluna)
			{
				TestTrue(
					FString::Printf(TEXT("Caverna %dx%d: dá para chegar em (%d,%d)"),
						Colunas, Linhas, Coluna, Linha),
					CaveLabyrinth::HasPath(Planta, Planta.EntranceColumn, 0, Coluna, Linha));
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveLabyrinthHasDeadEndsTest,
	"BattleSquare.CaveLabyrinth.HasDeadEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveLabyrinthHasDeadEndsTest::RunTest(const FString& Parameters)
{
	const CaveLabyrinth::FCaveGrid Planta = CaveLabyrinth::Carve(
		ColunasDaCavernaGrande, LinhasDaCavernaGrande, SementeDoLabirinto);

	// Sem beco não é labirinto, é corredor com curvas: quem entra chega ao fim
	// sem nunca ter escolhido nada.
	const int32 Becos = CaveLabyrinth::DeadEndCount(Planta);
	TestTrue(FString::Printf(TEXT("Uma caverna 11x11 tem becos (achei %d)"), Becos),
		Becos >= 8);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveLabyrinthEntranceIsTheOnlyOpeningTest,
	"BattleSquare.CaveLabyrinth.EntranceIsTheOnlyOpening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveLabyrinthEntranceIsTheOnlyOpeningTest::RunTest(const FString& Parameters)
{
	// "A entrada é a única abertura" deixou de ser verdade DE PROPÓSITO.
	//
	// Caverna com uma saída só é beco: entra-se, anda-se o labirinto inteiro e
	// volta-se pelo mesmo caminho. Com duas ela vira PASSAGEM — e é o que dá
	// sentido às galerias que ligam uma caverna à outra.
	//
	// O que continua tendo de valer, e é o que este teste passa a afirmar: as
	// bocas são POUCAS e ficam em paredes DIFERENTES. Muitas bocas fazem um
	// alpendre; duas na mesma parede leem de fora como uma entrada larga.
	const CaveLabyrinth::FCaveGrid Planta = CaveLabyrinth::Carve(9, 7, 4242u);
	if (!TestTrue(TEXT("a planta e valida"), Planta.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("ha mais de uma boca"), Planta.ExtraMouths.Num() >= 1);
	TestTrue(TEXT("e poucas — caverna nao e alpendre"), Planta.ExtraMouths.Num() <= 3);

	for (const CaveLabyrinth::FCaveGrid::FMouth& Boca : Planta.ExtraMouths)
	{
		TestTrue(TEXT("a boca extra nao fica na parede da entrada"), Boca.Edge != 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveLabyrinthHasDepthTest,
	"BattleSquare.CaveLabyrinth.HasDepth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveLabyrinthHasDepthTest::RunTest(const FString& Parameters)
{
	const CaveLabyrinth::FCaveGrid Planta = CaveLabyrinth::Carve(
		ColunasDaCavernaGrande, LinhasDaCavernaGrande, SementeDoLabirinto);

	int32 FundoColuna = -1;
	int32 FundoLinha = -1;
	const int32 Passos = CaveLabyrinth::DeepestFrom(
		Planta, Planta.EntranceColumn, 0, FundoColuna, FundoLinha);

	// O fundo precisa estar mais longe do que atravessar a caverna em linha
	// reta — senão a "caverna grande" é uma sala com paredes decorativas.
	TestTrue(FString::Printf(TEXT("O fundo está a %d passos, além dos %d de travessia"),
		Passos, ColunasDaCavernaGrande), Passos > ColunasDaCavernaGrande);

	TestTrue(TEXT("O fundo é uma casa da caverna"),
		Planta.Contains(FundoColuna, FundoLinha));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveLabyrinthIsDeterministicTest,
	"BattleSquare.CaveLabyrinth.IsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveLabyrinthIsDeterministicTest::RunTest(const FString& Parameters)
{
	const CaveLabyrinth::FCaveGrid Uma = CaveLabyrinth::Carve(9, 9, SementeDoLabirinto);
	const CaveLabyrinth::FCaveGrid Outra = CaveLabyrinth::Carve(9, 9, SementeDoLabirinto);
	const CaveLabyrinth::FCaveGrid Diferente = CaveLabyrinth::Carve(9, 9, SementeDoLabirinto + 1);

	TestTrue(TEXT("A mesma semente dá o mesmo labirinto"), PlantasDaCavernaIguais(Uma, Outra));
	TestFalse(TEXT("Outra semente dá outro labirinto"), PlantasDaCavernaIguais(Uma, Diferente));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveLabyrinthRejectsEmptyGridTest,
	"BattleSquare.CaveLabyrinth.RejectsEmptyGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveLabyrinthRejectsEmptyGridTest::RunTest(const FString& Parameters)
{
	// Grade inválida devolve planta vazia, e não meia planta: o ator que
	// construir a caverna precisa poder perguntar `IsValid` e parar.
	const CaveLabyrinth::FCaveGrid Vazia = CaveLabyrinth::Carve(0, 7, SementeDoLabirinto);

	TestFalse(TEXT("Grade sem coluna não é válida"), Vazia.IsValid());
	TestEqual(TEXT("Grade sem coluna não tem passagem"), CaveLabyrinth::OpenPassageCount(Vazia), 0);
	TestFalse(TEXT("Não há caminho em grade vazia"), CaveLabyrinth::HasPath(Vazia, 0, 0, 0, 0));

	return true;
}

#endif
