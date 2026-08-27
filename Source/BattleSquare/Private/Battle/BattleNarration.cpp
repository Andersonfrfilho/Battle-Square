// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleNarration.h"

#include "Battle/BattleEvent.h"
#include "Battle/BattleState.h"

#define LOCTEXT_NAMESPACE "BattleNarration"

namespace
{
	TArray<FBattleNarrationFeed::FLine> GNarrationLines;

	/** Nome ausente não pode quebrar a frase nem derrubar a partida: o feed é
	 *  a última coisa do jogo que tem direito de falhar. */
	FText SafeName(const FString& Name, const FText& Fallback)
	{
		return Name.IsEmpty() ? Fallback : FText::FromString(Name);
	}
}

FText FBattleNarration::Describe(const FBattleEvent& Event, const FString& ActorName, const FString& TargetName)
{
	// Argumentos NOMEADOS, não posicionais: em alemão o objeto vem antes do
	// verbo, e {0}/{1} obrigariam o tradutor a reordenar o que não é dele.
	FFormatNamedArguments Args;
	Args.Add(TEXT("Actor"), SafeName(ActorName, LOCTEXT("PetSemNome", "O pet")));
	Args.Add(TEXT("Target"), SafeName(TargetName, LOCTEXT("AdversarioSemNome", "o adversário")));
	Args.Add(TEXT("Value"), FText::AsNumber(Event.Value));

	switch (Event.Type)
	{
	case EBattleEventType::AtaqueAcertou:
		return FText::Format(LOCTEXT("AtaqueAcertou", "{Actor} atacou {Target} e acertou"), Args);

	case EBattleEventType::AtaqueErrou:
		return FText::Format(LOCTEXT("AtaqueErrou", "{Actor} atacou {Target} e errou"), Args);

	case EBattleEventType::Esquivou:
		return FText::Format(LOCTEXT("Esquivou", "{Actor} esquivou"), Args);

	case EBattleEventType::Defendeu:
		return FText::Format(LOCTEXT("Defendeu", "{Actor} defendeu o golpe"), Args);

	case EBattleEventType::DanoAplicado:
		return FText::Format(LOCTEXT("DanoAplicado", "{Target} perdeu {Value} de vida"), Args);

	case EBattleEventType::PetMorreu:
		return FText::Format(LOCTEXT("PetMorreu", "{Target} foi derrotado"), Args);

	case EBattleEventType::MovimentoBloqueado:
		// Dizer só "bloqueado" deixa o jogador achando que foi bug. A borda é
		// a explicação, e é ela que ensina a não repetir a escolha.
		return FText::Format(LOCTEXT("MovimentoBloqueado", "{Actor} tentou andar e esbarrou no limite da arena"), Args);

	case EBattleEventType::PosturaAssumida:
		switch (static_cast<EBattlePostureFlags>(Event.Value))
		{
		case EBattlePostureFlags::Defending:
			return FText::Format(LOCTEXT("PosturaDefender", "{Actor} se defendeu"), Args);
		case EBattlePostureFlags::Dodging:
			return FText::Format(LOCTEXT("PosturaEsquivar", "{Actor} ficou evasivo"), Args);
		case EBattlePostureFlags::Camouflaged:
			return FText::Format(LOCTEXT("PosturaCamuflar", "{Actor} se camuflou e sumiu de vista"), Args);
		case EBattlePostureFlags::Flying:
			return FText::Format(LOCTEXT("PosturaVoar", "{Actor} alçou voo — fora do alcance físico, exposto à magia"), Args);
		case EBattlePostureFlags::Underground:
			return FText::Format(LOCTEXT("PosturaSubmergir", "{Actor} mergulhou no solo"), Args);
		default:
			return FText::Format(LOCTEXT("PosturaAssumida", "{Actor} assumiu postura"), Args);
		}

	case EBattleEventType::Moveu:
		return FText::Format(LOCTEXT("Moveu", "{Actor} se moveu"), Args);

	default:
		// Contabilidade de slot, turno e fim de batalha: quem anuncia isso é a
		// arena, com o contexto de quem venceu, que o evento sozinho não tem.
		return FText::GetEmpty();
	}
}

void FBattleNarrationFeed::Push(const FText& Text, const FColor& Color)
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

#undef LOCTEXT_NAMESPACE
