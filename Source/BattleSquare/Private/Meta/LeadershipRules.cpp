// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/LeadershipRules.h"

FLeadershipRules::EChallengeVerdict FLeadershipRules::VerdictFor(
	const FTrainerProfile& Profile, const FString& SettlementKey)
{
	if (!IsLeaderAnywhere(Profile))
	{
		return EChallengeVerdict::Challenge;
	}

	return IsLeaderOf(Profile, SettlementKey)
		? EChallengeVerdict::Defense
		: EChallengeVerdict::FreeChallenge;
}

bool FLeadershipRules::RegisterChallengerOnNewDay(FTrainerProfile& Profile, int32 Today)
{
	if (!IsLeaderAnywhere(Profile) || Today < 0 || Profile.PendingDefenseDay >= 0)
	{
		return false;
	}

	Profile.PendingDefenseDay = Today;
	return true;
}

void FLeadershipRules::ClearPendingDefense(FTrainerProfile& Profile)
{
	Profile.PendingDefenseDay = -1;
}

void FLeadershipRules::TakeTitle(FTrainerProfile& Profile, const FString& SettlementKey)
{
	Profile.LeaderOf = SettlementKey;

	// O carimbo rearma na posse: a renda começa AMANHÃ — o dia da conquista
	// já pagou o prêmio do campeonato, e pagar os dois seria contar o mesmo
	// dia duas vezes.
	Profile.LastStipendDay = -1;
}

void FLeadershipRules::LoseTitle(FTrainerProfile& Profile)
{
	Profile.LeaderOf.Empty();
	Profile.LastStipendDay = -1;

	// A fila morre com o posto: desafiante esperando um título que já caiu
	// seria um aviso eterno sobre nada.
	Profile.PendingDefenseDay = -1;
}

int32 FLeadershipRules::CollectDailyStipend(FTrainerProfile& Profile,
	const FString& SettlementKey, int32 Today, int32 StipendPerDay)
{
	if (!IsLeaderOf(Profile, SettlementKey) || StipendPerDay <= 0 || Today < 0)
	{
		return 0;
	}

	// -1 é posse recém-tomada: carimba hoje e a renda começa no dia seguinte.
	// Dia para trás é relógio de sessão nova: rearma, sem pagar — pagar por
	// relógio zerado seria renda por reiniciar o jogo.
	if (Profile.LastStipendDay < 0 || Profile.LastStipendDay > Today)
	{
		Profile.LastStipendDay = Today;
		return 0;
	}

	if (Profile.LastStipendDay == Today)
	{
		return 0;
	}

	// SÓ o dia de hoje, nunca os perdidos: o posto cobra presença.
	Profile.LastStipendDay = Today;
	return StipendPerDay;
}
