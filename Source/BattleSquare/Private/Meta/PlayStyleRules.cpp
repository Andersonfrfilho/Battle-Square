// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PlayStyleRules.h"

void PlayStyleRules::AddTransition(TArray<FStyleTransition>& Transitions,
	uint8 From, uint8 To)
{
	for (FStyleTransition& Transicao : Transitions)
	{
		if (Transicao.From == From && Transicao.To == To)
		{
			++Transicao.Count;
			return;
		}
	}

	FStyleTransition Nova;
	Nova.From = From;
	Nova.To = To;
	Nova.Count = 1;
	Transitions.Add(Nova);
}

int32 PlayStyleRules::TransitionCount(const TArray<FStyleTransition>& Transitions,
	uint8 From, uint8 To)
{
	for (const FStyleTransition& Transicao : Transitions)
	{
		if (Transicao.From == From && Transicao.To == To)
		{
			return Transicao.Count;
		}
	}

	return 0;
}
