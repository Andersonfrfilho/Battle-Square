// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

/**
 * PODERES QUE DISTINGUEM SUBSTÂNCIA.
 *
 * A fundura já era dado (`SkillTerrainRequirement`), e foi ela que tirou o
 * "submergir exige água" de dentro de um `if` do núcleo. A substância ganha o
 * par: um poder que só funciona na lava passa a ser uma linha de configuração,
 * e não mais uma exceção no meio da fase.
 */

namespace ProvaDoRequisitoDeFluido
{
	FBattleState CampoDeAgua()
	{
		FBattleState Estado;
		Estado.GridColumns = 3;
		Estado.GridRows = 3;
		Estado.CellLayout.SetNumZeroed(9);
		Estado.ApplyDefaultTerrainRequirements();

		Estado.CellLayout[Estado.CellIndex(1, 1)] =
			static_cast<uint8>(ECellProperty::Water);

		return Estado;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillFluidRequirementLavaSkillFailsInEveryWaterTest,
	"BattleSim.SkillFluidRequirement.LavaSkillFailsInEveryWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillFluidRequirementLavaSkillFailsInEveryWaterTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = ProvaDoRequisitoDeFluido::CampoDeAgua();

	// Um poder que só funciona na LAVA. Uma linha de configuração — sem tocar
	// no núcleo, que é o ponto de o requisito ser dado.
	Estado.RequireFluidForSkill(EActionType::Atacar, EFluidKind::Lava);

	// FALHA EM TODA ÁGUA, e sem listar quais águas existem: a sexta água que
	// alguém acrescentar já nasce recusada, sem ninguém lembrar dela.
	int32 AguasConferidas = 0;
	for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
	{
		const EFluidKind Fluido = static_cast<EFluidKind>(Qual);
		if (!FluidRegistry::IsWater(Fluido))
		{
			continue;
		}

		++AguasConferidas;
		Estado.SetFluidAt(1, 1, Fluido);

		if (Estado.CellAllowsSkill(EActionType::Atacar, 1, 1))
		{
			AddError(FString::Printf(
				TEXT("o poder que exige LAVA passou em %s"),
				FluidRegistry::TraitsOf(Fluido).DebugName));
			return false;
		}
	}

	TestTrue(TEXT("houve mais de uma agua para conferir"), AguasConferidas > 1);

	// E FUNCIONA NA LAVA — o contrapeso, sem o qual uma regra que recusasse
	// tudo passaria no laço acima e o poder nunca funcionaria em lugar nenhum.
	Estado.SetFluidAt(1, 1, EFluidKind::Lava);
	TestTrue(TEXT("o poder que exige lava funciona na lava"),
		Estado.CellAllowsSkill(EActionType::Atacar, 1, 1));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillFluidRequirementWaterSkillWorksInEveryWaterTest,
	"BattleSim.SkillFluidRequirement.WaterSkillWorksInEveryWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillFluidRequirementWaterSkillWorksInEveryWaterTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = ProvaDoRequisitoDeFluido::CampoDeAgua();

	// A outra metade do aceite da F3: um poder que exige ÁGUA funciona nas
	// cinco e falha na lava. Aqui ninguém declara substância — vale a regra
	// que a FUNDURA implica, e é ela que já fazia submergir recusar a lava.
	int32 Aguas = 0;
	for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
	{
		const EFluidKind Fluido = static_cast<EFluidKind>(Qual);
		if (!FluidRegistry::IsWater(Fluido))
		{
			continue;
		}

		++Aguas;
		Estado.SetFluidAt(1, 1, Fluido);
		TestTrue(*FString::Printf(TEXT("submergir vale em %s"),
			FluidRegistry::TraitsOf(Fluido).DebugName),
			Estado.CellAllowsSkill(EActionType::Submergir, 1, 1));
	}

	TestTrue(TEXT("as cinco aguas foram conferidas"), Aguas >= 5);

	Estado.SetFluidAt(1, 1, EFluidKind::Lava);
	TestFalse(TEXT("e falha na lava"),
		Estado.CellAllowsSkill(EActionType::Submergir, 1, 1));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillFluidRequirementTheTwoAxesCombineTest,
	"BattleSim.SkillFluidRequirement.TheTwoAxesCombine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillFluidRequirementTheTwoAxesCombineTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = ProvaDoRequisitoDeFluido::CampoDeAgua();

	// OS DOIS EIXOS SÃO INDEPENDENTES, e um poder pode pedir os dois: água
	// FUNDA **e** que ela seja termal. Se a substância declarada apagasse o
	// requisito de fundura, um poder de água termal funcionaria numa poça.
	Estado.RequireTerrainForSkill(EActionType::Atacar, ECellProperty::Water);
	Estado.RequireFluidForSkill(EActionType::Atacar, EFluidKind::AguaTermal);
	Estado.SetFluidAt(1, 1, EFluidKind::AguaTermal);

	TestTrue(TEXT("agua termal funda: passa"),
		Estado.CellAllowsSkill(EActionType::Atacar, 1, 1));

	// Mesma substância, fundura MENOR: a poça não serve.
	Estado.CellLayout[Estado.CellIndex(1, 1)] =
		static_cast<uint8>(ECellProperty::ShallowWater);
	TestFalse(TEXT("agua termal RASA: nao passa"),
		Estado.CellAllowsSkill(EActionType::Atacar, 1, 1));

	// Fundura de volta, substância errada: também não serve.
	Estado.CellLayout[Estado.CellIndex(1, 1)] =
		static_cast<uint8>(ECellProperty::Water);
	Estado.SetFluidAt(1, 1, EFluidKind::AguaDoce);
	TestFalse(TEXT("agua DOCE funda: nao passa"),
		Estado.CellAllowsSkill(EActionType::Atacar, 1, 1));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillFluidRequirementEntersTheHashTest,
	"BattleSim.SkillFluidRequirement.EntersTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillFluidRequirementEntersTheHashTest::RunTest(const FString& Parameters)
{
	FBattleState Sem = ProvaDoRequisitoDeFluido::CampoDeAgua();

	FBattleState Com = ProvaDoRequisitoDeFluido::CampoDeAgua();
	Com.RequireFluidForSkill(EActionType::Atacar, EFluidKind::Lava);

	// Duas partidas em que um poder exige coisas diferentes são duas partidas
	// diferentes. Sem isto no hash, teriam a mesma assinatura — e a
	// dessincronia que o hash existe para pegar passaria.
	TestNotEqual(TEXT("exigir lava muda a assinatura"),
		Sem.ComputeHash(), Com.ComputeHash());

	return true;
}
