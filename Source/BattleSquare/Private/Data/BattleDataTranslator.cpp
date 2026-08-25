// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/BattleDataTranslator.h"

void FBattleDataTranslator::TranslatePet(
	const FLoadedPetRecord& Source,
	uint8 PetId,
	uint8 Side,
	uint8 Column,
	uint8 Row,
	FPetState& OutBattleState,
	FPetPresentationInfo& OutPresentation)
{
	// attack/defense/speed/maxHealth: cópia direta, sem conversão — já
	// chegam inteiros do backend (T18, tasks.md).
	OutBattleState = FPetState();
	OutBattleState.PetId = PetId;
	OutBattleState.Side = Side;
	OutBattleState.Column = Column;
	OutBattleState.Row = Row;
	OutBattleState.Attack = Source.Attack;
	OutBattleState.Defense = Source.Defense;
	OutBattleState.Speed = Source.Speed;
	OutBattleState.MaxHealth = Source.MaxHealth;
	// Health = MaxHealth no início da batalha (T18).
	OutBattleState.Health = Source.MaxHealth;

	OutPresentation = FPetPresentationInfo();
	OutPresentation.PetId = PetId;
	OutPresentation.Name = Source.Name;

	// "type" NUNCA entra em FPetState — vira FGameplayTag só aqui, na
	// camada de apresentação (BattleSquare). ErrorIfNotFound=false: tags
	// de tipo de pet ainda não foram registradas como conteúdo (fora do
	// escopo deste backend) — tag não encontrada vira tag vazia, não
	// crash. Registro de conteúdo é trabalho futuro (M3).
	const FName TagName(*FString::Printf(TEXT("Pet.Type.%s"), *Source.Type));
	OutPresentation.TypeTag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound=*/false);
}
