// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/DumbOpponentAI.h"

#include "Meta/PlayStyleRules.h"

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
	TArray<EBattleDirection> ValidMoveDirections(const FBattleState& State, const FPetState& Pet)
	{
		TArray<EBattleDirection> Valid;
		for (EBattleDirection Direction : AllDirections)
		{
			int8 DeltaColumn = 0;
			int8 DeltaRow = 0;
			GetDirectionDelta(Direction, DeltaColumn, DeltaRow);
			const int32 DestColumn = static_cast<int32>(Pet.Column) + DeltaColumn;
			const int32 DestRow = static_cast<int32>(Pet.Row) + DeltaRow;
			if (State.IsInside(DestColumn, DestRow))
			{
				Valid.Add(Direction);
			}
		}
		return Valid;
	}

	// A montagem de UMA ação de um TIPO JÁ DECIDIDO — separada da escolha do
	// tipo para o bot estilizado (decisão 15-d) reaproveitar a regra da grade
	// em vez de copiá-la.
	FBattleAction GenerateActionOfType(const FBattleState& State, const FPetState* Pet,
		EActionType ChosenType, FBattleRandom& Random)
	{
		FBattleAction Action;
		Action.Type = ChosenType;

		if (!BattleActionRequiresDirection(ChosenType))
		{
			Action.Direction = EBattleDirection::Nenhuma;
			return Action;
		}

		if (ChosenType == EActionType::Mover && Pet)
		{
			const TArray<EBattleDirection> ValidDirections = ValidMoveDirections(State, *Pet);
			if (ValidDirections.IsEmpty())
			{
				// Pet encurralado (não deveria acontecer numa grade usável
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

	FBattleAction GenerateOneAction(const FBattleState& State, const FPetState* Pet, FBattleRandom& Random)
	{
		const int32 TypeIndex = Random.NextRange(0, UE_ARRAY_COUNT(AllActionTypes) - 1);
		return GenerateActionOfType(State, Pet, AllActionTypes[TypeIndex], Random);
	}
}

FTurnCommit FDumbOpponentAI::GenerateRandomValidCommit(const FBattleState& State, uint8 Side, FBattleRandom& Random)
{
	const FPetState* Pet = FindPetOnSide(State, Side);

	FTurnCommit Commit;
	for (int32 Index = 0; Index < FTurnCommit::ActionsPerTurn; ++Index)
	{
		Commit.Actions[Index] = GenerateOneAction(State, Pet, Random);
	}
	return Commit;
}

FTurnCommit FDumbOpponentAI::GenerateStyledCommit(const FBattleState& State, uint8 Side,
	FBattleRandom& Random, const TArray<int32>& StyleCounts,
	const TArray<FStyleTransition>& StyleTransitions)
{
	const FPetState* Pet = FindPetOnSide(State, Side);

	FTurnCommit Commit;
	uint8 Anterior = PlayStyleRules::StartOfTurn;

	for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
	{
		// TENDÊNCIA × SEQUÊNCIA, em multiplicação — o mesmo desenho de fase ×
		// lugar da fauna: nenhum fator substitui o outro, e quem "abre
		// defendendo e fecha atacando" pesa as duas coisas de uma vez. O menu
		// é o MESMO do bot uniforme — um segundo menu divergiria dele na
		// primeira skill nova.
		int32 Total = 0;
		int32 Pesos[UE_ARRAY_COUNT(AllActionTypes)];
		for (int32 Indice = 0; Indice < UE_ARRAY_COUNT(AllActionTypes); ++Indice)
		{
			const int32 Valor = static_cast<int32>(AllActionTypes[Indice]);
			const int32 DaTendencia = 1 + (StyleCounts.IsValidIndex(Valor)
				? FMath::Max(0, StyleCounts[Valor]) : 0);
			const int32 DaSequencia = 1 + PlayStyleRules::TransitionCount(
				StyleTransitions, Anterior, static_cast<uint8>(Valor));

			// Suavização nos DOIS fatores: histórico vazio joga uniforme, e
			// nenhum tipo morre por nunca ter sido usado — estilo é viés,
			// não mordaça.
			Pesos[Indice] = DaTendencia * DaSequencia;
			Total += Pesos[Indice];
		}

		int32 Restante = Random.NextRange(0, Total - 1);
		EActionType Escolhido = AllActionTypes[0];
		for (int32 Indice = 0; Indice < UE_ARRAY_COUNT(AllActionTypes); ++Indice)
		{
			Restante -= Pesos[Indice];
			if (Restante < 0)
			{
				Escolhido = AllActionTypes[Indice];
				break;
			}
		}

		// A direção é a do bot de sempre, pela MESMA montagem: reescrever a
		// validade de Mover aqui seria a segunda cópia da regra da grade.
		// (Mover encurralado ainda degrada para Aguardar lá dentro — ação
		// VÁLIDA vale mais que estilo fiel.)
		Commit.Actions[Slot] = GenerateActionOfType(State, Pet, Escolhido, Random);

		// O elo da sequência é o que ACONTECEU, não o que se quis: se Mover
		// degradou para Aguardar, o próximo passo parte de Aguardar — a
		// simulação segue o jogo real, não a intenção.
		Anterior = static_cast<uint8>(Commit.Actions[Slot].Type);
	}

	return Commit;
}
