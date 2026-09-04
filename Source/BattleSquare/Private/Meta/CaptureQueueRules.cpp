// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/CaptureQueueRules.h"

namespace
{
	FPendingCapture* AchaPorCatalogo(TArray<FPendingCapture>& Fila, const FString& CatalogId)
	{
		return Fila.FindByPredicate(
			[&CatalogId](const FPendingCapture& Item)
			{
				return Item.CatalogId == CatalogId;
			});
	}
}

void CaptureQueueRules::Enqueue(TArray<FPendingCapture>& Fila,
	const FString& CatalogId, const FString& IdempotencyKey)
{
	if (AchaPorCatalogo(Fila, CatalogId) != nullptr)
	{
		return;
	}

	FPendingCapture Nova;
	Nova.CatalogId = CatalogId;
	Nova.IdempotencyKey = IdempotencyKey;
	Nova.AttemptCount = 0;
	Fila.Add(Nova);
}

void CaptureQueueRules::MarkSent(TArray<FPendingCapture>& Fila, const FString& CatalogId)
{
	Fila.RemoveAll(
		[&CatalogId](const FPendingCapture& Item)
		{
			return Item.CatalogId == CatalogId;
		});
}

bool CaptureQueueRules::RegisterAttempt(TArray<FPendingCapture>& Fila, const FString& CatalogId)
{
	FPendingCapture* Item = AchaPorCatalogo(Fila, CatalogId);
	if (Item == nullptr)
	{
		return false;
	}

	++Item->AttemptCount;
	return Item->AttemptCount < MaxAttempts;
}

bool CaptureQueueRules::IsExhausted(const FPendingCapture& Captura)
{
	return Captura.AttemptCount >= MaxAttempts;
}

TArray<FPendingCapture> CaptureQueueRules::Sendable(const TArray<FPendingCapture>& Fila)
{
	return Fila.FilterByPredicate(
		[](const FPendingCapture& Item)
		{
			return !IsExhausted(Item);
		});
}

TArray<FPendingCapture> CaptureQueueRules::Exhausted(const TArray<FPendingCapture>& Fila)
{
	return Fila.FilterByPredicate(
		[](const FPendingCapture& Item)
		{
			return IsExhausted(Item);
		});
}
