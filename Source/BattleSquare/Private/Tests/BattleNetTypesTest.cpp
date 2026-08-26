// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleNetTypes.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

// T2: valores de enum dentro do range são aceitos, inclusive o maior
// valor válido de cada um (caso de borda que costuma esconder off-by-one).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNetTypesValidatesAcceptsValidTest,
	"BattleSquare.Net.ValidateNetTurnCommit.AcceptsValid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNetTypesValidatesAcceptsValidTest::RunTest(const FString& Parameters)
{
	FNetTurnCommit Commit;
	Commit.ActionA = { EActionType::Aguardar, EBattleDirection::Nenhuma };
	Commit.ActionB = { EActionType::Esquivar, EBattleDirection::BaixoDireita }; // maior valor válido dos dois enums
	Commit.ActionC = { EActionType::Atacar, EBattleDirection::Direita };

	TestTrue(TEXT("Commit com valores válidos, incluindo o teto de cada enum, é aceito"), ValidateNetTurnCommit(Commit));
	return true;
}

// T2: enum fabricado fora de range (cliente adversarial) é rejeitado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNetTypesValidatesRejectsOutOfRangeTest,
	"BattleSquare.Net.ValidateNetTurnCommit.RejectsOutOfRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNetTypesValidatesRejectsOutOfRangeTest::RunTest(const FString& Parameters)
{
	FNetTurnCommit BadType;
	BadType.ActionA = { static_cast<EActionType>(200), EBattleDirection::Nenhuma };
	TestFalse(TEXT("EActionType fabricado fora do range é rejeitado"), ValidateNetTurnCommit(BadType));

	FNetTurnCommit BadDirection;
	BadDirection.ActionB = { EActionType::Atacar, static_cast<EBattleDirection>(200) };
	TestFalse(TEXT("EBattleDirection fabricado fora do range é rejeitado"), ValidateNetTurnCommit(BadDirection));

	return true;
}

// T3: ida e volta preserva as 3 ações exatamente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNetTypesRoundTripTest,
	"BattleSquare.Net.RoundTripConversion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNetTypesRoundTripTest::RunTest(const FString& Parameters)
{
	FNetTurnCommit Original;
	Original.ActionA = { EActionType::Mover, EBattleDirection::CimaEsquerda };
	Original.ActionB = { EActionType::Magia, EBattleDirection::BaixoDireita };
	Original.ActionC = { EActionType::Defender, EBattleDirection::Nenhuma };

	const FTurnCommit Core = ToTurnCommit(Original);
	const FNetTurnCommit RoundTripped = ToNetTurnCommit(Core);

	TestTrue(TEXT("ActionA sobrevive à ida e volta"),
		RoundTripped.ActionA.Type == Original.ActionA.Type && RoundTripped.ActionA.Direction == Original.ActionA.Direction);
	TestTrue(TEXT("ActionB sobrevive à ida e volta"),
		RoundTripped.ActionB.Type == Original.ActionB.Type && RoundTripped.ActionB.Direction == Original.ActionB.Direction);
	TestTrue(TEXT("ActionC sobrevive à ida e volta"),
		RoundTripped.ActionC.Type == Original.ActionC.Type && RoundTripped.ActionC.Direction == Original.ActionC.Direction);

	return true;
}

// T3: FTurnCommit convertido alimenta o resolvedor real sem erro —
// ponta a ponta, mesmo padrão de BuildCommitFeedsRealResolver.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleNetTypesFeedsRealResolverTest,
	"BattleSquare.Net.ConvertedCommitFeedsRealResolver",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleNetTypesFeedsRealResolverTest::RunTest(const FString& Parameters)
{
	FBattleState State;
	FPetState LeftPet;
	LeftPet.PetId = 1; LeftPet.Side = 0; LeftPet.Column = 1; LeftPet.Row = 1;
	LeftPet.Health = 50; LeftPet.MaxHealth = 50; LeftPet.Attack = 10; LeftPet.Defense = 5;
	FPetState RightPet;
	RightPet.PetId = 2; RightPet.Side = 1; RightPet.Column = 2; RightPet.Row = 1;
	RightPet.Health = 50; RightPet.MaxHealth = 50; RightPet.Attack = 10; RightPet.Defense = 5;
	State.Pets.Add(LeftPet);
	State.Pets.Add(RightPet);

	FNetTurnCommit NetLeftCommit;
	NetLeftCommit.ActionA = { EActionType::Atacar, EBattleDirection::Direita };

	FNetTurnCommit NetRightCommit;
	NetRightCommit.ActionA = { EActionType::Aguardar, EBattleDirection::Nenhuma };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, ToTurnCommit(NetLeftCommit), ToTurnCommit(NetRightCommit));
	TestTrue(TEXT("Resolvedor real aceitou o commit convertido do fio de rede"), Result.Trace.Num() > 0);

	return true;
}
