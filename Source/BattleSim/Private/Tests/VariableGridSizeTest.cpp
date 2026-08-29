// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FPetState MakeGridSizedPet(uint8 PetId, uint8 Side)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Health = 50;
		Pet.MaxHealth = 50;
		Pet.Attack = 10;
		Pet.Defense = 5;
		Pet.Speed = 5;
		return Pet;
	}

	FBattleState MakeGridSizedDuel(int32 Colunas, int32 Linhas)
	{
		FBattleState Estado;
		Estado.ResizeGrid(Colunas, Linhas);
		Estado.Pets.Add(MakeGridSizedPet(1, 0));
		Estado.Pets.Add(MakeGridSizedPet(2, 1));
		Estado.PlaceDuelistsAtStartingCells();
		Estado.Random.State = 12345;
		return Estado;
	}
}

// A grade deixou de ser 3x3 fixo. O layout precisa acompanhar as
// dimensões: um CellLayout com o tamanho antigo indexado com as colunas
// novas devolve a casa errada, calado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridResizeMatchesLayoutTest,
	"BattleSim.Grid.ResizeMatchesLayout",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGridResizeMatchesLayoutTest::RunTest(const FString& Parameters)
{
	FBattleState Padrao;
	TestEqual(TEXT("Grade nasce 3x3"), static_cast<int32>(Padrao.GridColumns), 3);
	TestEqual(TEXT("Layout padrão tem 9 casas"), Padrao.CellLayout.Num(), 9);

	FBattleState Retangular;
	Retangular.ResizeGrid(4, 6);
	TestEqual(TEXT("Colunas viram 4"), static_cast<int32>(Retangular.GridColumns), 4);
	TestEqual(TEXT("Linhas viram 6"), static_cast<int32>(Retangular.GridRows), 6);
	TestEqual(TEXT("Layout acompanha: 24 casas"), Retangular.CellLayout.Num(), 24);

	// Última casa da grade 4x6 é (3,5) — índice 23. Se o índice usasse
	// três colunas, cairia em 18 e sobrescreveria outra casa.
	TestEqual(TEXT("Índice da última casa usa as colunas da grade"),
		Retangular.CellIndex(3, 5), 23);

	TestTrue(TEXT("(3,5) está dentro de uma 4x6"), Retangular.IsInside(3, 5));
	TestFalse(TEXT("(4,0) está fora de uma 4x6"), Retangular.IsInside(4, 0));
	TestFalse(TEXT("(0,6) está fora de uma 4x6"), Retangular.IsInside(0, 6));

	return true;
}

// PackCell guarda 4 bits por eixo: acima de 15 a coluna daria a volta e
// viraria outra casa no trace. Recortar é preferível a recusar — número
// errado num .ini não deve impedir a batalha de existir.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridResizeClampsToPackableRangeTest,
	"BattleSim.Grid.ResizeClampsToPackableRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGridResizeClampsToPackableRangeTest::RunTest(const FString& Parameters)
{
	FBattleState Grande;
	Grande.ResizeGrid(99, 99);
	TestEqual(TEXT("Colunas recortadas ao teto de empacotamento"),
		static_cast<int32>(Grande.GridColumns), BattleGridMaxSide);
	TestEqual(TEXT("Linhas recortadas ao teto de empacotamento"),
		static_cast<int32>(Grande.GridRows), BattleGridMaxSide);

	FBattleState Vazia;
	Vazia.ResizeGrid(0, -4);
	TestEqual(TEXT("Grade nunca fica sem coluna"),
		static_cast<int32>(Vazia.GridColumns), BattleGridMinSide);
	TestEqual(TEXT("Grade nunca fica sem linha"),
		static_cast<int32>(Vazia.GridRows), BattleGridMinSide);

	return true;
}

