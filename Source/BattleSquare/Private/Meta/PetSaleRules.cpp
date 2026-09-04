// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetSaleRules.h"

EPetSaleVerdict FPetSaleRules::TrySell(TArray<FOwnedPetInstance>& Collection,
	const FString& CatalogId, const FString& ActiveCatalogId,
	const TSet<FString>& StolenCatalogIds)
{
	// A recusa do ativo vem ANTES da busca: se o id pedido é o do companheiro,
	// a resposta é "é o seu pet", mesmo que a coleção o tenha — dizer
	// "não tem" para um pet que o jogador está OLHANDO seria o painel mentindo.
	if (!ActiveCatalogId.IsEmpty() && CatalogId == ActiveCatalogId)
	{
		return EPetSaleVerdict::ActivePet;
	}

	// ROUBADO NÃO SE VENDE (CR6), e a recusa vem antes de remover: o Mercado
	// comum é onde o roubo tentaria virar dinheiro limpo. A marca é do
	// servidor (o cache da posse), nunca uma segunda verdade local
	// (invariante 18) — quem chama passa o conjunto de roubados que o cache
	// conhece.
	if (StolenCatalogIds.Contains(CatalogId))
	{
		return EPetSaleVerdict::Stolen;
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
