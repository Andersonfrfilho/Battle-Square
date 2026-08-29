// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/TacticalOpponentAI.h"

namespace
{
	// Ferido o bastante para valer a pena trocar um ataque por proteção.
	constexpr int32 HurtHealthPercent = 40;

	// Pesos em porcentagem. Não são "aleatoriedade decorativa": o commit é
	// simultâneo e ÀS CEGAS, então um oponente perfeitamente previsível é
	// explorado de cor na segunda partida. Intenção decide QUAIS ações fazem
	// sentido; o sorteio decide qual delas sai.
	constexpr int32 MagicOverAttackPercent = 35;
	constexpr int32 DefendWhenHurtPercent = 45;

	// Entre as cinco proteções, a escolha é sorteada com pesos: a defesa que o
	// jogador consegue prever é a defesa que ele fura. Camuflar e submergir
	// pesam MENOS porque custam a ação seguinte — usá-las sempre entregaria os
	// turnos de graça.
	constexpr int32 DodgePercent = 30;
	constexpr int32 DefendPercent = 30;
	constexpr int32 FlyPercent = 20;
	constexpr int32 CamouflagePercent = 12;
	constexpr int32 SubmergePercent = 8;

	const FPetState* FindLivingPetOnSide(const FBattleState& State, uint8 Side)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == Side && Pet.Health > 0)
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	const FPetState* FindLivingOpponent(const FBattleState& State, uint8 Side)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side != Side && Pet.Health > 0)
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	bool IsWithinReach(int32 SelfColumn, int32 SelfRow, int32 EnemyColumn, int32 EnemyRow)
	{
		// Alcance real do resolvedor: casa adjacente na direção escolhida, ou
		// o oponente coabitando a própria casa. Isto NÃO reinterpreta a regra
		// — se ela mudar em BattlePhaseCombat, isto aqui fica errado junto, e
		// é por isso que o teste NeverAttacksEmptyAir existe.
		return FMath::Abs(SelfColumn - EnemyColumn) <= 1 && FMath::Abs(SelfRow - EnemyRow) <= 1;
	}

	FBattleAction MakeAction(EActionType Type, EBattleDirection Direction)
	{
		FBattleAction Action;
		Action.Type = Type;
		Action.Direction = BattleActionRequiresDirection(Type) ? Direction : EBattleDirection::Nenhuma;
		return Action;
	}

	/**
	 * Ataque com um golpe SORTEADO entre os quatro.
	 *
	 * Desde DP-golpe-05 a direção não decide o alvo, então mirar deixou de
	 * fazer sentido para a IA. Sortear o golpe mantém a variação que o commit
	 * às cegas exige — um oponente que usa sempre o golpe 0 é decorado na
	 * segunda partida.
	 *
	 * Escolher o MELHOR golpe exigiria conhecer o poder de cada um, que só a
	 * montagem tem; isso é trabalho da fatia 3, quando cada golpe ganhar
	 * efeito próprio.
	 */
	FBattleAction MakeAttackAction(EActionType Type, FBattleRandom& Random)
	{
		return MakeMoveAction(Type, static_cast<uint8>(Random.NextRange(0, BattleMovesPerPet - 1)));
	}

	bool PodeUsar(const TArray<EActionType>& Disponiveis, EActionType Tipo)
	{
		// Vazio = sem restrição: comportamento de antes das skills por pet.
		return Disponiveis.IsEmpty() || Disponiveis.Contains(Tipo);
	}

	/**
	 * Proteção sorteada com pesos, entre as que ESTE pet sabe fazer.
	 *
	 * Sortear primeiro e filtrar depois faria o pet sem skill cair sempre na
	 * mesma proteção — previsível é explorável, e era justamente o que a
	 * variação existia para evitar.
	 */
	EActionType ChooseProtection(FBattleRandom& Random, const TArray<EActionType>& Disponiveis)
	{
		TArray<TPair<EActionType, int32>> Candidatas;
		int32 PesoTotal = 0;

		const TPair<EActionType, int32> Todas[] = {
			{ EActionType::Esquivar, DodgePercent },
			{ EActionType::Defender, DefendPercent },
			{ EActionType::Voar, FlyPercent },
			{ EActionType::Camuflar, CamouflagePercent },
			{ EActionType::Submergir, SubmergePercent },
		};

		for (const TPair<EActionType, int32>& Candidata : Todas)
		{
			if (PodeUsar(Disponiveis, Candidata.Key))
			{
				Candidatas.Add(Candidata);
				PesoTotal += Candidata.Value;
			}
		}

		if (Candidatas.IsEmpty() || PesoTotal <= 0)
		{
			return EActionType::Defender;
		}

		int32 Sorteio = Random.NextRange(0, PesoTotal - 1);
		for (const TPair<EActionType, int32>& Candidata : Candidatas)
		{
			Sorteio -= Candidata.Value;
			if (Sorteio < 0)
			{
				return Candidata.Key;
			}
		}

		return Candidatas.Last().Key;
	}

	/** Passo em direção ao inimigo que NÃO sai da grade. */
	EBattleDirection StepTowardsInsideGrid(const FBattleState& State, int32 SelfColumn, int32 SelfRow, int32 EnemyColumn, int32 EnemyRow)
	{
		const EBattleDirection Direct = GetDirectionTowards(EnemyColumn - SelfColumn, EnemyRow - SelfRow);

		int8 DeltaColumn = 0;
		int8 DeltaRow = 0;
		GetDirectionDelta(Direct, DeltaColumn, DeltaRow);

		if (State.IsInside(SelfColumn + DeltaColumn, SelfRow + DeltaRow))
		{
			return Direct;
		}

		// Diagonal barrada pela borda: tentar cada eixo sozinho antes de
		// desistir. Em grade pequena isto quase sempre salva o passo — e desistir
		// custaria o slot inteiro.
		const EBattleDirection ApenasColuna = GetDirectionTowards(EnemyColumn - SelfColumn, 0);
		GetDirectionDelta(ApenasColuna, DeltaColumn, DeltaRow);
		if (ApenasColuna != EBattleDirection::Nenhuma && State.IsInside(SelfColumn + DeltaColumn, SelfRow + DeltaRow))
		{
			return ApenasColuna;
		}

		const EBattleDirection ApenasLinha = GetDirectionTowards(0, EnemyRow - SelfRow);
		GetDirectionDelta(ApenasLinha, DeltaColumn, DeltaRow);
		if (ApenasLinha != EBattleDirection::Nenhuma && State.IsInside(SelfColumn + DeltaColumn, SelfRow + DeltaRow))
		{
			return ApenasLinha;
		}

		return EBattleDirection::Nenhuma;
	}
}

