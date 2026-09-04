// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/CharacterAppearance.h"

/**
 * A POLÍCIA PATRULHA, RECONHECE E AGE (crime-e-recompensa, CR9 — decisão 26).
 *
 * O ponto sutil: a polícia NÃO lê a trilha de auditoria — ela não sabe quem é
 * "de fato" procurado. Ela enxerga o que está na frente dela: a APARÊNCIA e o
 * PET. Por isso a decisão 26 diz que "a polícia erra quando um jogador coincide
 * em características E pet com o bandido": o falso positivo é intencional, e é o
 * preço de a lei ser um NPC que observa, não um oráculo que consulta o banco.
 *
 * A identificação exige as DUAS coincidências (aparência reconhecível E pet
 * igual), justamente para o erro ser raro — nunca uma só.
 */
enum class EPoliceAction : uint8
{
	/** Segue a ronda: fora de alcance, ou não bate a identificação. */
	Ignore,

	/** Identificou o procurado — a prisão (decisões 27/28) entra em cena. */
	Arrest
};

namespace PoliceEncounter
{
	/** O que o policial em ronda observa ao cruzar com alguém. */
	struct BATTLESQUARE_API FEncounter
	{
		/** O suspeito está dentro do alcance da ronda? Fora dele, nada acontece. */
		bool bWithinPatrolRange = false;

		/** O pet que o suspeito carrega é o mesmo do cartaz? */
		bool bPetMatchesWanted = false;

		/** A descrição que o cartaz guardou do bandido. */
		FCharacterAppearance PosterAppearance;

		/** A aparência de quem o policial está vendo agora. */
		FCharacterAppearance SuspectAppearance;
	};

	/**
	 * O que o policial faz ao cruzar com este suspeito.
	 *
	 * Prende só quando as DUAS coincidências batem, dentro do alcance. É o
	 * contrapeso do aceite: quem não bate a aparência do cartaz, ou carrega
	 * outro pet, passa direto — "reage" e "sempre ativo" ficam distinguíveis.
	 */
	BATTLESQUARE_API EPoliceAction Decide(const FEncounter& Encounter);
}
