// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleNarration.h"

#include "Battle/BattleEvent.h"

namespace
{
	TArray<FBattleNarrationFeed::FLine> GNarrationLines;

	/** Nome ausente não pode quebrar a frase nem derrubar a partida: o feed é
	 *  a última coisa do jogo que tem direito de falhar. */
	FString SafeName(const FString& Name, const TCHAR* Fallback)
	{
		return Name.IsEmpty() ? FString(Fallback) : Name;
	}
}

FString FBattleNarration::Describe(const FBattleEvent& Event, const FString& ActorName, const FString& TargetName)
{
	const FString Actor = SafeName(ActorName, TEXT("O pet"));
	const FString Target = SafeName(TargetName, TEXT("o adversário"));

	switch (Event.Type)
	{
	case EBattleEventType::AtaqueAcertou:
		return FString::Printf(TEXT("%s atacou %s e acertou"), *Actor, *Target);

	case EBattleEventType::AtaqueErrou:
		return FString::Printf(TEXT("%s atacou %s e errou"), *Actor, *Target);

	case EBattleEventType::Esquivou:
		return FString::Printf(TEXT("%s esquivou"), *Actor);

	case EBattleEventType::Defendeu:
		return FString::Printf(TEXT("%s defendeu o golpe"), *Actor);

	case EBattleEventType::DanoAplicado:
		return FString::Printf(TEXT("%s perdeu %d de vida"), *Target, Event.Value);

	case EBattleEventType::PetMorreu:
		return FString::Printf(TEXT("%s foi derrotado"), *Target);

	case EBattleEventType::MovimentoBloqueado:
		// Dizer só "bloqueado" deixa o jogador achando que foi bug. A borda é
		// a explicação, e é ela que ensina a não repetir a escolha.
		return FString::Printf(TEXT("%s tentou andar e esbarrou no limite da arena"), *Actor);

	case EBattleEventType::PosturaAssumida:
		return FString::Printf(TEXT("%s assumiu postura"), *Actor);

	case EBattleEventType::Moveu:
		return FString::Printf(TEXT("%s se moveu"), *Actor);

	default:
		// Contabilidade de slot, turno e fim de batalha: quem anuncia isso é a
		// arena, com o contexto de quem venceu, que o evento sozinho não tem.
		return FString();
	}
}

void FBattleNarrationFeed::Push(const FString& Text, const FColor& Color)
{
	if (Text.IsEmpty())
	{
		return;
	}

	GNarrationLines.Add(FLine{ Text, Color });

	while (GNarrationLines.Num() > MaxLines)
	{
		GNarrationLines.RemoveAt(0);
	}
}

void FBattleNarrationFeed::Clear()
{
	GNarrationLines.Reset();
}

const TArray<FBattleNarrationFeed::FLine>& FBattleNarrationFeed::GetLines()
{
	return GNarrationLines;
}
