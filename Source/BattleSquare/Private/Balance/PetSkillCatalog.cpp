// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetSkillCatalog.h"

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
	if (const TArray<EActionType>* Encontradas = SkillsByType.Find(PetType))
	{
		return *Encontradas;
	}
	return {};
}

TArray<EActionType> FPetSkillCatalog::GetAvailableActionsForType(const FString& PetType) const
{
	TArray<EActionType> Disponiveis = GetUniversalActions();
	Disponiveis.Append(GetSkillsForType(PetType));
	return Disponiveis;
}
