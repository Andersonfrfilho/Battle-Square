// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetSkillCatalog.h"
#include "Balance/PetTypeCatalog.h"
#include "Balance/PetTypeIdentity.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

EActionType FPetSkillCatalog::SkillFromName(const FString& SkillName)
{
	if (SkillName.Equals(TEXT("camuflar"), ESearchCase::IgnoreCase))
	{
		return EActionType::Camuflar;
	}
	if (SkillName.Equals(TEXT("voar"), ESearchCase::IgnoreCase))
	{
		return EActionType::Voar;
	}
	if (SkillName.Equals(TEXT("submergir"), ESearchCase::IgnoreCase))
	{
		return EActionType::Submergir;
	}

	if (SkillName.Equals(TEXT("atravessar"), ESearchCase::IgnoreCase))
	{
		return EActionType::Atravessar;
	}
	if (SkillName.Equals(TEXT("iluminar"), ESearchCase::IgnoreCase))
	{
		return EActionType::Iluminar;
	}

	if (SkillName.Equals(TEXT("escavar"), ESearchCase::IgnoreCase))
	{
		return EActionType::Escavar;
	}

	// Nome desconhecido não vira ação nenhuma. Mapear para Aguardar daria ao
	// pet uma ação que ninguém pediu, e o erro de digitação passaria calado.
	return EActionType::Aguardar;
}

const TArray<EActionType>& FPetSkillCatalog::GetUniversalActions()
{
	// DP-skill-03: a gramática do combate, que todo pet fala. Skills são o que
	// distingue um pet de outro; tornar estes condicionais mudaria o jogo.
	static const TArray<EActionType> Universais = {
		EActionType::Aguardar,
		EActionType::Mover,
		EActionType::Atacar,
		EActionType::Magia,
		EActionType::Defender,
		EActionType::Esquivar
	};
	return Universais;
}

FPetSkillCatalog FPetSkillCatalog::FromTypeCatalog(const FPetTypeCatalog& Types)
{
	// As skills passaram a morar em `PetTypes.json`, junto do elemento que as
	// tem. Um segundo arquivo declarando skill por tipo era a última cópia da
	// lista de elementos — e cópias concordam até a primeira edição, que aqui
	// significaria um elemento existindo para a cor e não para a skill.
	FPetSkillCatalog Catalogo;
	for (const FPetElementDefinition& Elemento : Types.GetElements())
	{
		TArray<EActionType> Acoes;
		for (const FString& Nome : Elemento.SkillNames)
		{
			const EActionType Acao = SkillFromName(Nome);
			if (Acao != EActionType::Aguardar)
			{
				Acoes.Add(Acao);
			}
		}
		Catalogo.SkillsByType.Add(Elemento.Name, Acoes);
	}
	return Catalogo;
}

bool FPetSkillCatalog::LoadFromJson(const FString& FilePath, FPetSkillCatalog& OutCatalog)
{
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

	const TArray<TSharedPtr<FJsonValue>>* Tipos = nullptr;
	if (!Raiz->TryGetArrayField(TEXT("types"), Tipos))
	{
		return false;
	}

	OutCatalog.SkillsByType.Reset();

	for (const TSharedPtr<FJsonValue>& Valor : *Tipos)
	{
		const TSharedPtr<FJsonObject>* Entrada = nullptr;
		if (!Valor.IsValid() || !Valor->TryGetObject(Entrada))
		{
			continue;
		}

		FString NomeDoTipo;
		if (!(*Entrada)->TryGetStringField(TEXT("type"), NomeDoTipo))
		{
			continue;
		}

		TArray<EActionType> Skills;
		const TArray<TSharedPtr<FJsonValue>>* NomesDeSkill = nullptr;
		if ((*Entrada)->TryGetArrayField(TEXT("skills"), NomesDeSkill))
		{
			for (const TSharedPtr<FJsonValue>& NomeValor : *NomesDeSkill)
			{
				const EActionType Skill = SkillFromName(NomeValor->AsString());
				if (Skill != EActionType::Aguardar)
				{
					Skills.AddUnique(Skill);
				}
			}
		}

		OutCatalog.SkillsByType.Add(NomeDoTipo, MoveTemp(Skills));
	}

	return true;
}

TArray<EActionType> FPetSkillCatalog::GetSkillsForType(const FString& PetType) const
{
	// O nome EXATO vem primeiro: um arquivo que declare `Natural/Fogo` — ou o
	// `Dog` do espelho de fixture — precisa continuar sendo encontrado.
	if (const TArray<EActionType>* Encontradas = SkillsByType.Find(PetType))
	{
		return *Encontradas;
	}

	// Depois, pelo ELEMENTO. A skill é coisa do corpo, e o corpo é feito do
	// elemento: `Fisica/Fogo` e `Psiquica/Fogo` voam pelo mesmo motivo, que é
	// serem os dois de fogo. Ligar a skill à escola faria todo psíquico voar
	// por nenhum motivo.
	const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(PetType);
	if (!Identidade.Element.IsEmpty())
	{
		if (const TArray<EActionType>* PorElemento = SkillsByType.Find(Identidade.Element))
		{
			return *PorElemento;
		}
	}

	return {};
}

TArray<EActionType> FPetSkillCatalog::GetAvailableActionsForType(const FString& PetType) const
{
	TArray<EActionType> Disponiveis = GetUniversalActions();
	Disponiveis.Append(GetSkillsForType(PetType));
	return Disponiveis;
}
