// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

namespace
{
	// v1 é 1v1 (spec: mais de um pet por lado é M3) — o primeiro pet vivo
	// do lado é o único pet do lado. A busca por Side, em vez de índice
	// fixo, é o que deixa a fase pronta para N pets sem mudar assinatura.
	FPetState* FindAlivePetOnSide(FBattleState& State, uint8 Side)
	{
		for (FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side && Pet.IsAlive())
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	void ApplyPostureForSide(
		FBattleState& State,
		uint8 Side,
		const FBattleAction& Action,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace)
	{
		FPetState* Pet = FindAlivePetOnSide(State, Side);
		if (!Pet)
		{
			return;
		}

		uint8 AssumedFlag = 0;
		if (Action.Type == EActionType::Defender)
		{
			AssumedFlag = static_cast<uint8>(EBattlePostureFlags::Defending);
		}
		else if (Action.Type == EActionType::Esquivar)
		{
			AssumedFlag = static_cast<uint8>(EBattlePostureFlags::Dodging);
		}
		else
		{
			return; // Ação não é de postura — nada a fazer nesta fase.
		}

		Pet->PostureFlags |= AssumedFlag;

		FBattleEvent Event;
		Event.Type = EBattleEventType::PosturaAssumida;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 2; // F2
		Event.ActorId = Pet->PetId;
		Event.TargetId = BattleEventNoActor;
		OutTrace.Add(Event);
	}
}

void BattlePhases::ApplyPostures(
	FBattleState& State,
	const FBattleAction& LeftAction,
	const FBattleAction& RightAction,
	uint8 SlotIndex,
	TArray<FBattleEvent>& OutTrace)
{
	ApplyPostureForSide(State, /*Side=*/0, LeftAction, SlotIndex, OutTrace);
	ApplyPostureForSide(State, /*Side=*/1, RightAction, SlotIndex, OutTrace);
}
