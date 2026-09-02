// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO (L-042), e a variável de arquivo teria nome próprio se
// houvesse alguma (L-045).
namespace CasasIniciaisTeste
{
	FBattleState MontarComNPorLado(int32 QuantosPorLado, uint8 Colunas, uint8 Linhas)
	{
		FBattleState State;
		State.GridColumns = Colunas;
		State.GridRows = Linhas;
		State.CellLayout.SetNumZeroed(Colunas * Linhas);

		uint8 ProximoId = 1;
		for (uint8 Lado = 0; Lado < 2; ++Lado)
		{
			for (int32 Qual = 0; Qual < QuantosPorLado; ++Qual)
			{
				FPetState Pet;
				Pet.PetId = ProximoId++;
				Pet.Side = Lado;
				Pet.Health = 100; Pet.MaxHealth = 100;
				State.Pets.Add(Pet);
			}
		}

		State.PlaceDuelistsAtStartingCells();
		return State;
	}

	int32 ContarParesNaMesmaCasa(const FBattleState& State)
	{
		int32 Pares = 0;
		for (int32 Qual = 0; Qual < State.Pets.Num(); ++Qual)
		{
			for (int32 Outro = Qual + 1; Outro < State.Pets.Num(); ++Outro)
			{
				if (State.Pets[Qual].Column == State.Pets[Outro].Column
					&& State.Pets[Qual].Row == State.Pets[Outro].Row)
				{
					++Pares;
				}
			}
		}
		return Pares;
	}
}

// ---------------------------------------------------------------------------
// CP2 — ALIADOS NASCEM EM CASAS DIFERENTES.
//
// Antes disto, TODO pet de um lado ia para a mesma casa. Com dois aliados eles
// nasciam um dentro do outro, e a não-coabitação (DP-02) os barrava no primeiro
// slot: a batalha abria com os dois se empurrando sem ninguém ter escolhido
// nada. A CP1 mediu que isso quebra já no SEGUNDO pet.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStartingCellsAlliesDoNotShareTest,
	"BattleSim.StartingCells.AlliesDoNotShare",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FStartingCellsAlliesDoNotShareTest::RunTest(const FString& Parameters)
{
	// Até o teto que a CP1 mediu: a coluna da ponta de uma grade de 3 linhas
	// comporta 3 por lado.
	for (const int32 PorLado : { 2, 3 })
	{
		const FBattleState State =
			CasasIniciaisTeste::MontarComNPorLado(PorLado, 3, 3);

		TestEqual(*FString::Printf(
			TEXT("%d por lado: ninguem divide casa"), PorLado),
			CasasIniciaisTeste::ContarParesNaMesmaCasa(State), 0);

		// E cada lado continua NA SUA COLUNA: abrir para os lados não pode
		// virar abrir para o campo do outro.
		for (const FPetState& Pet : State.Pets)
		{
			TestEqual(*FString::Printf(TEXT("pet %d na coluna do lado dele"),
				Pet.PetId),
				static_cast<int32>(Pet.Column),
				Pet.Side == 0 ? 0 : static_cast<int32>(State.GridColumns) - 1);
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// CONTRAPESO, E ELE É O QUE MAIS IMPORTA AQUI.
//
// UM pet por lado continua nascendo EXATAMENTE onde nascia: coluna 0 e última
// coluna, linha do meio arredondando para baixo. Um duelo que muda de casa
// inicial invalida todo instantâneo de determinismo de cenário — inclusive os
// que nem têm aliado, que são a maioria.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStartingCellsDuelIsUnchangedTest,
	"BattleSim.StartingCells.DuelIsUnchanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FStartingCellsDuelIsUnchangedTest::RunTest(const FString& Parameters)
{
	// As três formas de grade que o projeto declara suportar, e a de altura
	// PAR incluída de propósito: é nela que "linha do meio" tem duas
	// respostas, e o arredondamento para baixo é o que estava escrito.
	const uint8 Formas[3][2] = { {3, 3}, {4, 6}, {3, 2} };

	for (const auto& Forma : Formas)
	{
		const FBattleState State =
			CasasIniciaisTeste::MontarComNPorLado(1, Forma[0], Forma[1]);

		const uint8 LinhaEsperada = static_cast<uint8>((Forma[1] - 1) / 2);

		TestEqual(*FString::Printf(TEXT("%dx%d: lado 0 na coluna 0"),
			Forma[0], Forma[1]),
			static_cast<int32>(State.Pets[0].Column), 0);
		TestEqual(*FString::Printf(TEXT("%dx%d: lado 0 na linha do meio"),
			Forma[0], Forma[1]),
			static_cast<int32>(State.Pets[0].Row),
			static_cast<int32>(LinhaEsperada));

		TestEqual(*FString::Printf(TEXT("%dx%d: lado 1 na ultima coluna"),
			Forma[0], Forma[1]),
			static_cast<int32>(State.Pets[1].Column),
			static_cast<int32>(Forma[0]) - 1);
		TestEqual(*FString::Printf(TEXT("%dx%d: lado 1 na MESMA linha"),
			Forma[0], Forma[1]),
			static_cast<int32>(State.Pets[1].Row),
			static_cast<int32>(LinhaEsperada));
	}

	return true;
}

// A BATALHA CONTINUA SIMÉTRICA: o pet de ordem N de um lado nasce na mesma
// LINHA que o de ordem N do outro. Sem isto, abrir para os lados poderia dar
// vantagem posicional a um dos lados, e a vantagem seria invisível — nada na
// tela diria que o lado 1 nasceu mais perto do canto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStartingCellsAreSymmetricTest,
	"BattleSim.StartingCells.AreSymmetric",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FStartingCellsAreSymmetricTest::RunTest(const FString& Parameters)
{
	const FBattleState State = CasasIniciaisTeste::MontarComNPorLado(3, 3, 3);

	for (int32 Ordem = 0; Ordem < 3; ++Ordem)
	{
		TestEqual(*FString::Printf(TEXT("o %d de cada lado nasce na mesma linha"),
			Ordem),
			static_cast<int32>(State.Pets[Ordem].Row),
			static_cast<int32>(State.Pets[3 + Ordem].Row));
	}

	return true;
}

// MAIS PETS DO QUE A COLUNA COMPORTA volta a empilhar — e isso é HONESTO, não
// um defeito escondido. A CP1 mediu que o teto por lado é a altura da grade;
// passar dele é decisão de quem monta a partida, e este teste guarda o
// comportamento para que a decisão seja tomada sabendo o que acontece.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStartingCellsBeyondTheCeilingStackTest,
	"BattleSim.StartingCells.BeyondTheCeilingStack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FStartingCellsBeyondTheCeilingStackTest::RunTest(const FString& Parameters)
{
	// Quatro por lado numa coluna de três casas.
	const FBattleState State = CasasIniciaisTeste::MontarComNPorLado(4, 3, 3);
	const int32 Pares = CasasIniciaisTeste::ContarParesNaMesmaCasa(State);

	AddInfo(FString::Printf(
		TEXT("4 por lado em coluna de 3 casas: %d pares dividindo casa"), Pares));

	TestTrue(TEXT("passar do teto empilha, e o teste diz quanto"), Pares > 0);

	// E NINGUÉM SAI DA GRADE — que seria o defeito de verdade: uma linha
	// negativa ou além do fim empacotaria errado em `PackCell` e o pet
	// apareceria numa casa que não é a dele.
	for (const FPetState& Pet : State.Pets)
	{
		TestTrue(TEXT("a linha continua dentro da grade"),
			Pet.Row < State.GridRows);
	}

	return true;
}
