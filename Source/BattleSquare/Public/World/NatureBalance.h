// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * MÃE NATUREZA — O CORRETOR (mae-natureza, MN2/MN3/MN4).
 *
 * Recebe o CENSO (o que existe) e a FAIXA-ALVO (o que devia existir) e devolve a
 * CORREÇÃO (o que muda). Puro como `WorldTimeOfDay`: sem mundo, sem tempo, sem
 * banco — e por isso NUNCA importa `ForestBackdrop` nem nada que leia o mundo. É
 * a camada de FORA que busca o censo e aplica a correção; um corretor que se
 * serve sozinho seria um plano consultando outro que o consulta de volta (o
 * ciclo de reentrada do §14 de geracao-procedural).
 *
 * Mãe Natureza ajusta a TORNEIRA, nunca o BALDE: prazo de rebrota, demanda,
 * preço, prêmio — jamais a coleção, o atributo ou a especialidade do jogador. A
 * MN4 é estrutural: `FNatureCorrecao` não TEM campo que aponte para balde, então
 * escrever essa correção nem compila.
 */

/** As TORNEIRAS que Mãe Natureza pode girar — nunca um balde entre elas. */
enum class ENatureTap : uint8
{
	/** Prazo de rebrota de recurso (mundo-vivo MV4). */
	RegrowthDeadline,
	/** Demanda do comerciante. */
	MerchantDemand,
	/** Preço de serviço de assentamento. */
	Price,
	/** Prêmio de ranking. */
	RankingPrize,
	/** Peso de encontro de uma espécie — a torneira da migração (MN6). */
	EncounterWeight,
};

namespace NatureBalance
{
	/** O que existe hoje: o nível medido de algo que a natureza regula. */
	struct BATTLESQUARE_API FNatureCenso
	{
		/** Qual torneira este censo mede. */
		ENatureTap Tap = ENatureTap::RegrowthDeadline;

		/** O nível atual medido. */
		float CurrentLevel = 0.0f;
	};

	/** O que devia existir: a faixa saudável, entre um mínimo e um máximo. */
	struct BATTLESQUARE_API FNatureFaixaAlvo
	{
		float Min = 0.0f;
		float Max = 0.0f;
	};

	/**
	 * O que muda — e SÓ torneira. Não há, nem por acidente, campo de coleção,
	 * atributo ou especialidade: é a MN4 na assinatura, não em runtime.
	 */
	struct BATTLESQUARE_API FNatureCorrecao
	{
		/** Qual torneira girar. */
		ENatureTap Tap = ENatureTap::RegrowthDeadline;

		/** Quanto girar: positivo abre (aumenta), negativo fecha (reduz). */
		float TapAdjustment = 0.0f;

		bool IsZero() const { return FMath::IsNearlyZero(TapAdjustment); }
	};

	/**
	 * A correção para trazer o censo de volta à faixa.
	 *
	 * Abaixo do mínimo, abre a torneira (ajuste positivo até o mínimo); acima do
	 * máximo, fecha (negativo até o máximo); DENTRO da faixa, correção ZERO — Mãe
	 * Natureza não mexe no que está saudável. Faixa invertida (Min > Max) é
	 * tratada como sem-alvo: correção zero, nunca um ajuste doido.
	 */
	BATTLESQUARE_API FNatureCorrecao Correct(
		const FNatureCenso& Censo, const FNatureFaixaAlvo& Faixa);

	/** Uma linha do registro: correção sem registro é indistinguível de bug (MN3). */
	struct BATTLESQUARE_API FNatureLogEntry
	{
		ENatureTap Tap = ENatureTap::RegrowthDeadline;
		float FromLevel = 0.0f;
		float Adjustment = 0.0f;
	};

	/**
	 * Aplica a correção DELATANDO-A: toda correção não-nula grava uma linha no
	 * registro. Este é o ÚNICO caminho de aplicação — não há como aplicar sem
	 * registrar, e é isso que a MN3 exige. Devolve true se registrou (correção
	 * não-nula), false se não havia o que corrigir.
	 */
	BATTLESQUARE_API bool ApplyAndLog(
		const FNatureCorrecao& Correcao, float CensoLevel, TArray<FNatureLogEntry>& OutLog);
}
