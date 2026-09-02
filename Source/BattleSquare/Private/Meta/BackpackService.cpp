// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/BackpackService.h"

#include "Balance/ItemCatalog.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	/**
	 * Quatro é o padrão, e é configuração.
	 *
	 * Não é um número escolhido no vazio: com um slot só, a biologia decidiria
	 * tudo e o item viraria enfeite; com muitos, somar equipamentos anularia um
	 * fluido inteiro e a escolha de biologia perderia sentido. Quatro dá espaço
	 * para combinar sem tornar a soma irrelevante — e o teto real de quem
	 * balanceia é o `Clamp` de 100 da composição.
	 */
	constexpr int32 SlotsPadrao = 4;

	FBackpackStack* AcharPilha(UPetCollectionSaveGame* Save, const FString& ItemId)
	{
		return Save->Backpack.FindByPredicate([&ItemId](const FBackpackStack& Pilha)
		{
			return Pilha.ItemId.Equals(ItemId, ESearchCase::IgnoreCase);
		});
	}
}

int32 FBackpackService::SlotsPerPet()
{
	int32 Quantos = SlotsPadrao;
	GConfig->GetInt(TEXT("/Script/BattleSquare.Backpack"), TEXT("SlotsPerPet"),
		Quantos, GGameIni);

	// Piso de 1: zero slots deixaria o sistema inteiro inalcançável, e o
	// jogador não teria nada na tela dizendo por quê.
	return FMath::Max(1, Quantos);
}

int32 FBackpackService::CountInBackpack(const UPetCollectionSaveGame* SaveGame,
	const FString& ItemId)
{
	if (!SaveGame)
	{
		return 0;
	}

	for (const FBackpackStack& Pilha : SaveGame->Backpack)
	{
		if (Pilha.ItemId.Equals(ItemId, ESearchCase::IgnoreCase))
		{
			return Pilha.Quantity;
		}
	}

	return 0;
}

void FBackpackService::Add(UPetCollectionSaveGame* SaveGame,
	const FString& ItemId, int32 Quantity)
{
	if (!SaveGame || ItemId.IsEmpty() || Quantity <= 0)
	{
		return;
	}

	if (FBackpackStack* Pilha = AcharPilha(SaveGame, ItemId))
	{
		// SOMA na pilha que já existe, e não cria uma segunda linha do mesmo
		// item: duas linhas do mesmo id fariam a contagem depender de quantas
		// vezes alguém pegou, e não de quantos há.
		Pilha->Quantity += Quantity;
		return;
	}

	FBackpackStack Nova;
	Nova.ItemId = ItemId;
	Nova.Quantity = Quantity;
	SaveGame->Backpack.Add(MoveTemp(Nova));
}

bool FBackpackService::Equip(UPetCollectionSaveGame* SaveGame,
	const FString& PetCatalogId, const FString& ItemId)
{
	if (!SaveGame || PetCatalogId.IsEmpty() || ItemId.IsEmpty())
	{
		return false;
	}

	// TUDO O QUE PODE RECUSAR, ANTES DE MEXER EM QUALQUER COISA.
	//
	// Uma recusa depois de já ter tirado da mochila perderia o item sem
	// vesti-lo — e o jogador não teria como saber que o perdeu.
	FBackpackStack* Pilha = AcharPilha(SaveGame, ItemId);
	if (!Pilha || Pilha->Quantity <= 0)
	{
		return false;
	}

	if (EquippedOn(SaveGame, PetCatalogId).Num() >= SlotsPerPet())
	{
		return false;
	}

	--Pilha->Quantity;
	if (Pilha->Quantity <= 0)
	{
		SaveGame->Backpack.RemoveAll([&ItemId](const FBackpackStack& Qual)
		{
			return Qual.ItemId.Equals(ItemId, ESearchCase::IgnoreCase);
		});
	}

	FEquippedItem Vestido;
	Vestido.PetCatalogId = PetCatalogId;
	Vestido.ItemId = ItemId;
	SaveGame->Equipped.Add(MoveTemp(Vestido));

	return true;
}

bool FBackpackService::Unequip(UPetCollectionSaveGame* SaveGame,
	const FString& PetCatalogId, const FString& ItemId)
{
	if (!SaveGame)
	{
		return false;
	}

	const int32 Onde = SaveGame->Equipped.IndexOfByPredicate(
		[&PetCatalogId, &ItemId](const FEquippedItem& Vestido)
		{
			return Vestido.PetCatalogId.Equals(PetCatalogId, ESearchCase::IgnoreCase)
				&& Vestido.ItemId.Equals(ItemId, ESearchCase::IgnoreCase);
		});

	if (Onde == INDEX_NONE)
	{
		return false;
	}

	// UM, e não todos: `RemoveAll` tiraria as duas botas de quem veste duas
	// iguais, e a mochila receberia uma só — o total deixaria de bater.
	SaveGame->Equipped.RemoveAt(Onde);
	Add(SaveGame, ItemId, 1);

	return true;
}

TArray<FString> FBackpackService::EquippedOn(const UPetCollectionSaveGame* SaveGame,
	const FString& PetCatalogId)
{
	TArray<FString> Vestidos;
	if (!SaveGame)
	{
		return Vestidos;
	}

	for (const FEquippedItem& Vestido : SaveGame->Equipped)
	{
		if (Vestido.PetCatalogId.Equals(PetCatalogId, ESearchCase::IgnoreCase))
		{
			Vestidos.Add(Vestido.ItemId);
		}
	}

	return Vestidos;
}

bool FBackpackService::Consume(UPetCollectionSaveGame* SaveGame, const FString& ItemId)
{
	if (!SaveGame)
	{
		return false;
	}

	const FItemDefinition* Definicao = FItemCatalog::Get().Find(ItemId);
	if (!Definicao || Definicao->Nature != EItemNature::Consumivel)
	{
		// Gastar um EQUIPAMENTO seria a mecânica de um sistema respondendo
		// pela do outro: a bota age enquanto vestida, e não some ao ser usada.
		return false;
	}

	FBackpackStack* Pilha = AcharPilha(SaveGame, ItemId);
	if (!Pilha || Pilha->Quantity <= 0)
	{
		return false;
	}

	--Pilha->Quantity;
	if (Pilha->Quantity <= 0)
	{
		SaveGame->Backpack.RemoveAll([&ItemId](const FBackpackStack& Qual)
		{
			return Qual.ItemId.Equals(ItemId, ESearchCase::IgnoreCase);
		});
	}

	return true;
}
