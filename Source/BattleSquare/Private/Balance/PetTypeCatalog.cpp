// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetTypeCatalog.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/**
	 * NOME PRÓPRIO, e não `GTypeOverride`.
	 *
	 * Namespace anônimo NÃO isola em unity build: os arquivos viram uma unidade
	 * de tradução só, os anônimos se fundem, e três `GTypeOverride` de tipos
	 * diferentes viram uma redefinição. É a L-042 — que eu conhecia como regra
	 * de helper de TESTE — valendo para variável de PRODUÇÃO.
	 *
	 * E o modo de falhar é pior que não compilar: enquanto o agrupamento do
	 * unity build não os juntar, tudo compila e a instalação de um override
	 * pode acabar lida por OUTRO catálogo — que devolve um catálogo vazio, e o
	 * elemento "some" sem nada acusar.
	 */
	const FPetTypeCatalog* GTypeOverride = nullptr;

	/**
	 * Nome da malha da crista → a forma que o código conhece.
	 *
	 * Este mapa é o limite honesto do arquivo de configuração: uma malha nova
	 * é asset novo, e asset novo é código. Nome desconhecido devolve `Orelha`,
	 * que é a forma neutra — a escola aparece errada, mas aparece.
	 */
	EPetCrestShape FormaPorNome(const FString& Nome)
	{
		if (Nome.Equals(TEXT("Cristal"), ESearchCase::IgnoreCase)) { return EPetCrestShape::Cristal; }
		if (Nome.Equals(TEXT("Orbe"), ESearchCase::IgnoreCase))    { return EPetCrestShape::Orbe; }
		if (Nome.Equals(TEXT("Chama"), ESearchCase::IgnoreCase))   { return EPetCrestShape::Chama; }
		if (Nome.Equals(TEXT("Antena"), ESearchCase::IgnoreCase))  { return EPetCrestShape::Antena; }
		if (Nome.Equals(TEXT("Ponta"), ESearchCase::IgnoreCase))   { return EPetCrestShape::Ponta; }
		if (Nome.Equals(TEXT("Folha"), ESearchCase::IgnoreCase))   { return EPetCrestShape::Folha; }
		if (Nome.Equals(TEXT("Barbatana"), ESearchCase::IgnoreCase)) { return EPetCrestShape::Barbatana; }
		return EPetCrestShape::Orelha;
	}

	bool LerVetorDeTres(const TSharedPtr<FJsonObject>& Objeto, const TCHAR* Campo, FVector& Fora)
	{
		const TArray<TSharedPtr<FJsonValue>>* Lista = nullptr;
		if (!Objeto->TryGetArrayField(Campo, Lista) || !Lista || Lista->Num() != 3)
		{
			return false;
		}

		double Componentes[3] = { 0.0, 0.0, 0.0 };
		for (int32 Indice = 0; Indice < 3; ++Indice)
		{
			if (!(*Lista)[Indice].IsValid() || !(*Lista)[Indice]->TryGetNumber(Componentes[Indice]))
			{
				return false;
			}
		}

		Fora = FVector(Componentes[0], Componentes[1], Componentes[2]);
		return true;
	}
}

