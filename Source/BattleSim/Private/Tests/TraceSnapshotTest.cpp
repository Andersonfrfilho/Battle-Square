// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

// T11 (tasks.md, BTL-16): entradas fixas -> trace gravado -> comparação
// byte a byte. Se este teste falhar, um evento (tipo, ator, valor, slot)
// mudou entre execuções para a MESMA entrada — regressão de determinismo,
// o tipo de bug mais caro de descobrir tarde.

namespace
{
	FPetState MakeSnapshotPet(uint8 PetId, uint8 Side, uint8 Column, uint8 Row, int32 Health, int32 Attack, int32 Defense, int32 Speed)
	{
		FPetState Pet;
		Pet.PetId = PetId;
		Pet.Side = Side;
		Pet.Column = Column;
		Pet.Row = Row;
		Pet.Health = Health;
		Pet.MaxHealth = Health;
		Pet.Attack = Attack;
		Pet.Defense = Defense;
		Pet.Speed = Speed;
		return Pet;
	}

	FTurnCommit MakeSnapshotCommit(EActionType T0, EBattleDirection D0, EActionType T1, EBattleDirection D1, EActionType T2, EBattleDirection D2)
	{
		FTurnCommit Commit;
		Commit.Actions[0] = { T0, D0 };
		Commit.Actions[1] = { T1, D1 };
		Commit.Actions[2] = { T2, D2 };
		return Commit;
	}

	// Assinatura textual do trace inteiro — o "byte a byte" do requisito,
	// numa forma legível quando o teste falha e aponta a primeira linha
	// que diverge, em vez de só dizer "diferente" (design.md, estratégia
	// de verificação).
	FString SerializeTrace(const TArray<FBattleEvent>& Trace)
	{
		FString Result;
		for (const FBattleEvent& Event : Trace)
		{
			Result += FString::Printf(
				TEXT("[%d|F%d] Type=%d Actor=%d Target=%d From=%d To=%d Value=%d\n"),
				Event.SlotIndex, Event.Phase, static_cast<int32>(Event.Type),
				Event.ActorId, Event.TargetId, Event.FromCell, Event.ToCell, Event.Value);
		}
		return Result;
	}

