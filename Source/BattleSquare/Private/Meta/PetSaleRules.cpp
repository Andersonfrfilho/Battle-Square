// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetSaleRules.h"

EPetSaleVerdict FPetSaleRules::TrySell(TArray<FOwnedPetInstance>& Collection,
	const FString& CatalogId, const FString& ActiveCatalogId)
{
	// A recusa do ativo vem ANTES da busca: se o id pedido é o do companheiro,
	// a resposta é "é o seu pet", mesmo que a coleção o tenha — dizer
	// "não tem" para um pet que o jogador está OLHANDO seria o painel mentindo.
	if (!ActiveCatalogId.IsEmpty() && CatalogId == ActiveCatalogId)
	{
		return EPetSaleVerdict::ActivePet;
	}

	const int32 Indice = Collection.IndexOfByPredicate(
		[&CatalogId](const FOwnedPetInstance& Pet)
		{
			return Pet.CatalogId == CatalogId;
		});

	if (Indice == INDEX_NONE)
	{
		return EPetSaleVerdict::NotOwned;
	}

	Collection.RemoveAt(Indice);
	return EPetSaleVerdict::Sold;
}
