// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/CharacterAppearance.h"

/**
 * O PET ROUBADO SE DENUNCIA NUMA BATALHA (crime-e-recompensa, CR8) — a regra
 * pura de quando o aviso sai, e para quem.
 *
 * A spec chama isto de "o centro do sistema", e a invariante 19 o marca como a
 * única mecânica desta feature com risco de virar PERSEGUIÇÃO entre jogadores
 * reais. Por isso a regra é escrita pelo caso NEGATIVO: o aviso só sai para
 * quem TEM relação com o roubo, e provar que ele NÃO sai para o resto é tão
 * obrigatório quanto provar que sai para quem deve.
 *
 * Três condições, e todas necessárias:
 *  1. o pet é roubado (`bCarrierPetIsStolen`, do servidor — a marca da CR6);
 *  2. o observador PODE reconhecer (é a vítima, OU viu a lista de procurados);
 *  3. o portador ainda é RECONHECÍVEL como o procurado (aparência, CR7/23) —
 *     quem trocou de rosto despistou, e a marca deixou de identificar.
 */
namespace StolenPetDenounce
{
	/** O que o observador sabe e vê ao encarar o portador do pet. */
	struct BATTLESQUARE_API FObserverContext
	{
		/** O pet do oponente está marcado como roubado (marca do servidor). */
		bool bCarrierPetIsStolen = false;

		/** Este observador É a vítima do roubo? Ela sempre reconhece. */
		bool bObserverIsVictim = false;

		/** O observador já viu a lista de procurados (CR4)? */
		bool bObserverSawWantedList = false;

		/** A aparência que o cartaz/registro guardou do ladrão. */
		FCharacterAppearance PosterAppearance;

		/** A aparência de quem está do outro lado da batalha AGORA. */
		FCharacterAppearance CarrierAppearanceNow;
	};

	/**
	 * O aviso de "este pet é roubado" sai para este observador?
	 *
	 * Falso em toda ausência: pet não roubado, observador sem relação, ou
	 * portador irreconhecível. É o contrapeso da invariante 19 embutido na
	 * regra — o aviso NÃO sai por omissão, e só as três condições juntas o
	 * ligam.
	 */
	BATTLESQUARE_API bool ShouldWarn(const FObserverContext& Context);
}
