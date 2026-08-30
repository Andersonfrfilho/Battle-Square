// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/WorldLoadingScreen.h"

// A tela SAI quando tudo fica pronto, e não antes.
//
// O critério é o ESTADO, não o relógio. Uma tela de tempo fixo esconderia a
// espera real e mostraria uma falsa — e o jogador continuaria caindo num mundo
// pela metade, só que sem saber que caiu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoadingEndsOnStateNotOnTimeTest,
	"BattleSquare.UI.WorldLoading.EndsOnStateNotOnTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLoadingEndsOnStateNotOnTimeTest::RunTest(const FString& Parameters)
{
	FWorldLoadingProgress Progresso;
	TestFalse(TEXT("Nada pronto: não terminou"), Progresso.IsEverythingReady());

	Progresso.bTypeCatalogReady = true;
	Progresso.bMirrorVerified = true;
	Progresso.bSceneryBuilt = true;
	TestFalse(TEXT("Três de quatro ainda NÃO terminou"), Progresso.IsEverythingReady());

	Progresso.bEncountersPlaced = true;
	TestTrue(TEXT("Os quatro: terminou"), Progresso.IsEverythingReady());

	return true;
}

// O passo ATUAL é o que a tela diz, e cada etapa tem frase própria.
//
// "Carregando…" não diz se a coisa anda. Se travar, o jogador precisa saber
// EM QUÊ travou — e é a mesma informação que serve para depurar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoadingNamesTheStepItIsOnTest,
	"BattleSquare.UI.WorldLoading.NamesTheStepItIsOn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLoadingNamesTheStepItIsOnTest::RunTest(const FString& Parameters)
{
	FWorldLoadingProgress Progresso;

	const FString Primeiro = Progresso.DescribeCurrentStep().ToString();
	Progresso.bTypeCatalogReady = true;
	const FString Segundo = Progresso.DescribeCurrentStep().ToString();
	Progresso.bMirrorVerified = true;
	const FString Terceiro = Progresso.DescribeCurrentStep().ToString();
	Progresso.bSceneryBuilt = true;
	const FString Quarto = Progresso.DescribeCurrentStep().ToString();

	TestNotEqual(TEXT("O passo 1 difere do 2"), Primeiro, Segundo);
	TestNotEqual(TEXT("O passo 2 difere do 3"), Segundo, Terceiro);
	TestNotEqual(TEXT("O passo 3 difere do 4"), Terceiro, Quarto);

	// E a porcentagem acompanha, em vez de ficar parada.
	TestEqual(TEXT("Três de quatro é 75%"), Progresso.PercentComplete(), 75);

	return true;
}

// FALHA PERMANENTE é dita, não girada para sempre.
//
// Carregamento que nunca termina e nunca explica é o pior dos dois mundos: o
// jogador não joga e não sabe por quê. Este projeto já gastou rodadas com
// recusa silenciosa parecendo defeito — e ali era só a XP.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoadingSaysWhyItFailedTest,
	"BattleSquare.UI.WorldLoading.SaysWhyItFailed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLoadingSaysWhyItFailedTest::RunTest(const FString& Parameters)
{
	FWorldLoadingProgress Falhou;
	Falhou.PermanentProblem = TEXT("WorldEncounterMirrorPath não configurado");

	TestTrue(TEXT("Está em falha"), Falhou.HasFailed());
	TestTrue(TEXT("E a frase carrega o motivo"),
		Falhou.DescribeCurrentStep().ToString().Contains(TEXT("não configurado")));

	// Falha NÃO é "pronto": a tela não pode sair por ter desistido.
	TestFalse(TEXT("Falha não conta como pronto"), Falhou.IsEverythingReady());

	return true;
}
