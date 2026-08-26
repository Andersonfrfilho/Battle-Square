// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/BattleDataTranslator.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FLoadedPetRecord MakeFixturePet(const FString& Type, int32 Attack)
	{
		FLoadedPetRecord Record;
		Record.Id = TEXT("fixture-") + Type;
		Record.Name = Type + TEXT("Pet");
		Record.Type = Type;
		Record.Attack = Attack;
		Record.Defense = 5;
		Record.Speed = 5;
		Record.MaxHealth = 50;
		return Record;
	}
}

// T3: caso ASSIMÉTRICO — Left (Fogo) é super efetivo contra Right
// (Planta), mas Right (Planta) é NEUTRO contra Left (Fogo, sem entrada
// nesse sentido). Um bug de inversão atacante/defensor faria os dois
// lados mudarem simetricamente, ou o lado errado mudar — este teste
// pegaria os dois casos.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleDataTranslatorMatchupAsymmetricTest,
	"BattleSquare.Data.TranslateMatchupAppliesTypeEffectiveness",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleDataTranslatorMatchupAsymmetricTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Table;
	Table.SetPercent(TEXT("Fogo"), TEXT("Planta"), 150); // Fogo -> Planta: super efetivo
	// Planta -> Fogo: NENHUMA entrada — deve ficar neutro (100).

	const FLoadedPetRecord LeftSource = MakeFixturePet(TEXT("Fogo"), 20);
	const FLoadedPetRecord RightSource = MakeFixturePet(TEXT("Planta"), 20);

	FPetState LeftState, RightState;
	FPetPresentationInfo LeftPresentation, RightPresentation;
	FBattleDataTranslator::TranslateMatchup(LeftSource, RightSource, Table, /*LeftPetId=*/1, /*RightPetId=*/2,
		LeftState, LeftPresentation, RightState, RightPresentation);

	TestEqual(TEXT("Left (Fogo, super efetivo contra Planta) tem Attack aumentado: 20*150/100=30"), LeftState.Attack, 30);
	TestEqual(TEXT("Right (Planta, neutro contra Fogo — sem entrada nesse sentido) mantém Attack base"), RightState.Attack, 20);

	TestEqual(TEXT("Defense do Left não muda por tipo"), LeftState.Defense, 5);
	TestEqual(TEXT("Speed do Right não muda por tipo"), RightState.Speed, 5);
	TestEqual(TEXT("MaxHealth do Left não muda por tipo"), LeftState.MaxHealth, 50);

	// Caso resistido: Agua -> Fogo resistido (50%).
	FTypeEffectivenessTable ResistTable;
	ResistTable.SetPercent(TEXT("Agua"), TEXT("Fogo"), 50);
	const FLoadedPetRecord WaterSource = MakeFixturePet(TEXT("Agua"), 20);
	const FLoadedPetRecord FireSource = MakeFixturePet(TEXT("Fogo"), 20);
	FPetState WaterState, FireState;
	FPetPresentationInfo WaterPresentation, FirePresentation;
	FBattleDataTranslator::TranslateMatchup(WaterSource, FireSource, ResistTable, 1, 2, WaterState, WaterPresentation, FireState, FirePresentation);
	TestEqual(TEXT("Agua resistido por Fogo: Attack reduzido, 20*50/100=10"), WaterState.Attack, 10);
	TestEqual(TEXT("Fogo (neutro contra Agua, sem entrada nesse sentido) mantém Attack base"), FireState.Attack, 20);

	// Caso neutro (zero regressão — tabela vazia, comportamento idêntico a TranslatePet).
	FTypeEffectivenessTable EmptyTable;
	const FLoadedPetRecord NeutralA = MakeFixturePet(TEXT("Normal"), 20);
	const FLoadedPetRecord NeutralB = MakeFixturePet(TEXT("Normal"), 20);
	FPetState NeutralAState, NeutralBState;
	FPetPresentationInfo NeutralAPresentation, NeutralBPresentation;
	FBattleDataTranslator::TranslateMatchup(NeutralA, NeutralB, EmptyTable, 1, 2, NeutralAState, NeutralAPresentation, NeutralBState, NeutralBPresentation);
	TestEqual(TEXT("Tabela vazia — Attack de A idêntico ao base (zero regressão)"), NeutralAState.Attack, 20);
	TestEqual(TEXT("Tabela vazia — Attack de B idêntico ao base (zero regressão)"), NeutralBState.Attack, 20);

	// Ponta a ponta: o FPetState resultante alimenta o resolvedor real.
	FBattleState State;
	LeftState.Column = 1; LeftState.Row = 1;
	RightState.Column = 2; RightState.Row = 1;
	State.Pets.Add(LeftState);
	State.Pets.Add(RightState);

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
	FTurnCommit RightCommit;
	RightCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit);
	TestTrue(TEXT("FPetState com Attack pré-multiplicado alimenta o resolvedor real sem erro"), Result.Trace.Num() > 0);

	return true;
}
