#include "Misc/AutomationTest.h"

#include "World/IslandBakedPlan.h"

/**
 * A GUARDA contra o assado velho.
 *
 * O modo de falhar é silencioso: alguém muda um parâmetro, o assado continua
 * respondendo, e o mundo passa a ser de uma configuração que não existe mais.
 * Um aviso mudo aqui seria pior que nenhum — por isso o que se cobra não é só
 * que a guarda recuse, é que ela NOMEIE o parâmetro.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanGuardNamesTheChangedParameterTest,
	"BattleSquare.IslandBakedPlanGuard.NamesTheChangedParameter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanGuardNamesTheChangedParameterTest::RunTest(const FString& Parameters)
{
	const FIslandParameters Agora = IslandBakedPlan::GatherParameters();

	// Um assado VELHO: o raio da ilha era outro quando ele foi feito.
	FIslandParameters Velho = Agora;
	Velho.LandRadiusUnits = Agora.LandRadiusUnits + 1000.0f;

	const TArray<FString> Divergiram =
		IslandBakedPlan::DescribeParameterDivergence(Velho, Agora);

	TestEqual(TEXT("um parametro divergiu"), Divergiram.Num(), 1);
	if (Divergiram.Num() != 1)
	{
		return false;
	}

	// Nomear é o requisito. "Algo mudou" manda a pessoa procurar sozinha.
	TestTrue(TEXT("a mensagem nomeia o parametro"),
		Divergiram[0].Contains(TEXT("LandRadiusUnits")));

	// E diz os DOIS valores: só o nome não distingue "mudou de 3 para 4" de
	// "mudou de 3 para 30000", e é a diferença que diz se foi engano.
	TestTrue(TEXT("a mensagem diz o valor do assado"),
		Divergiram[0].Contains(TEXT("assado")));
	TestTrue(TEXT("a mensagem diz o valor de agora"),
		Divergiram[0].Contains(TEXT("agora")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanGuardNamesEveryChangedParameterTest,
	"BattleSquare.IslandBakedPlanGuard.NamesEveryChangedParameter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanGuardNamesEveryChangedParameterTest::RunTest(const FString& Parameters)
{
	const FIslandParameters Agora = IslandBakedPlan::GatherParameters();

	// Dois parâmetros mudaram. Nomear só o primeiro faria a pessoa reassar,
	// bater de novo no segundo, e voltar aqui — uma rodada por parâmetro.
	FIslandParameters Velho = Agora;
	Velho.LandRadiusUnits = Agora.LandRadiusUnits + 1000.0f;
	Velho.GroveCount = Agora.GroveCount + 7;

	const TArray<FString> Divergiram =
		IslandBakedPlan::DescribeParameterDivergence(Velho, Agora);

	TestEqual(TEXT("os dois parametros divergiram"), Divergiram.Num(), 2);

	const FString Tudo = FString::Join(Divergiram, TEXT("; "));
	TestTrue(TEXT("nomeia o raio"), Tudo.Contains(TEXT("LandRadiusUnits")));
	TestTrue(TEXT("nomeia a contagem de bosques"), Tudo.Contains(TEXT("GroveCount")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanGuardIsSilentWhenNothingChangedTest,
	"BattleSquare.IslandBakedPlanGuard.IsSilentWhenNothingChanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanGuardIsSilentWhenNothingChangedTest::RunTest(const FString& Parameters)
{
	// Guarda que acusa divergencia toda vez e uma guarda ignorada — foi
	// exatamente o que o resumo por `FName` produzia antes de ser consertado.
	const FIslandParameters Agora = IslandBakedPlan::GatherParameters();

	TestEqual(TEXT("nada divergiu"),
		IslandBakedPlan::DescribeParameterDivergence(Agora, Agora).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakedPlanGuardLetsTheCurrentBakeThroughTest,
	"BattleSquare.IslandBakedPlanGuard.LetsTheCurrentBakeThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakedPlanGuardLetsTheCurrentBakeThroughTest::RunTest(const FString& Parameters)
{
	// O assado gravado é o de agora, então a porta do mundo tem de abrir.
	// Sem esta prova, a guarda poderia recusar sempre e ninguém notaria até o
	// mundo subir vazio.
	const UIslandBakedPlan* Assado = IslandBakedPlan::LoadForWorld();

	if (!Assado)
	{
		AddError(TEXT("a guarda recusou o assado vigente — reasse com ")
			TEXT("./Tools/bake_island.sh e, se persistir, o defeito e da guarda"));
		return false;
	}

	TestTrue(TEXT("o assado que passou tem relevo"), Assado->GroundHeightUnits.Num() > 0);

	return true;
}
