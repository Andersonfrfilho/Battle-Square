// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetBiologyCatalog.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FPetBiologyCatalog* GOverride = nullptr;

	/**
	 * Um eixo inteiro, do JSON.
	 *
	 * UM leitor para os quatro eixos: quatro leitores quase iguais dariam
	 * quatro chances de esquecer um campo, e a quarta cópia é sempre a que
	 * esquece.
	 */
	void LerEixo(const TSharedPtr<FJsonObject>& Raiz, const TCHAR* Campo,
		TArray<FPetBiologyTrait>& Fora)
	{
		const TArray<TSharedPtr<FJsonValue>>* Lista = nullptr;
		if (!Raiz->TryGetArrayField(Campo, Lista) || !Lista)
		{
			return;
		}

		for (const TSharedPtr<FJsonValue>& Valor : *Lista)
		{
			const TSharedPtr<FJsonObject>* Objeto = nullptr;
			if (!Valor.IsValid() || !Valor->TryGetObject(Objeto) || !Objeto)
			{
				continue;
			}

			FPetBiologyTrait Traco;
			if (!(*Objeto)->TryGetStringField(TEXT("nome"), Traco.Name)
				|| Traco.Name.IsEmpty())
			{
				// Traço sem nome é traço que ninguém consegue escolher: entrar
				// no catálogo só o faria aparecer numa contagem.
				continue;
			}

			int32 Firmeza = 0;
			if ((*Objeto)->TryGetNumberField(TEXT("firmeza"), Firmeza))
			{
				Traco.FootingPerMille = Firmeza;
			}

			(*Objeto)->TryGetBoolField(TEXT("submergeSemCusto"), Traco.bBreathesUnderwater);

			const TSharedPtr<FJsonObject>* Resiste = nullptr;
			if ((*Objeto)->TryGetObjectField(TEXT("resiste"), Resiste) && Resiste)
			{
				for (const auto& Par : (*Resiste)->Values)
				{
					double Quanto = 0.0;
					if (Par.Value.IsValid() && Par.Value->TryGetNumber(Quanto))
					{
						Traco.FluidResists.Add(FString(Par.Key), static_cast<int32>(Quanto));
					}
				}
			}

			Fora.Add(MoveTemp(Traco));
		}
	}

	const FPetBiologyTrait* Achar(const TArray<FPetBiologyTrait>& Onde, const FString& Nome)
	{
		if (Nome.IsEmpty())
		{
			return nullptr;
		}

		return Onde.FindByPredicate([&Nome](const FPetBiologyTrait& Item)
		{
			return Item.Name.Equals(Nome, ESearchCase::IgnoreCase);
		});
	}
}

const FPetBiologyCatalog& FPetBiologyCatalog::Get()
{
	if (GOverride)
	{
		return *GOverride;
	}

	static FPetBiologyCatalog Carregado;
	static bool bTentou = false;

	if (!bTentou)
	{
		bTentou = true;

		FString Texto;
		const FString Caminho =
			FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetBiology.json"));

		if (FFileHelper::LoadFileToString(Texto, *Caminho))
		{
			TSharedPtr<FJsonObject> Raiz;
			const TSharedRef<TJsonReader<>> Leitor = TJsonReaderFactory<>::Create(Texto);
			if (FJsonSerializer::Deserialize(Leitor, Raiz) && Raiz.IsValid())
			{
				LerEixo(Raiz, TEXT("pele"), Carregado.Skins);
				LerEixo(Raiz, TEXT("porte"), Carregado.Builds);
				LerEixo(Raiz, TEXT("respiracao"), Carregado.Breathings);
				LerEixo(Raiz, TEXT("apoio"), Carregado.Limbs);
			}
		}
	}

	return Carregado;
}

void FPetBiologyCatalog::OverrideForTesting(const FPetBiologyCatalog* Catalog)
{
	GOverride = Catalog;
}

const FPetBiologyTrait* FPetBiologyCatalog::FindSkin(const FString& Name) const
{
	return Achar(Skins, Name);
}

const FPetBiologyTrait* FPetBiologyCatalog::FindBuild(const FString& Name) const
{
	return Achar(Builds, Name);
}

const FPetBiologyTrait* FPetBiologyCatalog::FindBreathing(const FString& Name) const
{
	return Achar(Breathings, Name);
}

const FPetBiologyTrait* FPetBiologyCatalog::FindLimbs(const FString& Name) const
{
	return Achar(Limbs, Name);
}

int32 FPetBiologyCatalog::ResistFor(const FPetBiology& Biology, const FString& FluidName) const
{
	int32 Soma = 0;

	const FPetBiologyTrait* Quatro[4] = {
		FindSkin(Biology.Skin), FindBuild(Biology.Build),
		FindBreathing(Biology.Breathing), FindLimbs(Biology.Limbs) };

	for (const FPetBiologyTrait* Traco : Quatro)
	{
		if (!Traco)
		{
			continue;
		}

		if (const int32* Quanto = Traco->FluidResists.Find(FluidName))
		{
			Soma += *Quanto;
		}
	}

	return Soma;
}

int32 FPetBiologyCatalog::FootingFor(const FPetBiology& Biology) const
{
	int32 Soma = 0;

	const FPetBiologyTrait* Quatro[4] = {
		FindSkin(Biology.Skin), FindBuild(Biology.Build),
		FindBreathing(Biology.Breathing), FindLimbs(Biology.Limbs) };

	for (const FPetBiologyTrait* Traco : Quatro)
	{
		if (Traco)
		{
			Soma += Traco->FootingPerMille;
		}
	}

	return Soma;
}

bool FPetBiologyCatalog::BreathesUnderwater(const FPetBiology& Biology) const
{
	const FPetBiologyTrait* Quatro[4] = {
		FindSkin(Biology.Skin), FindBuild(Biology.Build),
		FindBreathing(Biology.Breathing), FindLimbs(Biology.Limbs) };

	for (const FPetBiologyTrait* Traco : Quatro)
	{
		if (Traco && Traco->bBreathesUnderwater)
		{
			return true;
		}
	}

	return false;
}
