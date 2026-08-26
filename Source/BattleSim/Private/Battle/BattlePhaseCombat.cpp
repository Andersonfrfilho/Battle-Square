// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleArenaConstants.h"

namespace
{
	// Balanceamento é parâmetro, não estrutura (design.md) — mas os valores
	// vivem em algum lugar até virarem DataAsset de skill (M3/DP-04).
	// Percentuais inteiros: 100 = 1.0x. Nenhum float em lugar nenhum (AD-004).
	constexpr int32 MinDamage = 1;
	constexpr int32 DefendingDefenseFactorPercent = 150; // +50% de defesa efetiva ao defender
	constexpr int32 AttackDamageMultiplierPercent = 100;
	constexpr int32 MagicDamageMultiplierPercent = 150;

	bool HasPosture(const FPetState& Pet, EBattlePostureFlags Flag)
	{
		return (Pet.PostureFlags & static_cast<uint8>(Flag)) != 0;
	}

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

	// Encontra um oponente vivo na célula informada.
	FPetState* FindLivingOpponentAtCell(FBattleState& State, uint8 AttackerSide, int32 Column, int32 Row)
	{
		for (FPetState& Pet : State.Pets)
		{
			if (Pet.Side != AttackerSide && Pet.IsAlive() && Pet.Column == Column && Pet.Row == Row)
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	// DP-04 (spec.md, decidida): alcance 1 na direção escolhida, mais a
	// própria casa se houver oponente coabitando (zero-alcance/melee).
	// A própria casa é checada primeiro: se o oponente já está em cima de
	// você, não faz sentido a direção decidir se ele é alvo ou não.
	FPetState* ResolveTarget(FBattleState& State, const FPetState& Attacker, EBattleDirection Direction)
	{
		if (FPetState* CoabitingOpponent = FindLivingOpponentAtCell(State, Attacker.Side, Attacker.Column, Attacker.Row))
		{
			return CoabitingOpponent;
		}

		int8 DeltaColumn = 0;
		int8 DeltaRow = 0;
		GetDirectionDelta(Direction, DeltaColumn, DeltaRow);

		const int32 TargetColumn = static_cast<int32>(Attacker.Column) + DeltaColumn;
		const int32 TargetRow = static_cast<int32>(Attacker.Row) + DeltaRow;
		if (!IsInsideGrid(TargetColumn, TargetRow))
		{
			return nullptr;
		}

		return FindLivingOpponentAtCell(State, Attacker.Side, TargetColumn, TargetRow);
	}

	bool IsOnBuffCell(const FPetState& Pet, const TArray<uint8>& CellLayout)
	{
		return CellLayout[CellLayoutIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Buff);
	}

	// Fórmula de dano — só inteiros, multiplicador em percentual (design.md).
	// EfetivoAtaque = Ataque * (BuffAtacante ? CellBuffPercent : 100) / 100
	// DefesaEfetiva = Defesa * FatorDefesa / 100, onde FatorDefesa combina
	//   Defendendo e casa de buff (Arenas Variadas, design.md — buff é
	//   contextual: fortalece quem ataca a partir dela E quem defende
	//   nela, nunca persiste em FPetState).
	// Dano          = Max(DanoMinimo, EfetivoAtaque - DefesaEfetiva)
	int32 ComputeDamage(const FPetState& Attacker, const FPetState& Target, int32 ActionMultiplierPercent, const TArray<uint8>& CellLayout)
	{
		const bool bTargetDefending = HasPosture(Target, EBattlePostureFlags::Defending);
		const bool bAttackerBuffed = IsOnBuffCell(Attacker, CellLayout);
		const bool bTargetBuffed = IsOnBuffCell(Target, CellLayout);

		const int32 EffectiveAttack = bAttackerBuffed
			? (Attacker.Attack * BattleArenaConstants::CellBuffPercent) / 100
			: Attacker.Attack;

		int32 DefenseFactorPercent = bTargetDefending ? DefendingDefenseFactorPercent : 100;
		if (bTargetBuffed)
		{
			DefenseFactorPercent = (DefenseFactorPercent * BattleArenaConstants::CellBuffPercent) / 100;
		}
		const int32 EffectiveDefense = (Target.Defense * DefenseFactorPercent) / 100;

		const int32 RawDamage = (EffectiveAttack * ActionMultiplierPercent) / 100 - EffectiveDefense;
		return FMath::Max(MinDamage, RawDamage);
	}

	void EmitMiss(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Attacker)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::AtaqueErrou;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Attacker.PetId;
		Event.TargetId = BattleEventNoActor;
		OutTrace.Add(Event);
	}

	void EmitDodged(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Attacker, const FPetState& Target)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::Esquivou;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Target.PetId; // quem executou a esquiva é o sujeito do evento
		Event.TargetId = Attacker.PetId;
		OutTrace.Add(Event);
	}

	void EmitHit(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Attacker, const FPetState& Target, int32 Damage)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::AtaqueAcertou;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Attacker.PetId;
		Event.TargetId = Target.PetId;
		Event.Value = Damage;
		OutTrace.Add(Event);
	}

	void ResolveAttackForSide(
		FBattleState& State,
		uint8 AttackerSide,
		const FBattleAction& Action,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace)
	{
		if (Action.Type != EActionType::Atacar && Action.Type != EActionType::Magia)
		{
			return;
		}

		FPetState* Attacker = FindAlivePetOnSide(State, AttackerSide);
		if (!Attacker)
		{
			return;
		}

		FPetState* Target = ResolveTarget(State, *Attacker, Action.Direction);
		if (!Target)
		{
			EmitMiss(OutTrace, SlotIndex, *Attacker);
			return;
		}

		const bool bIsMagic = (Action.Type == EActionType::Magia);

		// Esquiva anula ataque FÍSICO. Magia ignora esquiva (BTL-10) —
		// é o segundo lado do triângulo ataque/defesa/esquiva.
		if (!bIsMagic && HasPosture(*Target, EBattlePostureFlags::Dodging))
		{
			EmitDodged(OutTrace, SlotIndex, *Attacker, *Target);
			return;
		}

		const int32 Multiplier = bIsMagic ? MagicDamageMultiplierPercent : AttackDamageMultiplierPercent;
		const int32 Damage = ComputeDamage(*Attacker, *Target, Multiplier, State.CellLayout);

		// Acumula — NÃO aplica. F5 (T8) aplica tudo de uma vez (BTL-07).
		Target->PendingDamage += Damage;

		EmitHit(OutTrace, SlotIndex, *Attacker, *Target, Damage);
	}
}

void BattlePhases::ApplyCombat(
	FBattleState& State,
	const FBattleAction& LeftAction,
	const FBattleAction& RightAction,
	uint8 SlotIndex,
	TArray<FBattleEvent>& OutTrace)
{
	ResolveAttackForSide(State, /*AttackerSide=*/0, LeftAction, SlotIndex, OutTrace);
	ResolveAttackForSide(State, /*AttackerSide=*/1, RightAction, SlotIndex, OutTrace);
}
