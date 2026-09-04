// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/OwnershipCache.h"
#include "Misc/AutomationTest.h"

/**
 * PS7 — a leitura da posse NUNCA impede jogar.
 *
 * A regra inteira desta feature, e a lembrança do dono ("offline é normal"),
 * cabe numa frase testável: falha de rede deixa o cache INTACTO. Estes testes
 * afirmam a diferença entre "não sei mais" e "não consegui perguntar agora".
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnershipFailedFetchKeepsTheCacheTest,
	"BattleSquare.Meta.Posse.FalhaNaoApagaOCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOwnershipFailedFetchKeepsTheCacheTest::RunTest(const FString&)
{
	// Uma busca boa carimba a posse e a hora.
	TArray<OwnershipCache::FKnownPet> DoServidor;
	DoServidor.Add({ TEXT("fire-drake-01"), false });
	DoServidor.Add({ TEXT("moss-turtle-02"), true });

	const OwnershipCache::FState Bom =
		OwnershipCache::AfterSuccessfulFetch(DoServidor, 100.0);
	TestEqual(TEXT("a busca boa guarda os dois"), Bom.Pets.Num(), 2);
	TestEqual(TEXT("e carimba a hora"), Bom.LastSyncSeconds, 100.0);

	// A FALHA deixa TUDO como estava — a posse não some porque o Wi-Fi caiu.
	const OwnershipCache::FState AposFalha = OwnershipCache::AfterFailedFetch(Bom);
	TestEqual(TEXT("a falha mantem os pets"), AposFalha.Pets.Num(), 2);
	TestEqual(TEXT("e nem envelhece a hora — falhar nao e novidade"),
		AposFalha.LastSyncSeconds, Bom.LastSyncSeconds);
	TestTrue(TEXT("o roubado continua marcado"), AposFalha.Pets[1].bStolen);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnershipServerIsTheTruthTest,
	"BattleSquare.Meta.Posse.OServidorEhAFonte",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOwnershipServerIsTheTruthTest::RunTest(const FString&)
{
	// O servidor manda no empate (decisão 38-b): a busca boa SUBSTITUI, não
	// funde — fundir manteria um pet que o servidor já tirou, que é o roubo
	// sobrevivendo à devolução.
	TArray<OwnershipCache::FKnownPet> Antigo;
	Antigo.Add({ TEXT("fire-drake-01"), false });
	Antigo.Add({ TEXT("stolen-ghost-09"), true });
	const OwnershipCache::FState Cache =
		OwnershipCache::AfterSuccessfulFetch(Antigo, 50.0);

	// O servidor devolve SÓ o drake — o fantasma roubado foi devolvido.
	TArray<OwnershipCache::FKnownPet> Novo;
	Novo.Add({ TEXT("fire-drake-01"), false });
	const OwnershipCache::FState Atualizado =
		OwnershipCache::AfterSuccessfulFetch(Novo, 60.0);

	TestEqual(TEXT("o cache novo tem so o que o servidor tem"),
		Atualizado.Pets.Num(), 1);
	TestEqual(TEXT("e e o drake"), Atualizado.Pets[0].CatalogId, FString(TEXT("fire-drake-01")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnershipStalenessTest,
	"BattleSquare.Meta.Posse.OQueEhVelhoEOQueEhAindaNao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOwnershipStalenessTest::RunTest(const FString&)
{
	TArray<OwnershipCache::FKnownPet> Pets;
	Pets.Add({ TEXT("fire-drake-01"), false });
	const OwnershipCache::FState Fresco =
		OwnershipCache::AfterSuccessfulFetch(Pets, 100.0);

	// Dentro do prazo: fresco. Passou: velho, e o jogo avisa (PS10).
	TestFalse(TEXT("recem-sincronizado nao e velho"),
		OwnershipCache::IsStale(Fresco, 120.0, /*StaleAfter=*/60.0));
	TestTrue(TEXT("passado o prazo, e velho"),
		OwnershipCache::IsStale(Fresco, 200.0, 60.0));

	// NUNCA sincronizado não é "velho": é "ainda não" — a distinção que
	// impede o aviso de piscar antes da primeira busca.
	const OwnershipCache::FState Novo;
	TestFalse(TEXT("nunca sincronizado nao conta como velho"),
		OwnershipCache::IsStale(Novo, 1000.0, 60.0));

	return true;
}
