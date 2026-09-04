// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetOwnershipQuery.h"

PetOwnershipQuery::EOwnerRelation PetOwnershipQuery::RelationOf(
	const FString& OwnerAccountId, const FString& AskerAccountId)
{
	// Vazio é SELVAGEM, e vem antes de tudo: um pet sem dono não é "de
	// ninguém em particular", é do mundo — e comparar vazio com vazio (dois
	// offline) NÃO pode dar "meu", senão todo bicho do mundo viraria seu.
	if (OwnerAccountId.IsEmpty())
	{
		return EOwnerRelation::Wild;
	}

	// O contrapeso da task: o próprio pet, visto pelo próprio dono, responde
	// "seu" — inclusive depois de uma reconexão, porque a conta é a mesma.
	if (!AskerAccountId.IsEmpty() && OwnerAccountId == AskerAccountId)
	{
		return EOwnerRelation::Self;
	}

	return EOwnerRelation::OtherAccount;
}
