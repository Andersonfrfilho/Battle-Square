// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/** Uma linha do painel do mundo, já pronta para desenhar. */
struct BATTLESQUARE_API FWorldStatusLine
{
	FText Text;
	FColor Color = FColor::White;
};

/** O que o mundo sabe agora, sem saber desenhar. */
struct BATTLESQUARE_API FWorldStatusSnapshot
{
	/** Falso quando o jogador ainda não tem pet na coleção. */
	bool bHasOwnedPet = false;
	FOwnedPetInstance OwnedPet;

	int32 EncountersAlive = 0;

	/** Negativo quando não há nenhum inimigo de quem medir distância. */
	float DistanceToNearestUnits = -1.0f;

	/**
	 * O que o pet do jogador está VESTINDO, pelo nome que se lê.
	 *
	 * Nomes, e não ids: `bota_de_lava` é o que o save guarda, e mostrá-lo na
	 * tela seria vazar a chave interna para quem joga.
	 */
	TArray<FString> EquippedItemNames;

	/** O que há na MOCHILA, já como "Nome ×3". */
	TArray<FString> BackpackLines;
};

/**
 * Traduz o estado do mundo nas linhas que o jogador lê.
 *
 * Pura: recebe um retrato, devolve texto. Não lê save, não varre o nível e não
 * desenha — por isso tem teste sem abrir o editor, como FBattleNarration.
 *
 * Existe porque a progressão só aparecia no FIM da batalha. Fora dela o
 * jogador andava por um mundo sem saber quem era o seu pet, o que tinha
 * conquistado, nem se havia alguém por perto — e a fatia de atributos vira
 * abstração se o único momento em que ela existe é uma linha que passa.
 */
class BATTLESQUARE_API FWorldStatusReadout
{
public:
	static TArray<FWorldStatusLine> Build(const FWorldStatusSnapshot& Snapshot);
};
