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
	OutPresentation.Type = Source.Type;
	OutPresentation.CatalogId = Source.Id;

	// "type" NUNCA entra em FPetState — vira FGameplayTag só aqui, na
	// camada de apresentação (BattleSquare). ErrorIfNotFound=false: tags
	// de tipo de pet ainda não foram registradas como conteúdo (fora do
	// escopo deste backend) — tag não encontrada vira tag vazia, não
	// crash. Registro de conteúdo é trabalho futuro (M3).
	const FName TagName(*FString::Printf(TEXT("Pet.Type.%s"), *Source.Type));
	OutPresentation.TypeTag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound=*/false);
}

void FBattleDataTranslator::TranslateMatchup(
	const FLoadedPetRecord& LeftSource,
	const FLoadedPetRecord& RightSource,
	const FTypeEffectivenessTable& EffectivenessTable,
	uint8 LeftPetId,
	uint8 RightPetId,
	FPetState& OutLeftState,
	FPetPresentationInfo& OutLeftPresentation,
	FPetState& OutRightState,
	FPetPresentationInfo& OutRightPresentation)
{
	// Posições padrão de duelo 1v1 — mesma convenção usada em todo o
	// projeto (esquerda coluna 1, direita coluna 2, linha 1).
	TranslatePet(LeftSource, LeftPetId, /*Side=*/0, /*Column=*/1, /*Row=*/1, OutLeftState, OutLeftPresentation);
	TranslatePet(RightSource, RightPetId, /*Side=*/1, /*Column=*/2, /*Row=*/1, OutRightState, OutRightPresentation);

	// Efetividade é sobre O ATAQUE do lado — o Attack do Left muda pela
	// efetividade DO TIPO DO LEFT CONTRA O TIPO DO RIGHT, nunca o
	// inverso (T3 é 🧠 justamente por essa inversão ser fácil de errar).
	const int32 LeftEffectivenessPercent = EffectivenessTable.GetPercent(LeftSource.Type, RightSource.Type);
	const int32 RightEffectivenessPercent = EffectivenessTable.GetPercent(RightSource.Type, LeftSource.Type);

	OutLeftState.Attack = (OutLeftState.Attack * LeftEffectivenessPercent) / 100;
	OutRightState.Attack = (OutRightState.Attack * RightEffectivenessPercent) / 100;

	// O mesmo número que multiplicou o ataque vai para a apresentação. Assim a
	// tela DIZ a efetividade sem recalculá-la — um valor só, computado uma vez.
	OutLeftPresentation.EffectivenessPercent = LeftEffectivenessPercent;
	OutRightPresentation.EffectivenessPercent = RightEffectivenessPercent;
}
