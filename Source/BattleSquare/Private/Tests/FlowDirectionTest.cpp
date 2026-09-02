// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Battle/BattleTypes.h"
#include "Environment/FreshWater.h"
#include "Misc/AutomationTest.h"
#include "World/IslandBakedPlan.h"

/**
 * A CASA SABE PARA ONDE A ÁGUA CORRE.
 *
 * O sentido é LIDO do traçado — a polilinha do curso já é ordenada da nascente
 * para a foz. Deduzi-lo de novo a partir do raio, do declive ou da geometria
 * seria uma segunda verdade, e ela concordaria com esta até a primeira edição,
 * com o rio correndo para trás num trecho que ninguém olhou.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowDirectionFollowsTheCourseOrderTest,
	"BattleSquare.FlowDirection.FollowsTheCourseOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowDirectionFollowsTheCourseOrderTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// O RUMO ASSADO tem de concordar com a ORDEM da polilinha, ponto a ponto.
	//
	// E o eixo da LINHA é invertido em relação ao Y do mundo: na grade, `Cima`
	// é linha MENOS UM. Ignorar isso faz a água correr para o lado errado — é
	// o defeito de "Baixo andava para a direita" deste projeto, e este teste
	// existe para ele não voltar.
	int32 Conferidos = 0;
	int32 Divergiram = 0;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (int32 Ponto = 0; Ponto + 1 < Curso.PointsUnits.Num(); ++Ponto)
		{
			if (!Curso.FlowDirectionByPoint.IsValidIndex(Ponto))
			{
				continue;
			}

			const FVector2D Passo =
				Curso.PointsUnits[Ponto + 1] - Curso.PointsUnits[Ponto];
			if (Passo.IsNearlyZero())
			{
				continue;
			}

			int8 EmColuna = 0;
			int8 EmLinha = 0;
			GetDirectionDelta(
				static_cast<EBattleDirection>(Curso.FlowDirectionByPoint[Ponto]),
				EmColuna, EmLinha);

			++Conferidos;

			// A coluna acompanha o X do mundo; a LINHA é invertida.
			const bool bColunaBate = FMath::IsNearlyZero(Passo.X, 1.0f)
				|| (Passo.X > 0.0f) == (EmColuna > 0);
			const bool bLinhaBate = FMath::IsNearlyZero(Passo.Y, 1.0f)
				|| (Passo.Y > 0.0f) == (EmLinha < 0);

			if (!bColunaBate || !bLinhaBate)
			{
				++Divergiram;
				if (Divergiram == 1)
				{
					AddError(FString::Printf(
						TEXT("o rumo assado nao acompanha a polilinha: passo ")
						TEXT("(%.0f,%.0f) virou coluna %d, linha %d"),
						Passo.X, Passo.Y, EmColuna, EmLinha));
				}
			}
		}
	}

	TestTrue(TEXT("houve trecho para conferir"), Conferidos > 1000);
	TestEqual(TEXT("nenhum trecho corre para o lado errado"), Divergiram, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowDirectionTheRiverDoesNotShiverTest,
	"BattleSquare.FlowDirection.TheRiverDoesNotShiver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowDirectionTheRiverDoesNotShiverTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A ESCOLHA DE OITO RUMOS FOI MEDIDA, e esta prova é a medição virando
	// afirmação: encaixar a bacia em oito direções troca de rumo em 2% dos
	// passos e VAI-E-VOLTA (A→B→A) em 0,3%. O rio não treme.
	//
	// Se um dia ele tremer, oito rumos deixaram de bastar — e o número aqui é
	// o que avisa, em vez de alguém achar estranho olhando a tela.
	int32 Passos = 0;
	int32 VaiEVolta = 0;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (int32 Ponto = 2; Ponto < Curso.FlowDirectionByPoint.Num(); ++Ponto)
		{
			++Passos;
			const uint8 Antes = Curso.FlowDirectionByPoint[Ponto - 2];
			const uint8 Meio = Curso.FlowDirectionByPoint[Ponto - 1];
			const uint8 Agora = Curso.FlowDirectionByPoint[Ponto];

			if (Agora == Antes && Agora != Meio)
			{
				++VaiEVolta;
			}
		}
	}

	if (Passos == 0)
	{
		AddError(TEXT("nenhum passo de rumo para medir"));
		return false;
	}

	// Teto de 5%, contra os 0,3% medidos — folga para o traçado mudar sem que
	// isto vire alarme falso, e apertado o bastante para pegar tremor de
	// verdade.
	const int32 PorMil = (VaiEVolta * 1000) / Passos;
	if (PorMil > 50)
	{
		AddError(FString::Printf(
			TEXT("o rumo vai-e-volta em %d por mil dos passos — oito rumos ")
			TEXT("deixaram de bastar"), PorMil));
		return false;
	}

	return true;
}
