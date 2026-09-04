// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/OwnershipCache.h"

OwnershipCache::FState OwnershipCache::AfterSuccessfulFetch(
	const TArray<FKnownPet>& FromServer, double NowSeconds)
{
	FState Novo;
	Novo.Pets = FromServer;
	Novo.LastSyncSeconds = NowSeconds;
	return Novo;
}

OwnershipCache::FState OwnershipCache::AfterFailedFetch(const FState& Atual)
{
	// Devolve o que já havia, sem tocar na hora: falhar não é novidade sobre
	// a posse, é ausência de novidade.
	return Atual;
}

bool OwnershipCache::IsStale(const FState& Estado, double NowSeconds, double StaleAfterSeconds)
{
	// Nunca sincronizado não é "velho": é "ainda não" — e o jogo já sabe
	// disso por outro caminho (a conta pode nem estar configurada). Velho é o
	// que UM DIA foi fresco e passou do prazo.
	if (Estado.LastSyncSeconds < 0.0)
	{
		return false;
	}

	return (NowSeconds - Estado.LastSyncSeconds) > StaleAfterSeconds;
}
