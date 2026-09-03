// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * A LIDERANÇA DE CENTRO (decisão 15) — pura sobre o perfil, como a carteira.
 *
 * "Ganhar o campeonato torna o jogador LÍDER do centro, com renda por isso —
 * e ele não pode sair nem aceitar desafios até alguém vencê-lo, inclusive
 * NPC."
 *
 * ⚠️ LEITURA REGISTRADA (é minha, e é reversível): "não pode sair nem aceitar
 * desafios" = o posto PRENDE — o líder não abandona o título por vontade, e
 * não desafia OUTRAS arenas enquanto o tem. O que ele faz no próprio centro é
 * DEFENDER: o desafio ali vira defesa contra o desafiante do dia, e perder a
 * defesa é como o título muda de mão — inclusive para NPC.
 */
class BATTLESQUARE_API FLeadershipRules
{
public:
	/** O que o desafio significa AQUI, para quem este perfil é. */
	enum class EChallengeVerdict : uint8
	{
		/** Desafio normal: tomar o título deste centro. */
		Challenge,

		/** Você é o líder DAQUI: o desafio é DEFESA contra o desafiante. */
		Defense,

		/**
		 * Você é líder de OUTRO centro — o posto prende. Desafiar fora seria
		 * abandonar a cadeira com o título no bolso.
		 */
		LockedElsewhere
	};

	static bool IsLeaderAnywhere(const FTrainerProfile& Profile)
	{
		return !Profile.LeaderOf.IsEmpty();
	}

	static bool IsLeaderOf(const FTrainerProfile& Profile, const FString& SettlementKey)
	{
		return !SettlementKey.IsEmpty() && Profile.LeaderOf == SettlementKey;
	}

	static EChallengeVerdict VerdictFor(const FTrainerProfile& Profile,
		const FString& SettlementKey);

	/** O campeonato foi vencido: o título é seu. */
	static void TakeTitle(FTrainerProfile& Profile, const FString& SettlementKey);

	/** A defesa foi perdida: o desafiante levou o posto. */
	static void LoseTitle(FTrainerProfile& Profile);

	/**
	 * A renda de HOJE, se houver — e só a de hoje.
	 *
	 * A renda não acumula: quem não aparece não recebe os dias perdidos —
	 * renda retroativa infinita viraria baú, e o posto que "não pode sair"
	 * cobra presença. Dia que anda para TRÁS (sessão nova zera o relógio até
	 * a idade offline da `mundo-vivo` existir) só rearma o carimbo.
	 *
	 * Devolve o valor pago; zero quando não há o que pagar.
	 */
	static int32 CollectDailyStipend(FTrainerProfile& Profile,
		const FString& SettlementKey, int32 Today, int32 StipendPerDay);
};
