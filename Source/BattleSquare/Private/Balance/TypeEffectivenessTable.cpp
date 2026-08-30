// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/TypeEffectivenessTable.h"
#include "Balance/PetTypeIdentity.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

namespace
{
	int32 PercentualDoEixo(const TMap<TPair<FString, FString>, int32>& Eixo,
		const FString& Atacante, const FString& Defensor)
	{
		if (Atacante.IsEmpty() || Defensor.IsEmpty())
		{
			return 100;
		}

		const int32* Encontrado = Eixo.Find(TPair<FString, FString>(Atacante, Defensor));
		return Encontrado ? *Encontrado : 100;
	}
}

int32 FTypeEffectivenessTable::GetPercent(const FString& AttackerType, const FString& DefenderType) const
{
	// O par EXPLÍCITO vence os eixos. É a válvula de escape para a exceção que
	// toda tabela composta acaba precisando — e mantê-la explícita é o que
	// impede alguém de torcer os eixos inteiros para acomodar um caso.
	if (const int32* Found = Entries.Find(TPair<FString, FString>(AttackerType, DefenderType)))
	{
		return *Found;
	}

	return GetComposedPercent(AttackerType, DefenderType);
}

int32 FTypeEffectivenessTable::GetComposedPercent(const FString& AttackerType, const FString& DefenderType) const
{
	const FPetTypeIdentity Atacante = FPetTypeIdentity::Parse(AttackerType);
	const FPetTypeIdentity Defensor = FPetTypeIdentity::Parse(DefenderType);

	const int32 PorEscola = PercentualDoEixo(SchoolEntries, Atacante.School, Defensor.School);
	const int32 PorElemento = PercentualDoEixo(ElementEntries, Atacante.Element, Defensor.Element);

	return (PorEscola * PorElemento) / 100;
}

void FTypeEffectivenessTable::SetPercent(const FString& AttackerType, const FString& DefenderType, int32 Percent)
{
	Entries.Add(TPair<FString, FString>(AttackerType, DefenderType), Percent);
}

bool FTypeEffectivenessTable::LoadFromJson(const FString& FilePath, FTypeEffectivenessTable& OutTable)
{
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		return false;
	}

	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *FilePath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	FTypeEffectivenessTable ParsedTable;

	// Os dois EIXOS são o caminho normal; `effectiveness` virou a exceção
	// nomeada. Ler os eixos primeiro deixa o arquivo antigo — que só tem
	// `effectiveness` — continuar carregando exatamente como antes.
	auto LerEixo = [&RootObject](const TCHAR* Campo, TMap<TPair<FString, FString>, int32>& Fora) -> bool
	{
		const TArray<TSharedPtr<FJsonValue>>* Lista = nullptr;
		if (!RootObject->TryGetArrayField(Campo, Lista) || !Lista)
		{
			return true; // ausente é legítimo: o arquivo pode não usar este eixo
		}

		for (const TSharedPtr<FJsonValue>& Item : *Lista)
		{
			const TSharedPtr<FJsonObject>* Objeto = nullptr;
			if (!Item.IsValid() || !Item->TryGetObject(Objeto) || !Objeto)
			{
				return false;
			}

			FString Atacante;
			FString Defensor;
			int32 Percentual = 100;
			if (!(*Objeto)->TryGetStringField(TEXT("attacker"), Atacante)
				|| !(*Objeto)->TryGetStringField(TEXT("defender"), Defensor)
				|| !(*Objeto)->TryGetNumberField(TEXT("percent"), Percentual))
			{
				return false;
			}

			Fora.Add(TPair<FString, FString>(Atacante, Defensor), Percentual);
		}
		return true;
	};

	if (!LerEixo(TEXT("schools"), ParsedTable.SchoolEntries)
		|| !LerEixo(TEXT("elements"), ParsedTable.ElementEntries))
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* EffectivenessArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("effectiveness"), EffectivenessArray) || !EffectivenessArray)
	{
		// Sem a lista de exceções, os eixos bastam — desde que exista ao menos
		// um deles. Arquivo sem nenhum dos três não é tabela nenhuma, e aceitar
		// devolveria combate neutro fingindo que carregou.
		OutTable = ParsedTable;
		return ParsedTable.SchoolEntries.Num() > 0 || ParsedTable.ElementEntries.Num() > 0;
	}

	for (const TSharedPtr<FJsonValue>& Entry : *EffectivenessArray)
	{
		const TSharedPtr<FJsonObject>* EntryObject = nullptr;
		if (!Entry.IsValid() || !Entry->TryGetObject(EntryObject) || !EntryObject)
		{
			return false;
		}

		FString AttackerType;
		FString DefenderType;
		int32 Percent = 100;
		if (!(*EntryObject)->TryGetStringField(TEXT("attacker"), AttackerType)
			|| !(*EntryObject)->TryGetStringField(TEXT("defender"), DefenderType)
			|| !(*EntryObject)->TryGetNumberField(TEXT("percent"), Percent))
		{
			return false;
		}

		ParsedTable.SetPercent(AttackerType, DefenderType, Percent);
	}

	OutTable = ParsedTable;
	return true;
}
