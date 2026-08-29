// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetAttributeProgression.h"

#include "Battle/BattleEvent.h"
#include "Battle/BattleState.h"

namespace
{
	// Dano vira músculo devagar: sem divisor, uma batalha de 300 de dano daria
	// 300 de musculatura, e o atributo estouraria antes de significar algo.
	constexpr int32 DamagePerMusclePoint = 10;

	/** O pet levou dano neste slot? É o que separa postura ÚTIL de postura à toa. */
	bool TookDamageInSlot(const TArray<FBattleEvent>& Trace, uint8 PetId, uint8 SlotIndex)
	{
		for (const FBattleEvent& Event : Trace)
		{
			if (Event.SlotIndex == SlotIndex
				&& Event.TargetId == PetId
				&& Event.Type == EBattleEventType::DanoAplicado
				&& Event.Value > 0)
			{
				return true;
			}
		}
		return false;
	}

	int32 SkillSlotForPosture(int32 PostureFlagValue)
	{
		switch (static_cast<EBattlePostureFlags>(PostureFlagValue))
		{
		case EBattlePostureFlags::Camouflaged: return FPetAttributeProgression::Camouflage;
		case EBattlePostureFlags::Flying:      return FPetAttributeProgression::Flight;
		case EBattlePostureFlags::Underground: return FPetAttributeProgression::Underground;
		default:                               return INDEX_NONE;
		}
	}
}

bool FPetAttributeGains::IsEmpty() const
{
	return Musculature == 0 && Personality == 0
		&& SkillProficiency[0] == 0 && SkillProficiency[1] == 0 && SkillProficiency[2] == 0;
}

FPetAttributeGains FPetAttributeProgression::ComputeGains(const TArray<FBattleEvent>& Trace, uint8 PetId)
{
	FPetAttributeGains Gains;
	int32 DamageDealt = 0;

	for (const FBattleEvent& Event : Trace)
	{
		if (Event.ActorId != PetId)
		{
			continue;
		}

		switch (Event.Type)
		{
		case EBattleEventType::AtaqueAcertou:
			// MUSCULATURA vem do dano que saiu, não do golpe que foi dado.
			DamageDealt += Event.Value;
			Gains.Personality += 1;
			break;

		case EBattleEventType::AtaqueErrou:
			// Errar ainda inclina para agressivo: a INTENÇÃO foi agressiva, e
			// é o comportamento que forma o temperamento, não o resultado.
			Gains.Personality += 1;
			break;

		case EBattleEventType::PosturaAssumida:
		{
			Gains.Personality -= 1;

			const int32 Slot = SkillSlotForPosture(Event.Value);
			if (Slot != INDEX_NONE && !TookDamageInSlot(Trace, PetId, Event.SlotIndex))
			{
				// Só conta a postura que PROTEGEU. Contar toda postura
				// recompensaria camuflar contra um oponente que não ataca.
				Gains.SkillProficiency[Slot] += 1;
			}
			break;
		}

		default:
			break;
		}
	}

	Gains.Musculature = DamageDealt / DamagePerMusclePoint;
	return Gains;
}

void FPetAttributeProgression::Apply(FOwnedPetInstance& Instance, const FPetAttributeGains& Gains)
{
	Instance.Musculature += Gains.Musculature;
	Instance.Personality += Gains.Personality;

	for (int32 Slot = 0; Slot < 3; ++Slot)
	{
		Instance.SkillProficiency[Slot] += Gains.SkillProficiency[Slot];
	}
}
