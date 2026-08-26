// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/TypeEffectivenessTable.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

int32 FTypeEffectivenessTable::GetPercent(const FString& AttackerType, const FString& DefenderType) const
{
	if (const int32* Found = Entries.Find(TPair<FString, FString>(AttackerType, DefenderType)))
	{
		return *Found;
	}
	return 100;
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

	const TArray<TSharedPtr<FJsonValue>>* EffectivenessArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("effectiveness"), EffectivenessArray) || !EffectivenessArray)
	{
		return false;
	}

	FTypeEffectivenessTable ParsedTable;
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