// O tamanho da grade MUDA a batalha: mesma semente e mesmas ações num
// 3x3 e num 4x4 são duas partidas diferentes. Se o hash não visse a
// grade, essa diferença passaria como se fosse a mesma partida — e o
// hash existe justamente para detectar divergência.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridSizeEntersStateHashTest,
	"BattleSim.Grid.SizeEntersStateHash",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGridSizeEntersStateHashTest::RunTest(const FString& Parameters)
{
	FBattleState Quadrada = MakeGridSizedDuel(3, 3);
	FBattleState Maior = MakeGridSizedDuel(4, 4);

	TestNotEqual(TEXT("Grades diferentes têm assinaturas diferentes"),
		Quadrada.ComputeHash(), Maior.ComputeHash());

	TestEqual(TEXT("Mesma grade e mesmo conteúdo dão a mesma assinatura"),
		MakeGridSizedDuel(3, 3).ComputeHash(), Quadrada.ComputeHash());

	return true;
}

// O pet do jogador começa na PRIMEIRA coluna, o oponente na última, os
// dois na linha do meio — e isso vale em qualquer tamanho de campo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDuelistsStartOnOppositeEdgesTest,
	"BattleSim.Grid.DuelistsStartOnOppositeEdges",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDuelistsStartOnOppositeEdgesTest::RunTest(const FString& Parameters)
{
	struct FCaso
	{
		int32 Colunas;
		int32 Linhas;
		int32 ColunaDoOponente;
		int32 LinhaDeAmbos;
	};

	const FCaso Casos[] = {
		{ 3, 3, 2, 1 },   // o campo de hoje: jogador em (0,1)
		{ 4, 4, 3, 1 },   // altura par: meio arredonda para baixo
		{ 5, 5, 4, 2 },
		{ 3, 2, 2, 0 },   // retangular baixo
		{ 4, 6, 3, 2 },   // retangular alto
	};

	for (const FCaso& Caso : Casos)
	{
		const FBattleState Estado = MakeGridSizedDuel(Caso.Colunas, Caso.Linhas);
		const FPetState& Jogador = Estado.Pets[0];
		const FPetState& Oponente = Estado.Pets[1];

		TestEqual(*FString::Printf(TEXT("%dx%d: jogador na primeira coluna"),
			Caso.Colunas, Caso.Linhas), static_cast<int32>(Jogador.Column), 0);
		TestEqual(*FString::Printf(TEXT("%dx%d: oponente na última coluna"),
			Caso.Colunas, Caso.Linhas), static_cast<int32>(Oponente.Column), Caso.ColunaDoOponente);
		TestEqual(*FString::Printf(TEXT("%dx%d: jogador na linha do meio"),
			Caso.Colunas, Caso.Linhas), static_cast<int32>(Jogador.Row), Caso.LinhaDeAmbos);
		TestEqual(*FString::Printf(TEXT("%dx%d: oponente na mesma linha"),
			Caso.Colunas, Caso.Linhas), static_cast<int32>(Oponente.Row), Caso.LinhaDeAmbos);
	}

	return true;
}

// A borda que importa é a do EIXO. Num campo 5x2, a coluna 4 é válida e
// a linha 2 não — o mesmo número, respostas opostas. Um limite único
// para os dois eixos erraria um dos dois em todo campo retangular.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMovementRespectsPerAxisBoundsTest,
	"BattleSim.Grid.MovementRespectsPerAxisBounds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMovementRespectsPerAxisBoundsTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = MakeGridSizedDuel(5, 2);

	TestTrue(TEXT("Coluna 4 cabe num campo de 5 colunas"), Estado.IsInside(4, 0));
	TestFalse(TEXT("Linha 4 não cabe num campo de 2 linhas"), Estado.IsInside(0, 4));

	// O pet do jogador nasce em (0,0) num 5x2 e não pode ir para a
	// esquerda; o do oponente nasce em (4,0) e não pode ir para a direita.
	FTurnCommit Jogador;
	Jogador.Actions[0].Type = EActionType::Mover;
	Jogador.Actions[0].Direction = EBattleDirection::Esquerda;
	FTurnCommit Oponente;
	Oponente.Actions[0].Type = EActionType::Mover;
	Oponente.Actions[0].Direction = EBattleDirection::Direita;

	const FBattleResolveResult Resultado =
		FBattleResolver::ResolveTurn(Estado, Jogador, Oponente);

	TestEqual(TEXT("Jogador barrado pela borda esquerda"),
		static_cast<int32>(Resultado.NextState.Pets[0].Column), 0);
	TestEqual(TEXT("Oponente barrado pela borda direita"),
		static_cast<int32>(Resultado.NextState.Pets[1].Column), 4);

	return true;
}
