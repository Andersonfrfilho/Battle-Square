// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetView.h"
#include "Battle/BattleTypes.h"

APetView::APetView()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APetView::SetInitialState(const FPetState& InitialState, const FPetPresentationInfo& Presentation)
{
	PetId = InitialState.PetId;
	Column = InitialState.Column;
	Row = InitialState.Row;
	MaxHealth = InitialState.MaxHealth;
	HealthRatio = MaxHealth > 0 ? 1.0f : 0.0f;
	bDefeated = false;
}

void APetView::ApplyEvent(const FBattleEvent& Event)
{
	switch (Event.Type)
	{
		case EBattleEventType::DanoAplicado:
		{
			// Nunca recalcula dano — Value já é o número final que o
			// núcleo aplicou (BTL-22). Só converte para uma razão visual.
			if (MaxHealth > 0)
			{
				const float DamageRatio = static_cast<float>(Event.Value) / static_cast<float>(MaxHealth);
				HealthRatio = FMath::Clamp(HealthRatio - DamageRatio, 0.0f, 1.0f);
			}
			break;
		}
		case EBattleEventType::PetMorreu:
		{
			bDefeated = true;
			break;
		}
		case EBattleEventType::Moveu:
		{
			uint8 NewColumn = 0;
			uint8 NewRow = 0;
			UnpackCell(Event.ToCell, NewColumn, NewRow);
			Column = NewColumn;
			Row = NewRow;
			break;
		}
		default:
			break;
	}
}
