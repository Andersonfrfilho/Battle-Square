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
	constexpr int32 DodgeOverDefendPercent = 50;

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

	/** Passo em direção ao inimigo que NÃO sai da grade. */
	EBattleDirection StepTowardsInsideGrid(int32 SelfColumn, int32 SelfRow, int32 EnemyColumn, int32 EnemyRow)
	{
		const EBattleDirection Direct = GetDirectionTowards(EnemyColumn - SelfColumn, EnemyRow - SelfRow);

		int8 DeltaColumn = 0;
		int8 DeltaRow = 0;
		GetDirectionDelta(Direct, DeltaColumn, DeltaRow);

		if (IsInsideGrid(SelfColumn + DeltaColumn, SelfRow + DeltaRow))
		{
			return Direct;
		}

		// Diagonal barrada pela borda: tentar cada eixo sozinho antes de
		// desistir. Numa grade 3x3 isto quase sempre salva o passo — e desistir
		// custaria o slot inteiro.
		const EBattleDirection ApenasColuna = GetDirectionTowards(EnemyColumn - SelfColumn, 0);
		GetDirectionDelta(ApenasColuna, DeltaColumn, DeltaRow);
		if (ApenasColuna != EBattleDirection::Nenhuma && IsInsideGrid(SelfColumn + DeltaColumn, SelfRow + DeltaRow))
		{
			return ApenasColuna;
		}

		const EBattleDirection ApenasLinha = GetDirectionTowards(0, EnemyRow - SelfRow);
		GetDirectionDelta(ApenasLinha, DeltaColumn, DeltaRow);
		if (ApenasLinha != EBattleDirection::Nenhuma && IsInsideGrid(SelfColumn + DeltaColumn, SelfRow + DeltaRow))
		{
			return ApenasLinha;
		}

		return EBattleDirection::Nenhuma;
	}
}

FTurnCommit FTacticalOpponentAI::GenerateCommit(const FBattleState& State, uint8 Side, FBattleRandom& Random)
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
			const EBattleDirection Step = StepTowardsInsideGrid(Column, Row, EnemyColumn, EnemyRow);
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
			// Esquivar anula o físico inteiro mas é furado por magia;
			// defender reduz os dois. Alternar impede que o jogador resolva o
			// oponente ferido sempre com a mesma resposta.
			const bool bDodge = Random.NextRange(0, 99) < DodgeOverDefendPercent;
			Commit.Actions[Slot] = MakeAction(
				bDodge ? EActionType::Esquivar : EActionType::Defender, EBattleDirection::Nenhuma);
			continue;
		}

		const EBattleDirection Towards = GetDirectionTowards(EnemyColumn - Column, EnemyRow - Row);

		// Coabitando a mesma casa: o resolvedor acha o alvo sem direção, mas
		// uma direção qualquer mantém a ação bem formada.
		const EBattleDirection AttackDirection = Towards != EBattleDirection::Nenhuma
			? Towards
			: EBattleDirection::Cima;

		const bool bUseMagic = Random.NextRange(0, 99) < MagicOverAttackPercent;
		Commit.Actions[Slot] = MakeAction(
			bUseMagic ? EActionType::Magia : EActionType::Atacar, AttackDirection);
	}

	return Commit;
}
