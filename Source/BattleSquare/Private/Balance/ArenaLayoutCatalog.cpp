// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/ArenaLayoutCatalog.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

bool FArenaLayoutCatalog::GetLayoutByName(const FString& ArenaName, TArray<uint8>& OutLayout) const
{
	if (const TArray<uint8>* Found = Layouts.Find(ArenaName))
	{
		OutLayout = *Found;
		return true;
	}
	return false;
}

bool FArenaLayoutCatalog::LoadFromJson(const FString& FilePath, FArenaLayoutCatalog& OutCatalog)
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

	const TArray<TSharedPtr<FJsonValue>>* ArenasArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("arenas"), ArenasArray) || !ArenasArray)
	{
		return false;
	}

	FArenaLayoutCatalog ParsedCatalog;
	for (const TSharedPtr<FJsonValue>& ArenaValue : *ArenasArray)
	{
		const TSharedPtr<FJsonObject>* ArenaObject = nullptr;
		if (!ArenaValue.IsValid() || !ArenaValue->TryGetObject(ArenaObject) || !ArenaObject)
		{
			return false;
		}

		FString ArenaName;
		if (!(*ArenaObject)->TryGetStringField(TEXT("name"), ArenaName))
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* LayoutArray = nullptr;
		if (!(*ArenaObject)->TryGetArrayField(TEXT("layout"), LayoutArray) || !LayoutArray
			|| LayoutArray->Num() != BattleGridCellCount)
		{
			return false;
		}

		TArray<uint8> Layout;
		Layout.Reserve(BattleGridCellCount);
		for (const TSharedPtr<FJsonValue>& CellValue : *LayoutArray)
		{
			int32 CellProperty = 0;
			if (!CellValue.IsValid() || !CellValue->TryGetNumber(CellProperty))
			{
				return false;
			}
			Layout.Add(static_cast<uint8>(CellProperty));
		}

		ParsedCatalog.Layouts.Add(ArenaName, Layout);
	}

	OutCatalog = ParsedCatalog;
	return true;
}
