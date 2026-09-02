// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"

/**
 * A CASA SABE DE QUE FLUIDO ELA É — o eixo da substância chegando ao tabuleiro.
 *
 * Enquanto o registro só respondia *sobre* um fluido e ninguém dizia que a casa
 * X tem o fluido Y, o eixo novo não tocava o jogo. Esta é a ligação.
 */

namespace ProvaDoFluidoDaCasa
{
	/** Um estado pequeno, com a grade montada e uma casa de água funda. */
	FBattleState ComUmaAgua()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellFluidDefaultsToFreshWaterTest,
	"BattleSim.CellFluid.DefaultsToFreshWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellFluidDefaultsToFreshWaterTest::RunTest(const FString& Parameters)
{
	const FBattleState Estado = ProvaDoFluidoDaCasa::ComUmaAgua();

	// A lista pode ficar VAZIA, e vazia quer dizer "tudo no padrão" — assim
	// uma arena comum não paga um byte por casa para dizer que água é água.
	TestEqual(TEXT("a lista de fluidos nasce vazia"), Estado.CellFluid.Num(), 0);

	// E mesmo vazia, a casa responde: a ilha é de água doce.
	TestEqual(TEXT("a casa de agua e agua doce"),
		static_cast<int32>(Estado.FluidAt(1, 1)),
		static_cast<int32>(EFluidKind::AguaDoce));

	// Casa seca não tem fluido nenhum.
	TestEqual(TEXT("a casa seca nao tem fluido"),
		static_cast<int32>(Estado.FluidAt(0, 0)),
		static_cast<int32>(EFluidKind::Nenhum));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellFluidIceIsNotAFluidTest,
	"BattleSim.CellFluid.IceIsNotAFluid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellFluidIceIsNotAFluidTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = ProvaDoFluidoDaCasa::ComUmaAgua();
	Estado.CellLayout[Estado.CellIndex(2, 2)] = static_cast<uint8>(ECellProperty::Ice);

	// No gelo se PISA, não se entra. Quem está sobre ele não está dentro de
	// fluido nenhum — e é isso que faz o escorregão dele ser regra de CHÃO e
	// não de líquido. Tratar gelo como fluido faria submergir valer nele.
	TestEqual(TEXT("o gelo nao e fluido"),
		static_cast<int32>(Estado.FluidAt(2, 2)),
		static_cast<int32>(EFluidKind::Nenhum));

	// A LAMA, sim: nela se afunda.
	Estado.CellLayout[Estado.CellIndex(0, 2)] = static_cast<uint8>(ECellProperty::Mud);
	TestEqual(TEXT("a lama e lama"),
		static_cast<int32>(Estado.FluidAt(0, 2)),
		static_cast<int32>(EFluidKind::Lama));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellFluidSubmergeFailsInLavaTest,
	"BattleSim.CellFluid.SubmergeFailsInLava",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellFluidSubmergeFailsInLavaTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = ProvaDoFluidoDaCasa::ComUmaAgua();

	// O ACEITE DA F2, e a razão de todo este eixo existir.
	//
	// A casa é água FUNDA — a fundura permite. Antes do registro, isso bastava:
	// `RequireTerrainForSkill(Submergir, Water)` olhava só a fundura, e
	// submergir numa casa de lava marcada como água funda PASSAVA, calado.
	TestTrue(TEXT("na agua funda, submergir vale"),
		Estado.CellAllowsSkill(EActionType::Submergir, 1, 1));

	// Mesma casa, mesma fundura, outra SUBSTÂNCIA.
	Estado.SetFluidAt(1, 1, EFluidKind::Lava);

	TestEqual(TEXT("a casa agora e de lava"),
		static_cast<int32>(Estado.FluidAt(1, 1)),
		static_cast<int32>(EFluidKind::Lava));

	// A fundura continua dizendo que sim — e é isso que prova que quem recusou
	// foi a substância, e não algum efeito colateral no layout.
	TestTrue(TEXT("a fundura continua permitindo"),
		Estado.TerrainAllowsSkill(EActionType::Submergir,
			Estado.CellLayout[Estado.CellIndex(1, 1)]));

	if (Estado.CellAllowsSkill(EActionType::Submergir, 1, 1))
	{
		AddError(TEXT("submergir passou numa casa de LAVA marcada como agua funda ")
			TEXT("— o eixo da substancia nao esta sendo consultado"));
		return false;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellFluidEveryWaterStillAllowsSubmergeTest,
	"BattleSim.CellFluid.EveryWaterStillAllowsSubmerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellFluidEveryWaterStillAllowsSubmergeTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = ProvaDoFluidoDaCasa::ComUmaAgua();

	// O contrapeso do teste acima: uma regra que recusasse TUDO passaria lá e
	// quebraria o jogo inteiro em silêncio — submergir nunca mais funcionaria,
	// e o teste da lava continuaria verde.
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

	TestTrue(TEXT("houve mais de uma agua para conferir"), Aguas > 1);

	// E numa casa SECA continua não valendo, seja qual for o fluido declarado:
	// a fundura manda no eixo dela.
	Estado.SetFluidAt(0, 0, EFluidKind::AguaDoce);
	TestFalse(TEXT("submergir nao vale em casa seca"),
		Estado.CellAllowsSkill(EActionType::Submergir, 0, 0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellFluidTheSubstanceEntersTheHashTest,
	"BattleSim.CellFluid.TheSubstanceEntersTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellFluidTheSubstanceEntersTheHashTest::RunTest(const FString& Parameters)
{
	FBattleState Agua = ProvaDoFluidoDaCasa::ComUmaAgua();
	FBattleState Lava = ProvaDoFluidoDaCasa::ComUmaAgua();
	Lava.SetFluidAt(1, 1, EFluidKind::Lava);

	// Dois tabuleiros com a mesma FUNDURA e substâncias diferentes são duas
	// partidas diferentes. Sem o fluido no hash, teriam a mesma assinatura — e
	// a dessincronia que o hash existe para pegar passaria.
	TestNotEqual(TEXT("agua e lava dao assinaturas diferentes"),
		Agua.ComputeHash(), Lava.ComputeHash());

	// E O CASO QUE O HASH DERIVADO RESOLVE: uma lista VAZIA e uma lista
	// materializada só de zeros querem dizer exatamente a mesma coisa — tudo no
	// padrão. Somando o array cru, elas dariam assinaturas diferentes sendo
	// idênticas, e isso seria uma dessincronia fantasma, das piores de achar.
	FBattleState Materializada = ProvaDoFluidoDaCasa::ComUmaAgua();
	Materializada.CellFluid.SetNumZeroed(Materializada.CellLayout.Num());

	TestEqual(TEXT("lista vazia e lista de zeros dao a MESMA assinatura"),
		Materializada.ComputeHash(), Agua.ComputeHash());

	// E declarar o padrão explicitamente também não muda nada: a água doce numa
	// casa de água já era água doce.
	FBattleState Explicita = ProvaDoFluidoDaCasa::ComUmaAgua();
	Explicita.SetFluidAt(1, 1, EFluidKind::AguaDoce);
	TestEqual(TEXT("declarar o padrao nao muda a assinatura"),
		Explicita.ComputeHash(), Agua.ComputeHash());

	return true;
}
