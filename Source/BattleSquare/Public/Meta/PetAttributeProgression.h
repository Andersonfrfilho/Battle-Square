// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

struct FBattleEvent;

/** Quanto cada atributo mudou numa batalha — para somar e para MOSTRAR. */
struct BATTLESQUARE_API FPetAttributeGains
{
	int32 Musculature = 0;
	int32 Personality = 0;
	int32 SkillProficiency[3] = { 0, 0, 0 };

	bool IsEmpty() const;
};

/**
 * Traduz o traço de uma batalha em crescimento de atributo.
 *
 * Função pura sobre o traço: não lê estado do mundo, não grava nada, e por isso
 * é testável sem abrir o editor — a mesma razão de FBattleNarration ser pura.
 *
 * DP-atr-03: cresce por uso EFETIVO. Musculatura vem do DANO CAUSADO, não do
 * ataque desferido; proficiência de skill só conta a postura que de fato
 * protegeu. Sem isso, o jogador é recompensado por moer contra um bot parado em
 * vez de por jogar.
 */
class BATTLESQUARE_API FPetAttributeProgression
{
public:
	/** Índices de SkillProficiency — a ordem é contrato com o save. */
	enum ESkillSlot : uint8
	{
		Camouflage = 0,
		Flight = 1,
		Underground = 2,
	};

	static FPetAttributeGains ComputeGains(const TArray<FBattleEvent>& Trace, uint8 PetId);

	static void Apply(FOwnedPetInstance& Instance, const FPetAttributeGains& Gains);
};
