// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Battle/FluidRegistry.h"

/**
 * O REGISTRO DOS FLUIDOS — a substância, que é o eixo que faltava.
 *
 * O que se cobra aqui não é que a tabela exista: é que ela SEPARE. Uma tabela
 * em que todos os fluidos têm os mesmos números passa em qualquer teste de
 * leitura e não distingue nada — e era exatamente esse o estado anterior, com
 * uma água só valendo para tudo.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidRegistryEveryFluidHasARowTest,
	"BattleSim.FluidRegistry.EveryFluidHasARow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidRegistryEveryFluidHasARowTest::RunTest(const FString& Parameters)
{
	// TODO valor do enum tem linha. O dia em que alguém acrescentar um fluido
	// e esquecer a linha, ele vira casa seca em silêncio — e um poder que
	// deveria reagir a ele simplesmente não reage, sem erro nenhum.
	for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
	{
		const EFluidKind Fluido = static_cast<EFluidKind>(Qual);
		const FFluidTraits& Linha = FluidRegistry::TraitsOf(Fluido);

		if (Linha.DensityPerMille <= 0)
		{
			AddError(FString::Printf(
				TEXT("o fluido %d nao tem densidade — ele nao foi registrado, e ")
				TEXT("nada boia nem afunda nele"), Qual));
			return false;
		}

		TestNotEqual(*FString::Printf(TEXT("o fluido %d tem nome"), Qual),
			FString(Linha.DebugName), FString(TEXT("?")));
	}

	// E a casa SECA é a única sem densidade: se ela tivesse uma, o vazio
	// passaria a ter empuxo.
	TestEqual(TEXT("casa seca nao tem densidade"),
		FluidRegistry::TraitsOf(EFluidKind::Nenhum).DensityPerMille, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidRegistryLavaIsNotWaterTest,
	"BattleSim.FluidRegistry.LavaIsNotWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidRegistryLavaIsNotWaterTest::RunTest(const FString& Parameters)
{
	// A RAZÃO DE O REGISTRO EXISTIR, numa linha.
	//
	// Antes dele, `RequireTerrainForSkill(Submergir, Water)` aprovava qualquer
	// casa marcada como água — e uma casa de lava marcada como água deixava
	// submergir. A substância é o que separa as duas.
	TestFalse(TEXT("lava nao e agua"), FluidRegistry::IsWater(EFluidKind::Lava));
	TestFalse(TEXT("nao se submerge na lava"),
		FluidRegistry::AllowsSubmerge(EFluidKind::Lava));
	TestTrue(TEXT("estar na lava custa"),
		FluidRegistry::DamagePerSlot(EFluidKind::Lava) > 0);

	// E a água não machuca: molhar não é queimar. Sem esta linha, um dano
	// global posto por engano em todos os fluidos passaria despercebido.
	TestEqual(TEXT("estar na agua doce nao custa"),
		FluidRegistry::DamagePerSlot(EFluidKind::AguaDoce), 0);

	// LAMA também não é água, e também não se submerge: afundar em lama não é
	// mergulhar, é atolar. Ela é o caso que um `bIsWater` frouxo pegaria junto.
	TestFalse(TEXT("lama nao e agua"), FluidRegistry::IsWater(EFluidKind::Lama));
	TestFalse(TEXT("nao se submerge na lama"),
		FluidRegistry::AllowsSubmerge(EFluidKind::Lama));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidRegistryEveryWaterIsSubmergibleTest,
	"BattleSim.FluidRegistry.EveryWaterIsSubmergible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidRegistryEveryWaterIsSubmergibleTest::RunTest(const FString& Parameters)
{
	// O ganho prático do `bIsWater`: um poder pode valer para ÁGUA sem
	// enumerar as cinco. Sem ele, cada skill nova nasceria com uma lista, e a
	// sexta água ficaria de fora de todas elas — calada.
	//
	// Aqui se afirma a coerência que torna isso seguro: se é água, submerge.
	int32 Aguas = 0;
	for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
	{
		const EFluidKind Fluido = static_cast<EFluidKind>(Qual);
		if (!FluidRegistry::IsWater(Fluido))
		{
			continue;
		}

		++Aguas;
		TestTrue(*FString::Printf(TEXT("se %s e agua, submerge"),
			FluidRegistry::TraitsOf(Fluido).DebugName),
			FluidRegistry::AllowsSubmerge(Fluido));
	}

	// Zero águas faria o laço acima passar sem afirmar nada, e o registro
	// inteiro deixaria de ter sobre o que falar.
	TestTrue(TEXT("existem aguas registradas"), Aguas > 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidRegistryTheFluidsActuallyDifferTest,
	"BattleSim.FluidRegistry.TheFluidsActuallyDiffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidRegistryTheFluidsActuallyDifferTest::RunTest(const FString& Parameters)
{
	// UMA TABELA QUE NÃO SEPARA NÃO É REGISTRO, é um enum com nomes bonitos.
	// Se todos os fluidos tivessem os mesmos números, cada teste acima
	// passaria — as leituras funcionam — e nenhum poder conseguiria distinguir
	// coisa nenhuma, que é o estado de onde esta tarefa partiu.
	TSet<int32> Densidades;
	for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
	{
		Densidades.Add(
			FluidRegistry::TraitsOf(static_cast<EFluidKind>(Qual)).DensityPerMille);
	}

	if (Densidades.Num() < 4)
	{
		AddError(FString::Printf(
			TEXT("os fluidos produzem so %d densidades distintas — a tabela nao ")
			TEXT("separa nada"), Densidades.Num()));
		return false;
	}

	// E a ordem que importa é a do mundo real, porque ter referência de fora é
	// o que impede a tabela de virar gosto:
	const int32 Doce = FluidRegistry::TraitsOf(EFluidKind::AguaDoce).DensityPerMille;

	TestTrue(TEXT("a agua salgada e mais densa que a doce"),
		FluidRegistry::TraitsOf(EFluidKind::AguaSalgada).DensityPerMille > Doce);
	TestTrue(TEXT("a agua termal e menos densa que a fria"),
		FluidRegistry::TraitsOf(EFluidKind::AguaTermal).DensityPerMille < Doce);
	TestTrue(TEXT("a lama e mais densa que qualquer agua"),
		FluidRegistry::TraitsOf(EFluidKind::Lama).DensityPerMille
			> FluidRegistry::TraitsOf(EFluidKind::AguaDePantano).DensityPerMille);
	TestTrue(TEXT("a lava e o mais denso de todos"),
		FluidRegistry::TraitsOf(EFluidKind::Lava).DensityPerMille
			> FluidRegistry::TraitsOf(EFluidKind::Lama).DensityPerMille);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidRegistryFloatingDependsOnBothSidesTest,
	"BattleSim.FluidRegistry.FloatingDependsOnBothSides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidRegistryFloatingDependsOnBothSidesTest::RunTest(const FString& Parameters)
{
	// O QUE BOIA DEPENDE DOS DOIS LADOS, e é por isso que a densidade é um
	// número em vez de um booleano `bFloats` no corpo. A mesma balsa que boia
	// na água afunda... em nada, porque não há nada menos denso que ela aqui —
	// mas o mesmo corpo pode boiar num fluido e afundar noutro, e é essa a
	// pergunta que a tabela passa a saber responder.
	const int32 Doce = FluidRegistry::FreshWaterDensityPerMille();

	// Madeira: boia na água doce e na salgada.
	constexpr int32 Madeira = 600;
	TestTrue(TEXT("madeira boia na agua doce"),
		FluidRegistry::FloatsOn(Madeira, EFluidKind::AguaDoce));
	TestTrue(TEXT("madeira boia na agua salgada"),
		FluidRegistry::FloatsOn(Madeira, EFluidKind::AguaSalgada));

	// Pedra: afunda nas duas.
	constexpr int32 Pedra = 2700;
	TestFalse(TEXT("pedra afunda na agua doce"),
		FluidRegistry::FloatsOn(Pedra, EFluidKind::AguaDoce));

	// E O CASO QUE SÓ A DENSIDADE PEGA: a mesma pedra BOIA na lava, porque a
	// lava é mais densa que ela. Um booleano `bFloats` no corpo erraria isto
	// sempre, e erraria calado.
	TestTrue(TEXT("pedra boia na lava"),
		FluidRegistry::FloatsOn(Pedra, EFluidKind::Lava));

	// O EMPATE AFUNDA. Deixar o igual boiando faria um corpo de densidade
	// exatamente igual à da água flutuar para sempre — e esse é o caso que
	// mais aparece, porque 1000 é o número redondo que alguém escreve primeiro.
	TestFalse(TEXT("densidade igual a da agua nao boia"),
		FluidRegistry::FloatsOn(Doce, EFluidKind::AguaDoce));

	// Em casa SECA nada boia: sem fluido não há empuxo.
	TestFalse(TEXT("nada boia no seco"),
		FluidRegistry::FloatsOn(1, EFluidKind::Nenhum));

	return true;
}
