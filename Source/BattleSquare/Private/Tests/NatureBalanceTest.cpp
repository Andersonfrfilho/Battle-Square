// Copyright 2026 Anderson. All Rights Reserved.

#include "World/NatureBalance.h"
#include "Misc/AutomationTest.h"

/**
 * mae-natureza MN2/MN3/MN4 — o corretor puro (censo entra, correcao sai), a
 * delacao obrigatoria, e a torneira nunca virando balde.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureBalanceCorrectorTest,
	"BattleSquare.World.MaeNatureza.CorretorPuroEDelatado",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNatureBalanceCorrectorTest::RunTest(const FString&)
{
	using namespace NatureBalance;

	FNatureFaixaAlvo Faixa; Faixa.Min = 10.0f; Faixa.Max = 20.0f;

	// ABAIXO do minimo: abre a torneira (ajuste positivo ate o minimo).
	FNatureCenso Falta; Falta.Tap = ENatureTap::RegrowthDeadline; Falta.CurrentLevel = 4.0f;
	const FNatureCorrecao CorrFalta = Correct(Falta, Faixa);
	TestTrue(TEXT("falta abre a torneira"), CorrFalta.TapAdjustment > 0.0f);
	TestEqual(TEXT("abre exatamente ate o minimo"), CorrFalta.TapAdjustment, 6.0f);

	// ACIMA do maximo: fecha (ajuste negativo ate o maximo).
	FNatureCenso Sobra; Sobra.CurrentLevel = 30.0f;
	const FNatureCorrecao CorrSobra = Correct(Sobra, Faixa);
	TestTrue(TEXT("sobra fecha a torneira"), CorrSobra.TapAdjustment < 0.0f);
	TestEqual(TEXT("fecha exatamente ate o maximo"), CorrSobra.TapAdjustment, -10.0f);

	// DENTRO da faixa: nao mexe.
	FNatureCenso Saudavel; Saudavel.CurrentLevel = 15.0f;
	TestTrue(TEXT("dentro da faixa, correcao zero"), Correct(Saudavel, Faixa).IsZero());

	// Faixa invertida: sem alvo, nao mexe.
	FNatureFaixaAlvo Invertida; Invertida.Min = 20.0f; Invertida.Max = 10.0f;
	TestTrue(TEXT("faixa invertida nao gera ajuste doido"),
		Correct(Falta, Invertida).IsZero());

	// MN3 — DELACAO OBRIGATORIA: correcao nao-nula grava linha; nula nao grava.
	TArray<FNatureLogEntry> Registro;
	TestTrue(TEXT("correcao nao-nula e delatada"),
		ApplyAndLog(CorrFalta, Falta.CurrentLevel, Registro));
	TestEqual(TEXT("gravou uma linha"), Registro.Num(), 1);
	TestEqual(TEXT("a linha diz de onde veio"), Registro[0].FromLevel, 4.0f);
	TestEqual(TEXT("e quanto girou"), Registro[0].Adjustment, 6.0f);

	// Correcao nula NAO grava — e o unico caminho de aplicacao, entao nao ha
	// aplicar sem delatar: aplicar o zero simplesmente nao registra nada.
	const int32 Antes = Registro.Num();
	TestFalse(TEXT("correcao nula nao registra"),
		ApplyAndLog(Correct(Saudavel, Faixa), Saudavel.CurrentLevel, Registro));
	TestEqual(TEXT("o registro nao cresceu com o zero"), Registro.Num(), Antes);

	return true;
}
