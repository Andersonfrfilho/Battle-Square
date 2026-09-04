// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * DE QUEM É ESTE PET (crime-e-recompensa, CR1) — a pergunta que roubo,
 * denúncia e polícia todos fazem, respondida num lugar só.
 *
 * Puro: recebe a conta dona (de onde `posse-no-servidor` a colocou) e a conta
 * de quem pergunta, devolve o vínculo. Não lê rede nem save — quem carrega a
 * posse é a costura da batalha (`SideOwners`, PS5) e o cache (PS7); esta função
 * só a INTERPRETA.
 *
 * "Selvagem" é RESPOSTA, não ausência de resposta (o aceite da task): um pet
 * sem dono é o bicho do mundo, e distinguir isso de "não sei" é o que separa
 * capturar de roubar.
 */
namespace PetOwnershipQuery
{
	enum class EOwnerRelation : uint8
	{
		/** É do próprio jogador que pergunta. */
		Self,

		/** Não tem dono — o bicho do mundo. Capturável, nunca roubável. */
		Wild,

		/** É de OUTRA conta. O único caso em que roubo faz sentido. */
		OtherAccount,
	};

	/**
	 * `OwnerAccountId` é a conta dona do pet (vazia = selvagem, a convenção de
	 * PS5); `AskerAccountId` é quem pergunta.
	 *
	 * Perguntador SEM conta (offline puro) vê todo pet com dono como
	 * `OtherAccount`: sem identidade própria, "meu" não existe — e é o certo,
	 * porque offline não rouba de ninguém de verdade (o roubo é do servidor).
	 */
	BATTLESQUARE_API EOwnerRelation RelationOf(
		const FString& OwnerAccountId, const FString& AskerAccountId);
}
