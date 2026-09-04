// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/BountyReward.h"
#include "Misc/AutomationTest.h"

/**
 * CR5 — a recompensa é paga pelo criminoso: debita o ladrão, o governo cobre o
 * rombo, e devolver o próprio pet não paga nada (o contrapeso).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBountyPaidByCriminalTest,
	"BattleSquare.Meta.Recompensa.PagaPeloCriminoso",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBountyPaidByCriminalTest::RunTest(const FString&)
{
	using namespace BountyReward;

	FRewardInput Base;
	Base.ReturnerAccountId = TEXT("heroi");
	Base.VictimAccountId = TEXT("vitima");
	Base.ThiefAccountId = TEXT("ladrao");
	Base.RewardAmount = 100;

	// LADRÃO COM SALDO: paga tudo, o governo não entra.
	FRewardInput ComSaldo = Base;
	ComSaldo.ThiefBalance = 500;
	FRewardResult R1 = Resolve(ComSaldo);
	TestTrue(TEXT("a recompensa e paga"), R1.bRewardPaid);
	TestEqual(TEXT("o resolvedor recebe a recompensa cheia"), R1.ToReturner, 100);
	TestEqual(TEXT("saiu tudo do ladrao"), R1.FromThief, 100);
	TestEqual(TEXT("o governo nao entrou"), R1.FromGovernment, 0);

	// LADRÃO SEM SALDO SUFICIENTE (decisão 28): o governo cobre o rombo, mas o
	// resolvedor recebe a recompensa cheia — nunca do nada em silencio.
	FRewardInput SemSaldo = Base;
	SemSaldo.ThiefBalance = 30;
	FRewardResult R2 = Resolve(SemSaldo);
	TestEqual(TEXT("o resolvedor recebe cheio mesmo assim"), R2.ToReturner, 100);
	TestEqual(TEXT("o ladrao paga o que tem"), R2.FromThief, 30);
	TestEqual(TEXT("o governo cobre o rombo, explicito"), R2.FromGovernment, 70);

	// LADRÃO ZERADO: tudo do governo, nada do nada — a origem fica clara.
	FRewardInput Zerado = Base;
	Zerado.ThiefBalance = 0;
	FRewardResult R3 = Resolve(Zerado);
	TestEqual(TEXT("nada do ladrao zerado"), R3.FromThief, 0);
	TestEqual(TEXT("tudo do governo"), R3.FromGovernment, 100);

	// O CONTRAPESO: devolver o PRÓPRIO pet não paga recompensa a ninguém.
	FRewardInput ProprioPet = Base;
	ProprioPet.ReturnerAccountId = TEXT("vitima"); // a vítima se resolvendo
	ProprioPet.ThiefBalance = 500;
	FRewardResult R4 = Resolve(ProprioPet);
	TestFalse(TEXT("a vitima se resolvendo nao gera recompensa"), R4.bRewardPaid);
	TestEqual(TEXT("nada sai do ladrao"), R4.FromThief, 0);
	TestEqual(TEXT("nada vai para a vitima"), R4.ToReturner, 0);

	return true;
}