	// Roda um cenário duas vezes e compara linha a linha, apontando a
	// primeira divergência em vez de só "diferente".
	bool RunAndCompareSnapshot(FAutomationTestBase& Test, const TCHAR* ScenarioName, const FBattleState& InitialState, const FTurnCommit& LeftCommit, const FTurnCommit& RightCommit)
	{
		const FBattleResolveResult ResultA = FBattleResolver::ResolveTurn(InitialState, LeftCommit, RightCommit);
		const FBattleResolveResult ResultB = FBattleResolver::ResolveTurn(InitialState, LeftCommit, RightCommit);

		const FString TraceA = SerializeTrace(ResultA.Trace);
		const FString TraceB = SerializeTrace(ResultB.Trace);

		if (TraceA == TraceB)
		{
			return true;
		}

		TArray<FString> LinesA, LinesB;
		TraceA.ParseIntoArrayLines(LinesA);
		TraceB.ParseIntoArrayLines(LinesB);

		const int32 MinLines = FMath::Min(LinesA.Num(), LinesB.Num());
		for (int32 Index = 0; Index < MinLines; ++Index)
		{
			if (LinesA[Index] != LinesB[Index])
			{
				Test.AddError(FString::Printf(
					TEXT("[%s] Primeira divergência na linha %d:\nA: %s\nB: %s"),
					ScenarioName, Index, *LinesA[Index], *LinesB[Index]));
				return false;
			}
		}
		Test.AddError(FString::Printf(TEXT("[%s] Traces têm tamanhos diferentes: %d vs %d linhas"), ScenarioName, LinesA.Num(), LinesB.Num()));
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraceSnapshotDeterminismTest,
	"BattleSim.Snapshot.FiveScenariosProduceIdenticalTraces",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTraceSnapshotDeterminismTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;

	// Cenário 1: dois pets adjacentes trocando ataques simples.
	{
		FBattleState State;
		State.Pets.Add(MakeSnapshotPet(1, 0, 1, 1, 50, 12, 6, 10));
		State.Pets.Add(MakeSnapshotPet(2, 1, 2, 1, 50, 10, 5, 8));
		const FTurnCommit Left = MakeSnapshotCommit(EActionType::Atacar, EBattleDirection::Direita, EActionType::Atacar, EBattleDirection::Direita, EActionType::Atacar, EBattleDirection::Direita);
		const FTurnCommit Right = MakeSnapshotCommit(EActionType::Atacar, EBattleDirection::Esquerda, EActionType::Defender, EBattleDirection::Nenhuma, EActionType::Esquivar, EBattleDirection::Nenhuma);
		bAllPassed &= RunAndCompareSnapshot(*this, TEXT("TrocaDeAtaques"), State, Left, Right);
	}

	// Cenário 2: morte mútua no mesmo slot.
	{
		FBattleState State;
		State.Pets.Add(MakeSnapshotPet(1, 0, 1, 1, 8, 20, 0, 10));
		State.Pets.Add(MakeSnapshotPet(2, 1, 2, 1, 8, 20, 0, 10));
		const FTurnCommit Left = MakeSnapshotCommit(EActionType::Atacar, EBattleDirection::Direita, EActionType::Aguardar, EBattleDirection::Nenhuma, EActionType::Aguardar, EBattleDirection::Nenhuma);
		const FTurnCommit Right = MakeSnapshotCommit(EActionType::Atacar, EBattleDirection::Esquerda, EActionType::Aguardar, EBattleDirection::Nenhuma, EActionType::Aguardar, EBattleDirection::Nenhuma);
		bAllPassed &= RunAndCompareSnapshot(*this, TEXT("MorteMutua"), State, Left, Right);
	}

	// Cenário 3: commit por timeout — um lado não enfileirou nada
	// (equivalente a 3 Aguardar, ver BTL-20).
	{
		FBattleState State;
		State.Pets.Add(MakeSnapshotPet(1, 0, 0, 1, 40, 15, 5, 12));
		State.Pets.Add(MakeSnapshotPet(2, 1, 2, 1, 40, 15, 5, 12));
		const FTurnCommit Left = MakeSnapshotCommit(EActionType::Mover, EBattleDirection::Direita, EActionType::Atacar, EBattleDirection::Direita, EActionType::Magia, EBattleDirection::Direita);
		FTurnCommit Right; // timeout: default-construído, os 3 slots são Aguardar
		bAllPassed &= RunAndCompareSnapshot(*this, TEXT("CommitPorTimeout"), State, Left, Right);
	}

	// Cenário 4: movimento simultâneo com troca de casas e bloqueio de borda.
	{
		FBattleState State;
		State.Pets.Add(MakeSnapshotPet(1, 0, 0, 0, 30, 10, 5, 8));
		State.Pets.Add(MakeSnapshotPet(2, 1, 1, 0, 30, 10, 5, 8));
		const FTurnCommit Left = MakeSnapshotCommit(EActionType::Mover, EBattleDirection::Direita, EActionType::Mover, EBattleDirection::Cima, EActionType::Mover, EBattleDirection::Baixo);
		const FTurnCommit Right = MakeSnapshotCommit(EActionType::Mover, EBattleDirection::Esquerda, EActionType::Aguardar, EBattleDirection::Nenhuma, EActionType::Aguardar, EBattleDirection::Nenhuma);
		bAllPassed &= RunAndCompareSnapshot(*this, TEXT("MovimentoComTrocaEBloqueio"), State, Left, Right);
	}

	// Cenário 5: triângulo completo — magia, esquiva e defesa no mesmo turno.
	{
		FBattleState State;
		State.Pets.Add(MakeSnapshotPet(1, 0, 1, 1, 60, 18, 8, 14));
		State.Pets.Add(MakeSnapshotPet(2, 1, 2, 1, 60, 14, 10, 9));
		const FTurnCommit Left = MakeSnapshotCommit(EActionType::Magia, EBattleDirection::Direita, EActionType::Defender, EBattleDirection::Nenhuma, EActionType::Atacar, EBattleDirection::Direita);
		const FTurnCommit Right = MakeSnapshotCommit(EActionType::Esquivar, EBattleDirection::Nenhuma, EActionType::Atacar, EBattleDirection::Esquerda, EActionType::Esquivar, EBattleDirection::Nenhuma);
		bAllPassed &= RunAndCompareSnapshot(*this, TEXT("TrianguloCompleto"), State, Left, Right);
	}

	return bAllPassed;
}
