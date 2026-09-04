// Copyright 2026 Anderson. All Rights Reserved.

#include "World/NatureBalance.h"
#include "Misc/AutomationTest.h"

/**
 * mae-natureza MN6 — a especie rara migra quando o censo cai: o censo REAL
 * (contagem agregada, do servidor) alimenta NatureBalance::Correct, e uma
 * especie abaixo da faixa-alvo produz uma correcao de migracao (peso de encontro
 * SOBE onde falta, DESCE onde sobra). Reusa o corretor de mae-natureza, nao um
 * segundo.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpeciesMigrationTest,
	"BattleSquare.World.MaeNatureza.EspecieRaraMigra",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpeciesMigrationTest::RunTest(const FString&)
{
	using namespace NatureBalance;

	// A faixa-alvo por regiao (config MN6).
	FNatureFaixaAlvo Faixa; Faixa.Min = 3.0f; Faixa.Max = 20.0f;

	// ESPECIE RARA (censo agregado real = 1, abaixo do minimo): migra para
	// dentro — o peso de encontro SOBE (ajuste positivo).
	FNatureCenso Rara; Rara.Tap = ENatureTap::EncounterWeight; Rara.CurrentLevel = 1.0f;
	const FNatureCorrecao MigraParaDentro = Correct(Rara, Faixa);
	TestTrue(TEXT("especie rara migra para dentro (peso sobe)"),
		MigraParaDentro.TapAdjustment > 0.0f);
	TestTrue(TEXT("a torneira e o peso de encontro"),
		MigraParaDentro.Tap == ENatureTap::EncounterWeight);

	// ESPECIE ABUNDANTE (censo = 50, acima do maximo): migra para fora — o peso
	// DESCE (ajuste negativo).
	FNatureCenso Abundante; Abundante.Tap = ENatureTap::EncounterWeight; Abundante.CurrentLevel = 50.0f;
	TestTrue(TEXT("especie abundante migra para fora (peso desce)"),
		Correct(Abundante, Faixa).TapAdjustment < 0.0f);

	// ESPECIE SAUDAVEL (dentro da faixa): nao migra — Mae Natureza nao mexe.
	FNatureCenso Saudavel; Saudavel.Tap = ENatureTap::EncounterWeight; Saudavel.CurrentLevel = 10.0f;
	TestTrue(TEXT("especie na faixa nao migra"), Correct(Saudavel, Faixa).IsZero());

	// E a migracao e DELATADA (MN3), como toda correcao.
	TArray<FNatureLogEntry> Registro;
	TestTrue(TEXT("a migracao e delatada"),
		ApplyAndLog(MigraParaDentro, Rara.CurrentLevel, Registro));
	TestEqual(TEXT("gravou a migracao"), Registro.Num(), 1);

	return true;
}
