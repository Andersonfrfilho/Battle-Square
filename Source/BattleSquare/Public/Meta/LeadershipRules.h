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
 * ⚠️ A LEITURA MUDOU COM O DONO (03/09): "se ele sair, manda um aviso — e ele
 * pode optar por batalhar ou deixar no automático". O posto NÃO prende mais:
 * o líder viaja e desafia onde quiser (sem tomar segundo título — líder é de
 * UM centro). O que o posto cobra é RESPOSTA: o desafiante chega com o dia e
 * espera; o líder batalha quando avisado, ou liga a defesa automática — e aí
 * o BattleSim joga por ele. Perder a defesa (jogada ou automática) é como o
 * título muda de mão — inclusive para NPC.
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
		 * Você é líder de OUTRO centro. A batalha é LIVRE (a tranca caiu com
		 * a emenda) — mas o título de lá não vem: líder é de UM centro, e o
		 * prêmio da vitória é só prêmio.
		 */
		FreeChallenge
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
	 * O DIA VIROU: chegou desafiante? (decisão 15, emendada)
	 *
	 * Um por vez — fila de um lugar: o segundo desafiante do mundo espera o
	 * primeiro ter resposta, senão o líder ausente voltava para um cerco.
	 * Devolve true quando um desafiante NOVO chegou agora.
	 */
	static bool RegisterChallengerOnNewDay(FTrainerProfile& Profile, int32 Today);

	/** A defesa foi respondida (jogada ou automática): a fila esvazia. */
	static void ClearPendingDefense(FTrainerProfile& Profile);

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
