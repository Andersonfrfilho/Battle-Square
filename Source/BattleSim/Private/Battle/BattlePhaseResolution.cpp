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

	// Passo 2b: o efeito de atributo CONTA UM SLOT, e some quando zera.
	//
	// Diferente da postura, que expira todo slot: um efeito que durasse um
	// slot só nunca chegaria ao golpe seguinte, e a magia de atributo não
	// valeria o turno que ela custa.
	for (FPetState& Pet : State.Pets)
	{
		if (Pet.ActiveEffectSlotsRemaining == 0)
		{
			continue;
		}

		--Pet.ActiveEffectSlotsRemaining;
		if (Pet.ActiveEffectSlotsRemaining > 0)
		{
			continue;
		}

		// O FIM é narrado como o começo foi. Bônus que some calado faz o
		// jogador achar que o dano ficou errado — e este projeto já gastou
		// rodadas com regra funcionando e parecendo quebrada.
		FBattleEvent Fim;
		Fim.Type = EBattleEventType::AtributoVoltouAoNormal;
		Fim.SlotIndex = SlotIndex;
		Fim.Phase = 5; // F5
		Fim.ActorId = Pet.PetId;
		Fim.Detail = Pet.ActiveEffectStat;
		Fim.Value = Pet.ActiveEffectPercent;
		OutTrace.Add(Fim);

		Pet.ActiveEffectStat = 0;
		Pet.ActiveEffectPercent = 0;
	}

	// Passo 3: postura expira ao fim do slot (BTL-12) — nunca acumula
	// entre slots. Reset incondicional, vivo ou morto, por limpeza de
	// estado (mantém o hash consistente com "nada pendente entre slots").
	for (FPetState& Pet : State.Pets)
	{
		// DP-ia-04: esconder-se por completo custa a ação seguinte, senão
		// camuflar todo slot seria dominante — quem nunca pode ser atingido
		// não pode perder. A cobrança viaja no próprio PostureFlags: um campo
		// novo entraria no hash do estado e invalidaria os snapshots de
		// determinismo de cenários que nem usam estas ações.
		const bool bEstavaCamuflado =
			(Pet.PostureFlags & static_cast<uint8>(EBattlePostureFlags::Camouflaged)) != 0;
		const bool bEstavaSubmerso =
			(Pet.PostureFlags & static_cast<uint8>(EBattlePostureFlags::Underground)) != 0;

		Pet.PostureFlags = 0;
		if (bEstavaCamuflado)
		{
			Pet.PostureFlags = static_cast<uint8>(EBattlePostureFlags::Revealing);
		}
		else if (bEstavaSubmerso)
		{
			Pet.PostureFlags = static_cast<uint8>(EBattlePostureFlags::Emerging);
		}
	}

	// Passo 4: SlotEncerrado por último, uma única vez.
	// O GELO DERRETE — e derrete AQUI, no mesmo lugar onde o efeito de
	// atributo expira, porque é a mesma pergunta: o que este slot consumiu.
	// Fizesse isso a fase de combate, terreno posto e terreno derretido
	// aconteceriam em ordens diferentes conforme quem atacou primeiro.
	//
	// E derrete DEPOIS do dano do slot: quem congelou uma casa para negar
	// terreno ao outro compra o slot inteiro, não o pedaço dele.
	for (int32 Indice = 0; Indice < State.CellCountdown.Num(); ++Indice)
	{
		if (State.CellCountdown[Indice] == 0)
		{
			continue;
		}

		--State.CellCountdown[Indice];
		if (State.CellCountdown[Indice] > 0)
		{
			continue;
		}

		const uint8 Volta = State.CellRevertsTo.IsValidIndex(Indice)
			? State.CellRevertsTo[Indice]
			: static_cast<uint8>(ECellProperty::None);

		State.CellLayout[Indice] = Volta;
		if (State.CellRevertsTo.IsValidIndex(Indice))
		{
			State.CellRevertsTo[Indice] = static_cast<uint8>(ECellProperty::None);
		}

		FBattleEvent Derreteu;
		Derreteu.Type = EBattleEventType::TerrenoDerreteu;
		Derreteu.SlotIndex = SlotIndex;
		Derreteu.Phase = 5; // F5
		Derreteu.ActorId = BattleEventNoActor;
		Derreteu.TargetId = BattleEventNoActor;
		Derreteu.ToCell = PackCell(
			static_cast<uint8>(Indice % State.GridColumns),
			static_cast<uint8>(Indice / State.GridColumns));
		Derreteu.Value = static_cast<int32>(Volta);
		OutTrace.Add(Derreteu);
	}

	EmitSlotEnded(OutTrace, SlotIndex);
}
