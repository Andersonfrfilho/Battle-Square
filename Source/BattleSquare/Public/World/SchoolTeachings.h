// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PetDataLoader.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * O QUADRO DE LIÇÕES da Escola (decisão 64): o que o seu pet alcança, o que
 * ainda não, e onde treinar para alcançar.
 *
 * PURO, no formato de `FWorldStatusReadout`: recebe o registro e a instância,
 * devolve linhas. Não lê save, não carrega espelho, não desenha — e por isso
 * tem teste sem abrir o editor.
 *
 * Ele existe porque o golpe trancado só aparecia DENTRO da batalha — o botão
 * inerte com o cadeado. Descobrir o requisito no meio da luta é descobrir
 * tarde: a Escola é onde se lê ANTES, com calma, e com o dedo apontando o
 * campo do pátio que destrava.
 */
namespace SchoolTeachings
{
	/**
	 * Uma linha por golpe do registro. As TRANCADAS dizem o requisito e onde
	 * treinar; as alcançadas dizem só que se alcançou — lição não é lista de
	 * conquista.
	 */
	BATTLESQUARE_API TArray<FString> BuildLessonLines(
		const FLoadedPetRecord& Record, const FOwnedPetInstance& Owned);
}
