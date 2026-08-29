// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

namespace
{
	// v1 é 1v1 (spec: mais de um pet por lado é M3) — o primeiro pet vivo
	// do lado é o único pet do lado. A busca por Side, em vez de índice
	// fixo, é o que deixa a fase pronta para N pets sem mudar assinatura.

	void ApplyPostureForSide(
		FBattleState& State,
		uint8 Side,
		const FBattleAction& Action,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace)
	{
		FPetState* Pet = State.FindAlivePetOnSide(Side);
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
		else if (Action.Type == EActionType::Camuflar)
		{
			AssumedFlag = static_cast<uint8>(EBattlePostureFlags::Camouflaged);
		}
		else if (Action.Type == EActionType::Voar)
		{
			AssumedFlag = static_cast<uint8>(EBattlePostureFlags::Flying);
		}
		else if (Action.Type == EActionType::Submergir)
		{
			AssumedFlag = static_cast<uint8>(EBattlePostureFlags::Underground);
		}
		else
		{
			return; // Ação não é de postura — nada a fazer nesta fase.
		}

		// Submergir exige ÁGUA. É a única postura condicionada a terreno:
		// condicionar as demais mudaria o combate inteiro, e ninguém pediu
		// isso — a skill é que descreve estar DENTRO da água.
		if (Action.Type == EActionType::Submergir
			&& State.CellLayout[CellLayoutIndex(Pet->Column, Pet->Row)] != static_cast<uint8>(ECellProperty::Water))
		{
			// Falha ALTA, com evento próprio. Silenciosamente virar Aguardar
			// deixaria o jogador achando que a skill não funciona, quando o
			// que faltava era ele estar na casa certa.
			FBattleEvent Falha;
			Falha.Type = EBattleEventType::PosturaFalhou;
			Falha.SlotIndex = SlotIndex;
			Falha.Phase = 2;
			Falha.ActorId = Pet->PetId;
			Falha.TargetId = BattleEventNoActor;
			Falha.Value = static_cast<int32>(AssumedFlag);
			OutTrace.Add(Falha);
			return;
		}

		Pet->PostureFlags |= AssumedFlag;

		FBattleEvent Event;
		Event.Type = EBattleEventType::PosturaAssumida;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 2; // F2
		Event.ActorId = Pet->PetId;
		Event.TargetId = BattleEventNoActor;

		// QUAL postura, e não só "assumiu alguma". Sem isto a tela diria
		// "assumiu postura" para camuflar, voar e submergir igualmente — e o
		// jogador não teria como aprender que magia fura camuflagem.
		Event.Value = static_cast<int32>(AssumedFlag);
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
