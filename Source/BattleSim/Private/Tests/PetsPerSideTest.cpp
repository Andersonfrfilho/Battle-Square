// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleTypes.h"
#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO (L-042).
namespace PetsPorLadoTeste
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
				Pet.Attack = 50; Pet.Defense = 50; Pet.Speed = 50;
				State.Pets.Add(Pet);
			}
		}

		State.PlaceDuelistsAtStartingCells();
		return State;
	}

	/** Quantos pets terminaram DIVIDINDO casa com outro. */
	int32 ContarEmpilhados(const FBattleState& State)
	{
		int32 Empilhados = 0;
		for (int32 Qual = 0; Qual < State.Pets.Num(); ++Qual)
		{
			for (int32 Outro = Qual + 1; Outro < State.Pets.Num(); ++Outro)
			{
				if (State.Pets[Qual].Column == State.Pets[Outro].Column
					&& State.Pets[Qual].Row == State.Pets[Outro].Row)
				{
					++Empilhados;
				}
			}
		}
		return Empilhados;
	}
}

// ---------------------------------------------------------------------------
// CP1 — MEDIR o teto de pets por lado.
//
// Nenhum número de pets por lado está escrito neste projeto, e NÃO SE INVENTA
// UM. `State.Pets` é um array sem teto declarado; o teto real está espalhado
// por quem consome o estado. Este teste descobre O QUE QUEBRA PRIMEIRO.
//
// ⚠️ ELE NÃO REPROVA POR O TETO SER BAIXO. Congelar uma decisão de produto
// dentro de um teste de medição é o erro que este projeto já cometeu ao cobrar
// zero onde o certo era cobrar o parâmetro. O que ele afirma é que a MEDIÇÃO
// aconteceu e que os números aparecem — a decisão sobre eles é do usuário.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetsPerSideCeilingIsMeasuredTest,
	"BattleSim.PetsPerSide.CeilingIsMeasured",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetsPerSideCeilingIsMeasuredTest::RunTest(const FString& Parameters)
{
	// ---- Os tetos ESTRUTURAIS, que não dependem de montar nada ----
	//
	// Estes são os únicos números que o código já declara. Lê-los é o começo:
	// se algum deles fosse o menor, a discussão acabaria aqui.
	AddInfo(FString::Printf(
		TEXT("teto de EIXO da grade: %d casas — PackCell guarda 4 bits por eixo"), 15));
	AddInfo(FString::Printf(
		TEXT("teto de PetId: %d — o campo e uint8"), 255));
	AddInfo(FString::Printf(
		TEXT("acoes por turno: %d — FTurnCommit::ActionsPerTurn"),
		FTurnCommit::ActionsPerTurn));

	// ---- O teto que a GRADE impõe ----
	//
	// Um lado nasce na coluna da ponta. Quantos cabem lá sem empilhar é a
	// altura da grade — e a grade sai do `DefaultGame.ini`, podendo ser 3x2.
	for (const uint8 Linhas : { uint8(2), uint8(3), uint8(6) })
	{
		AddInfo(FString::Printf(
			TEXT("grade de %d linhas: cabem %d por lado na coluna da ponta"),
			Linhas, Linhas));
	}

	// ---- O que QUEBRA PRIMEIRO, medido ----
	//
	// Monta 2, 3 e 4 por lado e conta quantos terminam dividindo casa. Hoje
	// `PlaceDuelistsAtStartingCells` põe TODOS de um lado na mesma casa, e é
	// isso que a CP2 conserta — mas o número tem de sair medido, não suposto.
	int32 PrimeiroQueEmpilha = 0;
	for (const int32 PorLado : { 1, 2, 3, 4 })
	{
		const FBattleState State = PetsPorLadoTeste::MontarComNPorLado(PorLado, 3, 3);
		const int32 Empilhados = PetsPorLadoTeste::ContarEmpilhados(State);

		AddInfo(FString::Printf(TEXT("%d por lado: %d pares dividindo casa"),
			PorLado, Empilhados));

		if (Empilhados > 0 && PrimeiroQueEmpilha == 0)
		{
			PrimeiroQueEmpilha = PorLado;
		}
	}

	AddInfo(FString::Printf(
		TEXT("QUEBRA PRIMEIRO em %d por lado, e quem impoe e "
			 "FBattleState::PlaceDuelistsAtStartingCells"),
		PrimeiroQueEmpilha));

	// A ÚNICA afirmação: a medição ACONTECEU e achou o ponto de quebra. Sem
	// isto, um erro no montador do teste faria a medição sair vazia e passar.
	TestTrue(TEXT("a medicao achou o ponto em que a colocacao empilha"),
		PrimeiroQueEmpilha > 0);

	// E ela achou onde a leitura do código diz que acharia: com UM por lado
	// ninguém divide casa, porque os dois lados nascem em colunas opostas.
	TestEqual(TEXT("com um por lado, ninguem divide casa"),
		PetsPorLadoTeste::ContarEmpilhados(
			PetsPorLadoTeste::MontarComNPorLado(1, 3, 3)), 0);

	return true;
}

// O HASH CONTINUA ESTÁVEL com muitos pets — e a medição confirma em vez de
// presumir. `ComputeHash` ordena por `PetId`; se a ordem do array influenciasse,
// dois clientes com os mesmos pets em ordem diferente divergiriam, e isso só
// apareceria numa partida em rede depois de a jogada já ter divergido.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetsPerSideHashIsOrderIndependentTest,
	"BattleSim.PetsPerSide.HashIsOrderIndependent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetsPerSideHashIsOrderIndependentTest::RunTest(const FString& Parameters)
{
	FBattleState Direta = PetsPorLadoTeste::MontarComNPorLado(3, 3, 3);
	FBattleState Invertida = Direta;
	Algo::Reverse(Invertida.Pets);

	AddInfo(FString::Printf(TEXT("6 pets: hash direto %u, invertido %u"),
		Direta.ComputeHash(), Invertida.ComputeHash()));

	TestEqual(TEXT("a ORDEM do array nao muda o hash"),
		Direta.ComputeHash(), Invertida.ComputeHash());

	// CONTRAPESO: mudar um pet DE VERDADE muda o hash. Sem ele, um
	// `ComputeHash` que devolvesse constante passaria no teste de cima.
	Invertida.Pets[0].Health -= 1;
	TestNotEqual(TEXT("mas mudar a vida de um pet muda"),
		Direta.ComputeHash(), Invertida.ComputeHash());

	return true;
}
