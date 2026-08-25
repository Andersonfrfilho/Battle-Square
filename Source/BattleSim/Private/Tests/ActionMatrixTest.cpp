// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

// T12 (tasks.md, BTL-09 a BTL-13): matriz completa 5x5 = 25 combinações.
// Os 5 tipos relevantes ao triângulo de combate: Aguardar, Atacar, Magia,
// Defender, Esquivar. MOVER fica de fora — não interage com postura de
// combate, e já está coberto por BattleSim.Phase.Movement.*.
//
// TABELA DE ESPERADOS (legível sem rodar o teste):
//
//                | Alvo:Aguardar | Alvo:Atacar | Alvo:Magia | Alvo:Defender | Alvo:Esquivar
// Atk:Aguardar   | sem dano       | sem dano     | sem dano    | sem dano       | sem dano
// Atk:Atacar     | dano normal    | dano normal  | dano normal | dano REDUZIDO  | dano ZERO (esquivado)
// Atk:Magia      | dano normal    | dano normal  | dano normal | dano REDUZIDO  | dano normal (ignora esquiva)
// Atk:Defender   | sem dano       | sem dano     | sem dano    | sem dano       | sem dano
// Atk:Esquivar   | sem dano       | sem dano     | sem dano    | sem dano       | sem dano
//
// Linhas "Atk" que não são Atacar/Magia nunca causam dano — são posturas,
// não ações ofensivas (ApplyCombat só resolve Atacar/Magia; ver design.md).
// As colunas "Alvo:Aguardar/Atacar/Magia" são equivalentes entre si do
// ponto de vista de dano recebido: nenhuma delas assume postura defensiva.

namespace
{
	const EActionType AllFiveTypes[] = {
		EActionType::Aguardar,
		EActionType::Atacar,
		EActionType::Magia,
		EActionType::Defender,
		EActionType::Esquivar
	};

	FPetState MakeMatrixPet(uint8 PetId, uint8 Side, uint8 Column, int32 Attack, int32 Defense)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Column = Column;
		Pet.Row = 1;
		Pet.Health = 999; // alto o bastante para nunca morrer no meio do sweep
		Pet.MaxHealth = 999;
		Pet.Attack = Attack;
		Pet.Defense = Defense;
		return Pet;
	}

	FBattleAction MakeMatrixAction(EActionType Type, EBattleDirection Direction)
	{
		FBattleAction Action;
		Action.Type = Type;
		Action.Direction = Direction;
		return Action;
	}

	bool IsOffensive(EActionType Type)
	{
		return Type == EActionType::Atacar || Type == EActionType::Magia;
	}

	const TCHAR* ToLabel(EActionType Type)
	{
		switch (Type)
		{
			case EActionType::Aguardar: return TEXT("Aguardar");
			case EActionType::Atacar:   return TEXT("Atacar");
			case EActionType::Magia:    return TEXT("Magia");
			case EActionType::Defender: return TEXT("Defender");
			case EActionType::Esquivar: return TEXT("Esquivar");
			default: return TEXT("?");
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionMatrixFullFiveByFiveTest,
	"BattleSim.ActionMatrix.FullFiveByFiveGrid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionMatrixFullFiveByFiveTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	int32 CombinationsRun = 0;

	for (EActionType AttackerType : AllFiveTypes)
	{
		for (EActionType TargetType : AllFiveTypes)
		{
			++CombinationsRun;

			FBattleState State;
			State.Pets.Add(MakeMatrixPet(1, 0, 1, /*Attack=*/20, /*Defense=*/5));
			State.Pets.Add(MakeMatrixPet(2, 1, 2, /*Attack=*/10, /*Defense=*/5));

			const FBattleAction AttackerAction = MakeMatrixAction(AttackerType, IsOffensive(AttackerType) ? EBattleDirection::Direita : EBattleDirection::Nenhuma);
			const FBattleAction TargetAction = MakeMatrixAction(TargetType, EBattleDirection::Nenhuma);

			TArray<FBattleEvent> Trace;
			// F2 primeiro — postura do alvo precisa estar ativa antes de F4.
			BattlePhases::ApplyPostures(State, AttackerAction, TargetAction, 0, Trace);
			BattlePhases::ApplyCombat(State, AttackerAction, MakeMatrixAction(EActionType::Aguardar, EBattleDirection::Nenhuma), 0, Trace);

			const int32 ActualDamage = State.Pets[1].PendingDamage;
			const FString CaseLabel = FString::Printf(TEXT("Atk:%s vs Alvo:%s"), ToLabel(AttackerType), ToLabel(TargetType));

			if (!IsOffensive(AttackerType))
			{
				// Linha inteira: atacante não ofensivo nunca causa dano,
				// seja qual for a postura do alvo.
				bAllPassed &= TestEqual(*FString::Printf(TEXT("[%s] Sem dano — atacante não é ofensivo"), *CaseLabel), ActualDamage, 0);
				continue;
			}

			if (TargetType == EActionType::Esquivar && AttackerType == EActionType::Atacar)
			{
				bAllPassed &= TestEqual(*FString::Printf(TEXT("[%s] Ataque físico anulado pela esquiva"), *CaseLabel), ActualDamage, 0);
			}
			else if (TargetType == EActionType::Esquivar && AttackerType == EActionType::Magia)
			{
				bAllPassed &= TestTrue(*FString::Printf(TEXT("[%s] Magia ignora esquiva"), *CaseLabel), ActualDamage > 0);
			}
			else if (TargetType == EActionType::Defender)
			{
				// Compara com o dano do mesmo ataque sem postura nenhuma do alvo.
				FBattleState BaselineState;
				BaselineState.Pets.Add(MakeMatrixPet(1, 0, 1, 20, 5));
				BaselineState.Pets.Add(MakeMatrixPet(2, 1, 2, 10, 5));
				TArray<FBattleEvent> BaselineTrace;
				BattlePhases::ApplyCombat(BaselineState, AttackerAction, MakeMatrixAction(EActionType::Aguardar, EBattleDirection::Nenhuma), 0, BaselineTrace);
				const int32 BaselineDamage = BaselineState.Pets[1].PendingDamage;

				bAllPassed &= TestTrue(*FString::Printf(TEXT("[%s] Dano reduzido pela defesa (%d < %d)"), *CaseLabel, ActualDamage, BaselineDamage), ActualDamage < BaselineDamage);
			}
			else // Alvo: Aguardar, Atacar ou Magia — nenhuma postura defensiva assumida.
			{
				bAllPassed &= TestTrue(*FString::Printf(TEXT("[%s] Dano normal aplicado"), *CaseLabel), ActualDamage > 0);
			}
		}
	}

	TestEqual(TEXT("As 25 combinações da matriz 5x5 foram executadas"), CombinationsRun, 25);

	return bAllPassed;
}
