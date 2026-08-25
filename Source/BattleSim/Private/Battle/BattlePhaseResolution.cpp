// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

namespace
{
	void EmitDamageApplied(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Pet, int32 Damage)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DanoAplicado;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 5; // F5
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		Event.Value = Damage;
		OutTrace.Add(Event);
	}

	void EmitPetDied(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Pet)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::PetMorreu;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 5; // F5
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		OutTrace.Add(Event);
	}

	void EmitSlotEnded(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::SlotEncerrado;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 5; // F5
		Event.ActorId = BattleEventNoActor;
		Event.TargetId = BattleEventNoActor;
		OutTrace.Add(Event);
	}
}

void BattlePhases::ApplyResolution(
	FBattleState& State,
	uint8 SlotIndex,
	TArray<FBattleEvent>& OutTrace)
{
	// Passo 1: TODO o dano acumulado em F4 é aplicado agora, de uma vez,
	// ANTES de qualquer checagem de morte (BTL-07) — é isto que garante
	// que dois pets que se matam no mesmo slot morrem os dois: nenhum
	// "morre primeiro" e escapa de revidar.
	TArray<bool> WasAliveBeforeDamage;
	WasAliveBeforeDamage.Reserve(State.Pets.Num());

	for (FPetState& Pet : State.Pets)
	{
		WasAliveBeforeDamage.Add(Pet.IsAlive());

		if (Pet.PendingDamage > 0)
		{
			Pet.Health -= Pet.PendingDamage;
			EmitDamageApplied(OutTrace, SlotIndex, Pet, Pet.PendingDamage);
			Pet.PendingDamage = 0;
		}
	}

	// Passo 2: checagem de morte, só depois de TODO dano aplicado.
	// PetMorreu emitido apenas na transição vivo -> morto deste slot —
	// um pet já morto em slot anterior não gera evento repetido.
	for (int32 Index = 0; Index < State.Pets.Num(); ++Index)
	{
		FPetState& Pet = State.Pets[Index];
		const bool bDiedThisSlot = WasAliveBeforeDamage[Index] && !Pet.IsAlive();
		if (bDiedThisSlot)
		{
			EmitPetDied(OutTrace, SlotIndex, Pet);
		}
	}

	// Passo 3: postura expira ao fim do slot (BTL-12) — nunca acumula
	// entre slots. Reset incondicional, vivo ou morto, por limpeza de
	// estado (mantém o hash consistente com "nada pendente entre slots").
	for (FPetState& Pet : State.Pets)
	{
		Pet.PostureFlags = 0;
	}

	// Passo 4: SlotEncerrado por último, uma única vez.
	EmitSlotEnded(OutTrace, SlotIndex);
}
