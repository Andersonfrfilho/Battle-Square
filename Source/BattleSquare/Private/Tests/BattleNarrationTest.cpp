// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleNarration.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"
#include "Internationalization/Text.h"

namespace
{
	FBattleEvent MakeEvent(EBattleEventType Type, uint8 Actor = 1, uint8 Target = 2, int32 Value = 0)
	{
		FBattleEvent Event;
		Event.Type = Type;
		Event.ActorId = Actor;
		Event.TargetId = Target;
		Event.Value = Value;
		return Event;
	}

	FString Describe(const FBattleEvent& Event)
	{
		return FBattleNarration::Describe(Event, TEXT("Faísca"), TEXT("Brisa")).ToString();
	}
}

// T1: cada desfecho que o jogador percebe vira uma frase que NOMEIA quem agiu,
// quem sofreu e quanto doeu. Frase sem o número do dano não ensina nada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNarrationDescribesOutcomesTest,
	"BattleSquare.Battle.BattleNarration.DescribesOutcomes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNarrationDescribesOutcomesTest::RunTest(const FString& Parameters)
{
	const FString Acertou = Describe(MakeEvent(EBattleEventType::AtaqueAcertou));
	TestTrue(TEXT("Acerto nomeia quem atacou"), Acertou.Contains(TEXT("Faísca")));
	TestTrue(TEXT("Acerto nomeia quem sofreu"), Acertou.Contains(TEXT("Brisa")));

	const FString Dano = Describe(MakeEvent(EBattleEventType::DanoAplicado, 1, 2, 12));
	TestTrue(TEXT("Dano mostra o número"), Dano.Contains(TEXT("12")));

	// Esquiva e defesa são o coração da lição: sem vê-las, o jogador não
	// conclui que atacar quem está evasivo é desperdício.
	TestTrue(TEXT("Esquiva é dita"),
		Describe(MakeEvent(EBattleEventType::Esquivou, 2, 1)).Contains(TEXT("esquiv")));
	TestTrue(TEXT("Defesa é dita"),
		Describe(MakeEvent(EBattleEventType::Defendeu, 2, 1)).Contains(TEXT("defend")));
	TestTrue(TEXT("Erro é dito"),
		Describe(MakeEvent(EBattleEventType::AtaqueErrou)).Contains(TEXT("err")));
	TestTrue(TEXT("Morte é dita"),
		Describe(MakeEvent(EBattleEventType::PetMorreu, 2, 2)).Contains(TEXT("Brisa")));
	TestTrue(TEXT("Bloqueio explica a borda"),
		Describe(MakeEvent(EBattleEventType::MovimentoBloqueado, 1, 1)).Contains(TEXT("Faísca")));

	return true;
}

// T1: evento que o jogador não precisa ler (contabilidade de slot/fase) sai
// VAZIO. Um feed que narra tudo não narra nada — a frase útil some no meio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNarrationStaysSilentOnBookkeepingTest,
	"BattleSquare.Battle.BattleNarration.StaysSilentOnBookkeeping",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNarrationStaysSilentOnBookkeepingTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("SlotIniciado é silencioso"),
		Describe(MakeEvent(EBattleEventType::SlotIniciado)).IsEmpty());
	TestTrue(TEXT("SlotEncerrado é silencioso"),
		Describe(MakeEvent(EBattleEventType::SlotEncerrado)).IsEmpty());

	// Nome ausente não pode produzir frase quebrada nem crash: o feed é a
	// última coisa que pode derrubar uma partida.
	const FString SemNome = FBattleNarration::Describe(
		MakeEvent(EBattleEventType::AtaqueAcertou), FString(), FString()).ToString();
	TestFalse(TEXT("Sem nome ainda produz frase"), SemNome.IsEmpty());

	return true;
}

// T1: o anel guarda as ÚLTIMAS linhas e descarta as antigas. Feed que cresce
// sem teto é exatamente o defeito que o painel de depuração já teve.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNarrationFeedKeepsRecentLinesTest,
	"BattleSquare.Battle.BattleNarration.FeedKeepsRecentLines",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNarrationFeedKeepsRecentLinesTest::RunTest(const FString& Parameters)
{
	FBattleNarrationFeed::Clear();
	TestTrue(TEXT("Começa vazio"), FBattleNarrationFeed::GetLines().IsEmpty());

	// Linha vazia é recusada: é assim que o silêncio da contabilidade não vira
	// linha em branco empurrando a frase útil para fora.
	FBattleNarrationFeed::Push(FText::GetEmpty(), FColor::White);
	TestTrue(TEXT("Linha vazia não entra"), FBattleNarrationFeed::GetLines().IsEmpty());

	const int32 Excesso = FBattleNarrationFeed::MaxLines + 3;
	for (int32 Index = 0; Index < Excesso; ++Index)
	{
		FBattleNarrationFeed::Push(
			FText::FromString(FString::Printf(TEXT("linha %d"), Index)), FColor::White);
	}

	const TArray<FBattleNarrationFeed::FLine>& Lines = FBattleNarrationFeed::GetLines();
	TestEqual(TEXT("Respeita o teto"), Lines.Num(), FBattleNarrationFeed::MaxLines);
	TestEqual(TEXT("A última é a mais recente"),
		Lines.Last().Text.ToString(), FString::Printf(TEXT("linha %d"), Excesso - 1));

	FBattleNarrationFeed::Clear();
	return true;
}

// A garantia de que a frase é COLETÁVEL não cabe aqui: a coleta lê o
// código-fonte, não o processo. Quem verifica isso é
// Tools/audit_localizable_text.sh — em runtime não existe propriedade que
// separe FText::Format de FString::Printf, e um teste que finge separar é
// pior que nenhum.
