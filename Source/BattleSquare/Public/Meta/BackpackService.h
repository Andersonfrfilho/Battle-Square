// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Meta/PetCollectionSaveGame.h"

/**
 * A MOCHILA e o que está vestido — as duas metades do mesmo item.
 *
 * ## A regra que este arquivo existe para garantir
 *
 * **O total nunca muda.** Equipar TIRA da mochila; desequipar DEVOLVE. Um item
 * que ficasse nos dois lugares existiria duas vezes, e a mesma bota vestiria
 * cinco pets — que é o defeito que a decisão do usuário ("carregamos na
 * mochila, a não ser que esteja equipado com o pet") descreve ao contrário.
 *
 * Puro sobre o save em memória, como `CaptureIfNewInMemory`: assim a regra tem
 * teste sem tocar em disco, e o disco é só o chamador.
 */
class BATTLESQUARE_API FBackpackService
{
public:
	/**
	 * Quantos deste item há NA MOCHILA. Vestido não conta.
	 *
	 * Não conta de propósito: a pergunta que quem abre a mochila faz é "o que
	 * eu tenho para dar a alguém", e o que já está vestido não está disponível.
	 */
	static int32 CountInBackpack(const UPetCollectionSaveGame* SaveGame,
		const FString& ItemId);

	/** Acrescenta à PILHA. Quantidade zero ou negativa não faz nada. */
	static void Add(UPetCollectionSaveGame* SaveGame,
		const FString& ItemId, int32 Quantity);

	/**
	 * VESTE um item num pet, tirando-o da mochila.
	 *
	 * Falso quando não há o item na mochila, quando o pet já encheu os slots,
	 * ou quando o save não existe. **Falha não consome nada** — meio equipar
	 * seria perder o item sem vestir.
	 */
	static bool Equip(UPetCollectionSaveGame* SaveGame,
		const FString& PetCatalogId, const FString& ItemId);

	/** TIRA e devolve à mochila. Falso quando aquele pet não veste aquilo. */
	static bool Unequip(UPetCollectionSaveGame* SaveGame,
		const FString& PetCatalogId, const FString& ItemId);

	/** O que este pet está vestindo, na ordem em que vestiu. */
	static TArray<FString> EquippedOn(const UPetCollectionSaveGame* SaveGame,
		const FString& PetCatalogId);

	/**
	 * USA um consumível da mochila: a quantidade cai; zerou, a pilha sai.
	 *
	 * Falso quando não há o item, ou quando ele é EQUIPAMENTO — gastar uma
	 * bota seria a mecânica de um sistema respondendo pela do outro.
	 */
	static bool Consume(UPetCollectionSaveGame* SaveGame, const FString& ItemId);

	/**
	 * Quantos slots cada pet tem.
	 *
	 * Configuração, e não constante espalhada, pela mesma razão que o tamanho
	 * da grade é: um número cravado no código mente na primeira vez que alguém
	 * quiser um pet com mais espaço, e mente em silêncio.
	 */
	static int32 SlotsPerPet();
};
