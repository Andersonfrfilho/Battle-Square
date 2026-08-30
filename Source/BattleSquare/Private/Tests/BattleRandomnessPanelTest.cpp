// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Misc/AutomationTest.h"

// A semente é o que falta para REPRODUZIR uma partida: commits já apareciam no
// painel todo turno, mas sem o número que iniciou o gerador nada disso volta a
// acontecer igual. Ela sai em hexadecimal porque é para copiar, não para ler.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPainelDoAcasoMostraASementeTest,
	"BattleSquare.Battle.PainelDoAcaso.MostraASemente",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPainelDoAcasoMostraASementeTest::RunTest(const FString& Parameters)
{
	const FString Linha = ABattleArena::FormatSeedPanelLine(0xABCDEF0123456789ULL);

	TestTrue(TEXT("a linha diz que aquilo é a semente"), Linha.Contains(TEXT("semente")));
	TestTrue(TEXT("e carrega o número inteiro, em hexadecimal"),
		Linha.Contains(TEXT("ABCDEF0123456789")));

	// Semente pequena não pode sair encurtada: 0x...0007 e 0x...0070 são
	// partidas diferentes, e um zero à esquerda perdido as confunde.
	TestTrue(TEXT("semente pequena mantém os 16 dígitos"),
		ABattleArena::FormatSeedPanelLine(7ULL).Contains(TEXT("0000000000000007")));

	return true;
}

// "Esse resultado estranho foi sorteio ou foi regra?" é a pergunta que o painel
// não respondia. O gerador anda um passo por sorteio; comparar o estado antes e
// depois do turno responde sem precisar contar quantos passos foram.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPainelDoAcasoDizSeSorteouTest,
	"BattleSquare.Battle.PainelDoAcaso.DizSeSorteou",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPainelDoAcasoDizSeSorteouTest::RunTest(const FString& Parameters)
{
	const FString Andou = ABattleArena::FormatRandomPanelLine(100ULL, 200ULL);
	TestTrue(TEXT("estado que mudou é anunciado como sorteio"), Andou.Contains(TEXT("SORTEOU")));
	TestTrue(TEXT("e mostra o estado depois, não o de antes"), Andou.Contains(TEXT("00000000000000C8")));

	const FString Parado = ABattleArena::FormatRandomPanelLine(100ULL, 100ULL);
	TestTrue(TEXT("estado parado é anunciado como turno sem sorteio"),
		Parado.Contains(TEXT("nenhum sorteio")));
	TestFalse(TEXT("e não pode dizer as duas coisas"), Parado.Contains(TEXT("SORTEOU")));

	return true;
}
