// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/LeadershipRules.h"
#include "Misc/AutomationTest.h"
#include "World/SettlementEconomy.h"

/**
 * A LIDERANÇA DE CENTRO (decisão 15) — e as duas frases dela viram teste:
 * "com renda por isso" e "não pode sair até alguém vencê-lo, inclusive NPC".
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeadershipBindsAndChangesHandsByBattleTest,
	"BattleSquare.Meta.Lideranca.OPostoPrendeEMudaDeMaoEmBatalha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeadershipBindsAndChangesHandsByBattleTest::RunTest(const FString&)
{
	FTrainerProfile Perfil;

	// Sem título, todo centro é DESAFIO — o campeonato está aberto.
	TestEqual(TEXT("sem titulo, o desafio e desafio"),
		static_cast<int32>(FLeadershipRules::VerdictFor(Perfil, TEXT("vila-inicial"))),
		static_cast<int32>(FLeadershipRules::EChallengeVerdict::Challenge));

	// O campeonato vencido toma o título...
	FLeadershipRules::TakeTitle(Perfil, TEXT("vila-inicial"));
	TestTrue(TEXT("o titulo e seu"),
		FLeadershipRules::IsLeaderOf(Perfil, TEXT("vila-inicial")));

	// ...e o posto PRENDE (a leitura registrada de "não pode sair"): no
	// próprio centro o desafio vira DEFESA; em qualquer outro, é recusa.
	TestEqual(TEXT("no proprio centro, defende-se"),
		static_cast<int32>(FLeadershipRules::VerdictFor(Perfil, TEXT("vila-inicial"))),
		static_cast<int32>(FLeadershipRules::EChallengeVerdict::Defense));
	TestEqual(TEXT("noutro centro, o posto tranca"),
		static_cast<int32>(FLeadershipRules::VerdictFor(Perfil, TEXT("cidade-grande"))),
		static_cast<int32>(FLeadershipRules::EChallengeVerdict::LockedElsewhere));

	// A defesa perdida é como o título muda de mão — inclusive para NPC.
	FLeadershipRules::LoseTitle(Perfil);
	TestFalse(TEXT("perdeu a defesa, perdeu o posto"),
		FLeadershipRules::IsLeaderAnywhere(Perfil));
	TestEqual(TEXT("e o mundo reabre"),
		static_cast<int32>(FLeadershipRules::VerdictFor(Perfil, TEXT("cidade-grande"))),
		static_cast<int32>(FLeadershipRules::EChallengeVerdict::Challenge));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeadershipStipendPaysTodayOnlyTest,
	"BattleSquare.Meta.Lideranca.ARendaEhDeHojeESoDeHoje",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeadershipStipendPaysTodayOnlyTest::RunTest(const FString&)
{
	FTrainerProfile Perfil;
	FLeadershipRules::TakeTitle(Perfil, TEXT("vila-inicial"));

	// O DIA DA POSSE NÃO PAGA: o campeonato já pagou o prêmio, e pagar os
	// dois seria contar o mesmo dia duas vezes. A primeira visita só carimba.
	TestEqual(TEXT("o dia da posse carimba e nao paga"),
		FLeadershipRules::CollectDailyStipend(Perfil, TEXT("vila-inicial"), 5, 9), 0);

	// O dia seguinte paga — UMA vez.
	TestEqual(TEXT("o dia seguinte paga"),
		FLeadershipRules::CollectDailyStipend(Perfil, TEXT("vila-inicial"), 6, 9), 9);
	TestEqual(TEXT("e nao paga duas vezes no mesmo dia"),
		FLeadershipRules::CollectDailyStipend(Perfil, TEXT("vila-inicial"), 6, 9), 0);

	// DIAS PERDIDOS NÃO ACUMULAM: sumir três dias rende UM pagamento — o
	// posto que "não pode sair" cobra presença, e renda retroativa vira baú.
	TestEqual(TEXT("tres dias fora, um pagamento"),
		FLeadershipRules::CollectDailyStipend(Perfil, TEXT("vila-inicial"), 9, 9), 9);

	// RELÓGIO PARA TRÁS (sessão nova zera as horas) rearma sem pagar: renda
	// por reiniciar o jogo seria a torneira mais boba de todas.
	TestEqual(TEXT("dia para tras rearma e nao paga"),
		FLeadershipRules::CollectDailyStipend(Perfil, TEXT("vila-inicial"), 2, 9), 0);
	TestEqual(TEXT("e o dia seguinte do relogio novo paga"),
		FLeadershipRules::CollectDailyStipend(Perfil, TEXT("vila-inicial"), 3, 9), 9);

	// E o centro ERRADO não paga nada: a renda é do posto, não do cargo.
	TestEqual(TEXT("na arena alheia nao ha renda"),
		FLeadershipRules::CollectDailyStipend(Perfil, TEXT("cidade-grande"), 10, 30), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeadershipStipendComesFromTheTableTest,
	"BattleSquare.Meta.Lideranca.ARendaVemDaTabela",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeadershipStipendComesFromTheTableTest::RunTest(const FString&)
{
	// "Com renda por isso" — e a renda vem da MESMA tabela do prêmio
	// (invariante 15): fração do prêmio de ranking do lugar, afirmada CONTRA
	// a tabela e nunca como número digitado aqui.
	const ESettlementKind Lugares[] = {
		ESettlementKind::VilaInicial, ESettlementKind::CidadeGrande,
		ESettlementKind::PostoDeFronteira,
	};

	for (ESettlementKind Lugar : Lugares)
	{
		TestEqual(TEXT("a renda e a fracao do premio do MESMO lugar"),
			SettlementEconomy::LeaderDailyStipend(Lugar),
			SettlementEconomy::RankingPrize(Lugar) * 30 / 100);
	}

	// A renda diária é MENOR que o prêmio: renda alta faria o título valer
	// mais que o campeonato, e o campeonato é o clímax.
	TestTrue(TEXT("um dia de posto vale menos que o campeonato"),
		SettlementEconomy::LeaderDailyStipend(ESettlementKind::CidadeGrande)
			< SettlementEconomy::RankingPrize(ESettlementKind::CidadeGrande));

	// Onde não há Arena não há posto — nem renda.
	TestEqual(TEXT("sem Arena, sem renda"),
		SettlementEconomy::LeaderDailyStipend(ESettlementKind::PostoDeFronteira), 0);

	return true;
}
