// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

/**
 * A CORRENTE NA CASA — a metade que é do NÚCLEO.
 *
 * Mora no `BattleSim` porque `ComputeHash` é do núcleo e não é exportado: um
 * teste noutro módulo obrigaria a alargar a fronteira do núcleo por
 * conveniência de teste, e a fronteira é o que faz o núcleo ser núcleo.
 *
 * A metade que lê o traçado assado fica em `BattleSquare`, onde o assado mora.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowDirectionStillWaterHasNoHeadingTest,
	"BattleSim.CellFlow.StillWaterHasNoHeading",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowDirectionStillWaterHasNoHeadingTest::RunTest(const FString& Parameters)
{
	// O CONTRAPESO: sem ele, uma regra que desse rumo a toda casa passaria em
	// tudo acima, e a poça parada empurraria quem entrasse nela.
	FBattleState Estado;
	Estado.GridColumns = 3;
	Estado.GridRows = 3;
	Estado.CellLayout.SetNumZeroed(9);

	TestEqual(TEXT("a lista de corrente nasce vazia"),
		Estado.CellFlowDirection.Num(), 0);
	TestEqual(TEXT("casa sem corrente nao tem rumo"),
		static_cast<int32>(Estado.FlowDirectionAt(1, 1)),
		static_cast<int32>(EBattleDirection::Nenhuma));
	TestEqual(TEXT("e nao tem forca"), Estado.FlowStrengthAt(1, 1), 0);

	// FORÇA SEM RUMO É ZERO, sempre. Uma casa com força e sem direção seria
	// uma corrente que empurra para lugar nenhum, e o empurrão sairia MUDO em
	// vez de errado — que é a falha mais cara de achar.
	Estado.SetFlowAt(1, 1, EBattleDirection::Nenhuma, 200);
	TestEqual(TEXT("forca sem rumo vale zero"), Estado.FlowStrengthAt(1, 1), 0);

    // E com rumo ela vale.
	Estado.SetFlowAt(1, 1, EBattleDirection::Direita, 200);
	TestEqual(TEXT("com rumo, a forca vale"), Estado.FlowStrengthAt(1, 1), 200);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowDirectionEntersTheHashTest,
	"BattleSim.CellFlow.EntersTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowDirectionEntersTheHashTest::RunTest(const FString& Parameters)
{
	FBattleState Parada;
	Parada.GridColumns = 3;
	Parada.GridRows = 3;
	Parada.CellLayout.SetNumZeroed(9);

	FBattleState Corrente = Parada;
	Corrente.SetFlowAt(1, 1, EBattleDirection::Direita, 200);

	// Dois tabuleiros com a mesma água, um correndo e outro parado, são duas
	// partidas diferentes.
	TestNotEqual(TEXT("a corrente muda a assinatura"),
		Parada.ComputeHash(), Corrente.ComputeHash());

	// E o mesmo caso do fluido: lista VAZIA e lista de ZEROS querem dizer a
	// mesma coisa — água parada. Somando o array cru, dariam assinaturas
	// diferentes sendo idênticas, e isso é dessincronia fantasma.
	FBattleState Materializada = Parada;
	Materializada.CellFlowDirection.SetNumZeroed(9);
	Materializada.CellFlowStrength.SetNumZeroed(9);

	TestEqual(TEXT("lista vazia e lista de zeros dao a MESMA assinatura"),
		Materializada.ComputeHash(), Parada.ComputeHash());

	return true;
}