FTurnCommit FTacticalOpponentAI::GenerateCommit(const FBattleState& State, uint8 Side, FBattleRandom& Random,
	const TArray<EActionType>& AvailableActions)
{
	FTurnCommit Commit;

	const FPetState* Self = FindLivingPetOnSide(State, Side);
	const FPetState* Enemy = FindLivingOpponent(State, Side);

	if (!Self || !Enemy)
	{
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			Commit.Actions[Slot] = MakeAction(EActionType::Aguardar, EBattleDirection::Nenhuma);
		}
		return Commit;
	}

	// Posição SIMULADA ao longo dos três slots. A IA antiga decidia os três a
	// partir da posição inicial, então "aproximar e atacar" era impossível
	// para ela: o slot 2 não sabia que o slot 1 tinha andado.
	int32 Column = static_cast<int32>(Self->Column);
	int32 Row = static_cast<int32>(Self->Row);

	// O inimigo fica onde estava: o commit é às cegas, e prever o movimento
	// dele seria a IA sabendo o que não pode saber.
	const int32 EnemyColumn = static_cast<int32>(Enemy->Column);
	const int32 EnemyRow = static_cast<int32>(Enemy->Row);

	const int32 HealthPercent = Self->MaxHealth > 0
		? (Self->Health * 100) / Self->MaxHealth
		: 100;
	const bool bIsHurt = HealthPercent <= HurtHealthPercent;

	for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
	{
		if (!IsWithinReach(Column, Row, EnemyColumn, EnemyRow))
		{
			const EBattleDirection Step = StepTowardsInsideGrid(State, Column, Row, EnemyColumn, EnemyRow);
			if (Step != EBattleDirection::Nenhuma)
			{
				int8 DeltaColumn = 0;
				int8 DeltaRow = 0;
				GetDirectionDelta(Step, DeltaColumn, DeltaRow);
				Column += DeltaColumn;
				Row += DeltaRow;

				Commit.Actions[Slot] = MakeAction(EActionType::Mover, Step);
				continue;
			}

			// Encurralado sem passo possível: proteger vale mais que aguardar.
			Commit.Actions[Slot] = MakeAction(EActionType::Defender, EBattleDirection::Nenhuma);
			continue;
		}

		if (bIsHurt && Random.NextRange(0, 99) < DefendWhenHurtPercent)
		{
			Commit.Actions[Slot] = MakeAction(ChooseProtection(Random, AvailableActions), EBattleDirection::Nenhuma);
			continue;
		}

		const bool bUseMagic = Random.NextRange(0, 99) < MagicOverAttackPercent;
		Commit.Actions[Slot] = MakeAttackAction(
			bUseMagic ? EActionType::Magia : EActionType::Atacar, Random);
	}

	return Commit;
}
