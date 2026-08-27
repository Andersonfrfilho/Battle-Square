// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleTracePlayer.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTracePlayerTimedPlaybackTest,
	"BattleSquare.TracePlayer.TimedPlaybackRevealsOneGroupAtATime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTracePlayerTimedPlaybackTest::RunTest(const FString& Parameters)
{
	UBattleTracePlayer* Player = NewObject<UBattleTracePlayer>();
	Player->SecondsPerGroup = 1.0f;

	// Três fases distintas: sem tempo, as três sairiam no mesmo frame — que
	// era exatamente o defeito ("fiz três movimentos e ele fez tudo de uma vez").
	TArray<FBattleEvent> Trace;
	for (int32 Phase = 0; Phase < 3; ++Phase)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DanoAplicado;
		Event.Phase = static_cast<uint8>(Phase);
		Event.ActorId = 1;
		Event.TargetId = 2;
		Event.Value = 1;
		Trace.Add(Event);
	}

	int32 Aplicados = 0;
	Player->OnEventApplied.AddLambda([&Aplicados](const FBattleEvent&) { ++Aplicados; });

	Player->StartPlayback(Trace);
	TestEqual(TEXT("o primeiro grupo sai na hora — commit tem de dar retorno imediato"), Aplicados, 1);

	Player->Advance(0.5f);
	TestEqual(TEXT("meio intervalo não revela o grupo seguinte"), Aplicados, 1);

	Player->Advance(0.5f);
	TestEqual(TEXT("completado o intervalo, sai o segundo"), Aplicados, 2);

	TestTrue(TEXT("ainda há grupo por mostrar"), Player->IsPlaying());
	Player->Advance(1.0f);
	TestEqual(TEXT("e o terceiro"), Aplicados, 3);
	TestFalse(TEXT("terminou"), Player->IsPlaying());

	return true;
}
