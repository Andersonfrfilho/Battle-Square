// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

namespace
{
	struct FMoveIntent
	{
		FPetState* Pet = nullptr;
		int32 DestColumn = 0;
		int32 DestRow = 0;
		bool bInsideGrid = false;
	};

	void CollectIntent(FBattleState& State, uint8 Side, const FBattleAction& Action, TArray<FMoveIntent>& OutIntents)
	{
		if (Action.Type != EActionType::Mover)
		{
			return;
		}

		for (FPetState& Pet : State.Pets)
		{
			if (Pet.Side != Side || !Pet.IsAlive())
			{
				continue;
			}

			int8 DeltaColumn = 0;
			int8 DeltaRow = 0;
			GetDirectionDelta(Action.Direction, DeltaColumn, DeltaRow);

			const int32 DestColumn = static_cast<int32>(Pet.Column) + DeltaColumn;
			const int32 DestRow = static_cast<int32>(Pet.Row) + DeltaRow;

			FMoveIntent Intent;
			Intent.Pet = &Pet;
			Intent.DestColumn = DestColumn;
			Intent.DestRow = DestRow;
			Intent.bInsideGrid = IsInsideGrid(DestColumn, DestRow);
			OutIntents.Add(Intent);

			// v1 é 1v1 — um único pet vivo por lado, então este laço
			// encontra no máximo uma intenção por lado. Mantido como
			// busca (em vez de índice fixo) para generalizar sem mudar
			// assinatura quando M3 trouxer múltiplos pets por lado.
			break;
		}
	}

	void EmitMoved(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Pet, uint8 FromCell, uint8 ToCell)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::Moveu;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 3; // F3
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		Event.FromCell = FromCell;
		Event.ToCell = ToCell;
		OutTrace.Add(Event);
	}

	void EmitBlocked(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Pet)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::MovimentoBloqueado;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 3; // F3
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		Event.FromCell = PackCell(Pet.Column, Pet.Row);
		Event.ToCell = Event.FromCell; // Bloqueado: fica onde estava.
		OutTrace.Add(Event);
	}
}

void BattlePhases::ApplyMovement(
	FBattleState& State,
	const FBattleAction& LeftAction,
	const FBattleAction& RightAction,
	uint8 SlotIndex,
	TArray<FBattleEvent>& OutTrace)
{
	// Passo 1: coletar TODAS as intenções antes de aplicar qualquer uma —
	// é o que torna o movimento simultâneo de verdade (design.md).
	TArray<FMoveIntent> Intents;
	CollectIntent(State, /*Side=*/0, LeftAction, Intents);
	CollectIntent(State, /*Side=*/1, RightAction, Intents);

	if (Intents.IsEmpty())
	{
		return;
	}

	// Passo 2: fora da grade é bloqueio individual, resolvido já aqui —
	// não compete por destino com mais ninguém.
	TArray<FMoveIntent> ValidIntents;
	for (const FMoveIntent& Intent : Intents)
	{
		if (Intent.bInsideGrid)
		{
			ValidIntents.Add(Intent);
		}
		else
		{
			EmitBlocked(OutTrace, SlotIndex, *Intent.Pet);
		}
	}

	// Passo 3: colisão entre aliados. Dois+ pets do MESMO lado disputando
	// a mesma casa de destino são todos anulados (BTL-05). Lados opostos
	// coabitam livremente (DP-02) — não entram nesta contagem.
	TMap<uint32, TArray<int32>> DestinationClaimsBySide; // (side<<16 | col<<8 | row) -> índices em ValidIntents
	for (int32 Index = 0; Index < ValidIntents.Num(); ++Index)
	{
		const FMoveIntent& Intent = ValidIntents[Index];
		const uint32 Key = (static_cast<uint32>(Intent.Pet->Side) << 16)
			| (static_cast<uint32>(Intent.DestColumn) << 8)
			| static_cast<uint32>(Intent.DestRow);
		DestinationClaimsBySide.FindOrAdd(Key).Add(Index);
	}

	for (const auto& ClaimPair : DestinationClaimsBySide)
	{
		const TArray<int32>& ClaimantIndices = ClaimPair.Value;
		const bool bCollides = ClaimantIndices.Num() > 1;

		for (int32 Index : ClaimantIndices)
		{
			const FMoveIntent& Intent = ValidIntents[Index];
			if (bCollides)
			{
				EmitBlocked(OutTrace, SlotIndex, *Intent.Pet);
			}
			else
			{
				const uint8 FromCell = PackCell(Intent.Pet->Column, Intent.Pet->Row);
				Intent.Pet->Column = static_cast<uint8>(Intent.DestColumn);
				Intent.Pet->Row = static_cast<uint8>(Intent.DestRow);
				const uint8 ToCell = PackCell(Intent.Pet->Column, Intent.Pet->Row);
				EmitMoved(OutTrace, SlotIndex, *Intent.Pet, FromCell, ToCell);
			}
		}
	}
}