bool FPetTypeCatalog::LoadFromJson(const FString& FilePath, FPetTypeCatalog& OutCatalog)
{
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		return false;
	}

	FString Conteudo;
	if (!FFileHelper::LoadFileToString(Conteudo, *FilePath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> Raiz;
	const TSharedRef<TJsonReader<>> Leitor = TJsonReaderFactory<>::Create(Conteudo);
	if (!FJsonSerializer::Deserialize(Leitor, Raiz) || !Raiz.IsValid())
	{
		return false;
	}

	FPetTypeCatalog Lido;

	const TArray<TSharedPtr<FJsonValue>>* Escolas = nullptr;
	if (Raiz->TryGetArrayField(TEXT("schools"), Escolas) && Escolas)
	{
		for (const TSharedPtr<FJsonValue>& Valor : *Escolas)
		{
			const TSharedPtr<FJsonObject>* Objeto = nullptr;
			if (!Valor.IsValid() || !Valor->TryGetObject(Objeto) || !Objeto)
			{
				return false;
			}

			FPetSchoolDefinition Escola;
			if (!(*Objeto)->TryGetStringField(TEXT("name"), Escola.Name))
			{
				return false;
			}

			FString NomeDaMalha;
			(*Objeto)->TryGetStringField(TEXT("crest"), NomeDaMalha);
			Escola.Crest = FormaPorNome(NomeDaMalha);
			LerVetorDeTres(*Objeto, TEXT("crestScale"), Escola.CrestScale);

			Lido.Schools.Add(Escola);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Elementos = nullptr;
	if (Raiz->TryGetArrayField(TEXT("elements"), Elementos) && Elementos)
	{
		for (const TSharedPtr<FJsonValue>& Valor : *Elementos)
		{
			const TSharedPtr<FJsonObject>* Objeto = nullptr;
			if (!Valor.IsValid() || !Valor->TryGetObject(Objeto) || !Objeto)
			{
				return false;
			}

			FPetElementDefinition Elemento;
			if (!(*Objeto)->TryGetStringField(TEXT("name"), Elemento.Name))
			{
				return false;
			}

			FVector Cor = FVector::ZeroVector;
			if (LerVetorDeTres(*Objeto, TEXT("color"), Cor))
			{
				Elemento.Color = FLinearColor(Cor.X, Cor.Y, Cor.Z);
			}

			double Tombo = 0.0;
			(*Objeto)->TryGetNumberField(TEXT("tiltDegrees"), Tombo);
			Elemento.TiltDegrees = static_cast<float>(Tombo);

			// Ausente é ZERO: elemento que não diz nada não resiste a nada, e
			// esse é o inócuo — nunca uma imunidade dada por engano.
			const TSharedPtr<FJsonObject>* Resistencias = nullptr;
			if ((*Objeto)->TryGetObjectField(TEXT("resists"), Resistencias)
				&& Resistencias && Resistencias->IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Par
					: (*Resistencias)->Values)
				{
					double Quanto = 0.0;
					if (Par.Value.IsValid() && Par.Value->TryGetNumber(Quanto))
					{
						Elemento.FluidResists.Add(Par.Key,
							FMath::Clamp(static_cast<int32>(Quanto), 0, 100));
					}
				}
			}

			// Ausente é ZERO: elemento que não diz nada não se segura em nada.
			double Firmeza = 0.0;
			(*Objeto)->TryGetNumberField(TEXT("footing"), Firmeza);
			Elemento.FootingPerMille = FMath::Max(0, static_cast<int32>(Firmeza));

			// Ausente é falso: elemento que não diz nada não conduz.
			(*Objeto)->TryGetBoolField(TEXT("conducts"), Elemento.bConducts);

			const TArray<TSharedPtr<FJsonValue>>* Skills = nullptr;
			if ((*Objeto)->TryGetArrayField(TEXT("skills"), Skills) && Skills)
			{
				for (const TSharedPtr<FJsonValue>& Skill : *Skills)
				{
					FString Nome;
					if (Skill.IsValid() && Skill->TryGetString(Nome))
					{
						Elemento.SkillNames.Add(Nome);
					}
				}
			}

			Lido.Elements.Add(Elemento);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Legados = nullptr;
	if (Raiz->TryGetArrayField(TEXT("legacyNames"), Legados) && Legados)
	{
		for (const TSharedPtr<FJsonValue>& Valor : *Legados)
		{
			const TSharedPtr<FJsonObject>* Objeto = nullptr;
			if (!Valor.IsValid() || !Valor->TryGetObject(Objeto) || !Objeto)
			{
				return false;
			}

			FPetLegacyTypeName Legado;
			if (!(*Objeto)->TryGetStringField(TEXT("name"), Legado.Name)
				|| !(*Objeto)->TryGetStringField(TEXT("school"), Legado.School)
				|| !(*Objeto)->TryGetStringField(TEXT("element"), Legado.Element))
			{
				return false;
			}
			Lido.LegacyNames.Add(Legado);
		}
	}

	// Arquivo sem escola E sem elemento não é catálogo: aceitar devolveria um
	// jogo sem tipos fingindo que carregou.
	if (Lido.IsEmpty())
	{
		return false;
	}

	OutCatalog = Lido;
	return true;
}

const FPetTypeCatalog& FPetTypeCatalog::Get()
{
	if (GTypeOverride)
	{
		return *GTypeOverride;
	}

	static FPetTypeCatalog Carregado;
	static bool bTentou = false;

	if (!bTentou)
	{
		bTentou = true;
		LoadFromJson(FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetTypes.json")), Carregado);
	}

	return Carregado;
}

void FPetTypeCatalog::OverrideForTesting(const FPetTypeCatalog* Catalog)
{
	GTypeOverride = Catalog;
}

const FPetElementDefinition* FPetTypeCatalog::FindElement(const FString& Name) const
{
	return Elements.FindByPredicate([&Name](const FPetElementDefinition& Item)
	{
		return Item.Name.Equals(Name, ESearchCase::IgnoreCase);
	});
}

const FPetSchoolDefinition* FPetTypeCatalog::FindSchool(const FString& Name) const
{
	return Schools.FindByPredicate([&Name](const FPetSchoolDefinition& Item)
	{
		return Item.Name.Equals(Name, ESearchCase::IgnoreCase);
	});
}

bool FPetTypeCatalog::ResolveLegacyName(const FString& Name, FString& OutSchool, FString& OutElement) const
{
	const FPetLegacyTypeName* Encontrado = LegacyNames.FindByPredicate(
		[&Name](const FPetLegacyTypeName& Item)
		{
			return Item.Name.Equals(Name, ESearchCase::IgnoreCase);
		});

	if (!Encontrado)
	{
		return false;
	}

	OutSchool = Encontrado->School;
	OutElement = Encontrado->Element;
	return true;
}
