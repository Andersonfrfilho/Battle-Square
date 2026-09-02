// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/ItemCatalog.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FItemCatalog* GOverride = nullptr;

	/**
	 * Natureza desconhecida vira EQUIPAMENTO, que é a inócua.
	 *
	 * Cair em consumível faria um erro de digitação no cadastro GASTAR o item
	 * do jogador — e ele não teria como saber por quê. Errar para o lado que
	 * não destrói nada é a única escolha defensável aqui.
	 */
	EItemNature NaturezaPorNome(const FString& Nome)
	{
		return Nome.Equals(TEXT("consumivel"), ESearchCase::IgnoreCase)
			? EItemNature::Consumivel
			: EItemNature::Equipamento;
	}
}

const FItemCatalog& FItemCatalog::Get()
{
	if (GOverride)
	{
		return *GOverride;
	}

	static FItemCatalog Carregado;
	static bool bTentou = false;

	if (!bTentou)
	{
		bTentou = true;

		FString Texto;
		const FString Caminho =
			FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Items.json"));

		if (FFileHelper::LoadFileToString(Texto, *Caminho))
		{
			TSharedPtr<FJsonObject> Raiz;
			const TSharedRef<TJsonReader<>> Leitor = TJsonReaderFactory<>::Create(Texto);

			const TArray<TSharedPtr<FJsonValue>>* Lista = nullptr;
			if (FJsonSerializer::Deserialize(Leitor, Raiz) && Raiz.IsValid()
				&& Raiz->TryGetArrayField(TEXT("itens"), Lista) && Lista)
			{
				for (const TSharedPtr<FJsonValue>& Valor : *Lista)
				{
					const TSharedPtr<FJsonObject>* Objeto = nullptr;
					if (!Valor.IsValid() || !Valor->TryGetObject(Objeto) || !Objeto)
					{
						continue;
					}

					FItemDefinition Item;
					if (!(*Objeto)->TryGetStringField(TEXT("id"), Item.Id)
						|| Item.Id.IsEmpty())
					{
						// Item sem id é item que o save não consegue guardar:
						// entrar no catálogo só o faria aparecer numa contagem.
						continue;
					}

					(*Objeto)->TryGetStringField(TEXT("nome"), Item.Name);

					FString Natureza;
					(*Objeto)->TryGetStringField(TEXT("natureza"), Natureza);
					Item.Nature = NaturezaPorNome(Natureza);

					int32 Firmeza = 0;
					if ((*Objeto)->TryGetNumberField(TEXT("firmeza"), Firmeza))
					{
						Item.FootingPerMille = Firmeza;
					}

					const TSharedPtr<FJsonObject>* Resiste = nullptr;
					if ((*Objeto)->TryGetObjectField(TEXT("resiste"), Resiste) && Resiste)
					{
						for (const auto& Par : (*Resiste)->Values)
						{
							double Quanto = 0.0;
							if (Par.Value.IsValid() && Par.Value->TryGetNumber(Quanto))
							{
								Item.FluidResists.Add(FString(Par.Key), static_cast<int32>(Quanto));
							}
						}
					}

					Carregado.Items.Add(MoveTemp(Item));
				}
			}
		}
	}

	return Carregado;
}

void FItemCatalog::OverrideForTesting(const FItemCatalog* Catalog)
{
	GOverride = Catalog;
}

const FItemDefinition* FItemCatalog::Find(const FString& Id) const
{
	if (Id.IsEmpty())
	{
		return nullptr;
	}

	return Items.FindByPredicate([&Id](const FItemDefinition& Item)
	{
		return Item.Id.Equals(Id, ESearchCase::IgnoreCase);
	});
}
