// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleTracePlayer.h"

TArray<TArray<FBattleEvent>> GroupBattleEventsByPhase(const TArray<FBattleEvent>& Trace)
{
	TArray<TArray<FBattleEvent>> Groups;

	// Chave "nenhum grupo aberto ainda" — usa um par que nenhum evento
	// real produz simultaneamente por convenção (SlotIndex=0xFF nunca é
	// emitido pelo núcleo, que só usa 0..2).
	uint8 CurrentSlot = 0xFF;
	uint8 CurrentPhase = 0xFF;
	bool bHasOpenGroup = false;

	for (const FBattleEvent& Event : Trace)
	{
		const bool bStartsNewGroup = !bHasOpenGroup || Event.SlotIndex != CurrentSlot || Event.Phase != CurrentPhase;

		if (bStartsNewGroup)
		{
			Groups.AddDefaulted();
			CurrentSlot = Event.SlotIndex;
			CurrentPhase = Event.Phase;
			bHasOpenGroup = true;
		}

		Groups.Last().Add(Event);
	}

	return Groups;
}

void UBattleTracePlayer::PlayTrace(const TArray<FBattleEvent>& Trace)
{
	const TArray<TArray<FBattleEvent>> Groups = GroupBattleEventsByPhase(Trace);
	for (const TArray<FBattleEvent>& Group : Groups)
	{
		for (const FBattleEvent& Event : Group)
		{
			OnEventApplied.Broadcast(Event);
		}
	}
}

void UBattleTracePlayer::SkipToEnd(const TArray<FBattleEvent>& Trace)
{
	for (const FBattleEvent& Event : Trace)
	{
		OnEventApplied.Broadcast(Event);
	}
}

void UBattleTracePlayer::StartPlayback(const TArray<FBattleEvent>& Trace)
{
	PendingGroups = GroupBattleEventsByPhase(Trace);
	NextGroupIndex = 0;
	// O primeiro grupo sai NA HORA: esperar um intervalo antes de qualquer
	// coisa acontecer faria o commit parecer que não registrou.
	SecondsSinceLastGroup = SecondsPerGroup;
	Advance(0.0f);
}

bool UBattleTracePlayer::Advance(float DeltaSeconds)
{
	SecondsSinceLastGroup += DeltaSeconds;

	while (PendingGroups.IsValidIndex(NextGroupIndex) && SecondsSinceLastGroup >= SecondsPerGroup)
	{
		for (const FBattleEvent& Event : PendingGroups[NextGroupIndex])
		{
			OnEventApplied.Broadcast(Event);
		}
		++NextGroupIndex;
		SecondsSinceLastGroup -= SecondsPerGroup;
	}

	return IsPlaying();
}
