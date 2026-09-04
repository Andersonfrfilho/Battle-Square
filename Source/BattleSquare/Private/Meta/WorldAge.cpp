// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/WorldAge.h"

#define LOCTEXT_NAMESPACE "WorldAge"

WorldAge::FWorldAge WorldAge::Unknown()
{
	return FWorldAge{ /*bKnown*/ false, /*AgeInDays*/ 0 };
}

WorldAge::FWorldAge WorldAge::Known(int32 AgeInDays)
{
	return FWorldAge{ /*bKnown*/ true, FMath::Max(0, AgeInDays) };
}

FText WorldAge::Describe(const FWorldAge& Age)
{
	if (!Age.bKnown)
	{
		// O fallback visivel da task: nunca um numero, nunca zero silencioso.
		return LOCTEXT("IdadeDesconhecida", "idade do mundo: desconhecida");
	}
	return FText::Format(
		LOCTEXT("IdadeConhecida", "idade do mundo: {Dias} dias"),
		FText::AsNumber(Age.AgeInDays));
}

#undef LOCTEXT_NAMESPACE
