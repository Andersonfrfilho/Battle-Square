// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/CaptureQueueRules.h"
#include "Misc/AutomationTest.h"

/**
 * PS8 — a captura sobe UMA VEZ SÓ, e o que desiste APARECE.
 *
 * A fila é o efeito externo da captura, separado da captura em si (que já
 * aconteceu local). Estes testes afirmam as três garantias da task: não perde,
 * não duplica, e não acumula calado.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaptureQueueNoDuplicatesTest,
	"BattleSquare.Meta.FilaCaptura.NaoEnfileiraDuasVezes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureQueueNoDuplicatesTest::RunTest(const FString&)
{
	TArray<FPendingCapture> Fila;

	CaptureQueueRules::Enqueue(Fila, TEXT("fire-drake-01"), TEXT("conta:fire-drake-01"));
	// A MESMA captura de novo (recapturou antes de a primeira subir): a fila
	// não cresce — a chave de idempotência é por (conta, catálogo).
	CaptureQueueRules::Enqueue(Fila, TEXT("fire-drake-01"), TEXT("conta:fire-drake-01"));

	TestEqual(TEXT("a mesma captura e UMA linha"), Fila.Num(), 1);

	CaptureQueueRules::Enqueue(Fila, TEXT("moss-turtle-02"), TEXT("conta:moss-turtle-02"));
	TestEqual(TEXT("outra captura e outra linha"), Fila.Num(), 2);

	// Subiu: sai da fila.
	CaptureQueueRules::MarkSent(Fila, TEXT("fire-drake-01"));
	TestEqual(TEXT("a que subiu saiu"), Fila.Num(), 1);
	TestEqual(TEXT("e sobrou a certa"), Fila[0].CatalogId, FString(TEXT("moss-turtle-02")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaptureQueueRetriesWithACeilingTest,
	"BattleSquare.Meta.FilaCaptura.RetentaComTetoEDepoisAparece",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureQueueRetriesWithACeilingTest::RunTest(const FString&)
{
	TArray<FPendingCapture> Fila;
	CaptureQueueRules::Enqueue(Fila, TEXT("fire-drake-01"), TEXT("k"));

	// Falha antes do teto: ainda vale tentar, e continua enviável.
	for (int32 Tentativa = 1; Tentativa < CaptureQueueRules::MaxAttempts; ++Tentativa)
	{
		TestTrue(TEXT("dentro do teto, ainda ha tentativa"),
			CaptureQueueRules::RegisterAttempt(Fila, TEXT("fire-drake-01")));
	}
	TestEqual(TEXT("antes do teto, segue enviavel"),
		CaptureQueueRules::Sendable(Fila).Num(), 1);
	TestEqual(TEXT("e nada abandonado ainda"),
		CaptureQueueRules::Exhausted(Fila).Num(), 0);

	// A tentativa que ESTOURA o teto: para de valer.
	TestFalse(TEXT("no teto, nao ha mais tentativa"),
		CaptureQueueRules::RegisterAttempt(Fila, TEXT("fire-drake-01")));

	// E aí ela DESAPARECE dos enviáveis e APARECE nos abandonados — o
	// contrapeso da task: progresso perdido não fica calado.
	TestEqual(TEXT("nao e mais enviavel"), CaptureQueueRules::Sendable(Fila).Num(), 0);
	TestEqual(TEXT("mas APARECE como abandonada"),
		CaptureQueueRules::Exhausted(Fila).Num(), 1);

	// A abandonada CONTINUA na fila (não some): PS10 tem de poder mostrá-la.
	TestEqual(TEXT("e continua na fila, para ser mostrada"), Fila.Num(), 1);

	return true;
}
