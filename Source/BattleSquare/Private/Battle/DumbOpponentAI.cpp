// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/DumbOpponentAI.h"

namespace
{
	const EActionType AllActionTypes[] = {
		EActionType::Aguardar,
		EActionType::Mover,
		EActionType::Atacar,
		EActionType::Magia,
		EActionType::Defender,
		EActionType::Esquivar
	};

	const EBattleDirection AllDirections[] = {
		EBattleDirection::Cima, EBattleDirection::Baixo,
		EBattleDirection::Esquerda, EBattleDirection::Direita,
		EBattleDirection::CimaEsquerda, EBattleDirection::CimaDireita,
		EBattleDirection::BaixoEsquerda, EBattleDirection::BaixoDireita
	};

	const FPetState* FindPetOnSide(const FBattleState& State, uint8 Side)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side)
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	// Direções de Mover que mantêm o pet dentro da grade, a partir da
	// posição registrada em FBattleState (início do turno).
	TArray<EBattleDirection> ValidMoveDirections(const FPetState& Pet)
	{
		TArray<EBattleDirection> Valid;
		for (EBattleDirection Direction : AllDirections)
		{
			int8 DeltaColumn = 0;
			int8 DeltaRow = 0;
			GetDirectionDelta(Direction, DeltaColumn, DeltaRow);
			const int32 DestColumn = static_cast<int32>(Pet.Column) + DeltaColumn;
			const int32 DestRow = static_cast<int32>(Pet.Row) + DeltaRow;
			if (IsInsideGrid(DestColumn, DestRow))
			{
				Valid.Add(Direction);
			}
		}
		return Valid;
	}

	FBattleAction GenerateOneAction(const FPetState* Pet, FBattleRandom& Random)
	{
		const int32 TypeIndex = Random.NextRange(0, UE_ARRAY_COUNT(AllActionTypes) - 1);
		const EActionType ChosenType = AllActionTypes[TypeIndex];

		FBattleAction Action;
		Action.Type = ChosenType;

		if (!BattleActionRequiresDirection(ChosenType))
		{
			Action.Direction = EBattleDirection::Nenhuma;
			return Action;
		}

		if (ChosenType == EActionType::Mover && Pet)
		{
			const TArray<EBattleDirection> ValidDirections = ValidMoveDirections(*Pet);
			if (ValidDirections.IsEmpty())
			{
				// Pet encurralado (não deveria acontecer numa grade 3x3
				// com posição válida, mas nunca gerar Mover impossível).
				Action.Type = EActionType::Aguardar;
				Action.Direction = EBattleDirection::Nenhuma;
				return Action;
			}
			const int32 DirIndex = Random.NextRange(0, ValidDirections.Num() - 1);
			Action.Direction = ValidDirections[DirIndex];
			return Action;
		}

		// Atacar/Magia: qualquer direção é uma escolha válida — o
		// resolvedor decide "acertou"/"errou", a IA não precisa garantir alvo.
		const int32 DirIndex = Random.NextRange(0, UE_ARRAY_COUNT(AllDirections) - 1);
		Action.Direction = AllDirections[DirIndex];
		return Action;
	}
}

FTurnCommit FDumbOpponentAI::GenerateRandomValidCommit(const FBattleState& State, uint8 Side, FBattleRandom& Random)
{
	const FPetState* Pet = FindPetOnSide(State, Side);

	FTurnCommit Commit;
	for (int32 Index = 0; Index < FTurnCommit::ActionsPerTurn; ++Index)
	{
		Commit.Actions[Index] = GenerateOneAction(Pet, Random);
	}
	return Commit;
}
