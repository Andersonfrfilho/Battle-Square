// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * Quantas especialidades o treinador pode ter, e o que acontece ao aprender.
 *
 * Pura sobre o perfil: não lê save, não conhece ator. A ESCASSEZ é a regra —
 * ela espelha no jogador o que os atributos fazem no pet, e é o que impede
 * dois veteranos de serem idênticos.
 */
class BATTLESQUARE_API FTrainerSpecialtyRules
{
public:
	/**
	 * O teto. Dois, e não cinco: com cinco não haveria escolha nenhuma, e o
	 * "nunca completo" que a decisão pediu deixaria de valer.
	 */
	static constexpr int32 MaxSpecialties = 2;

	static bool IsSpecialistIn(const FTrainerProfile& Profile, const FString& Attribute);

	/** Vagas que restam. Zero significa escolha feita, e ela não se desfaz. */
	static int32 FreeSlots(const FTrainerProfile& Profile);

	/**
	 * Aprende, se puder. Devolve false — sem efeito — quando já sabe, quando
	 * não há vaga, ou quando o atributo não existe.
	 *
	 * NÃO existe troca. Não porque seja o certo, mas porque é a metade
	 * reversível: dar a troca depois é acrescentar, tirá-la depois seria
	 * retirar algo que o jogador já usou. A decisão de PRODUTO (trocar nunca,
	 * de graça, ou com custo) segue aberta — ver DP-atr-11.
	 */
	static bool TryLearn(FTrainerProfile& Profile, const FString& Attribute);
};
