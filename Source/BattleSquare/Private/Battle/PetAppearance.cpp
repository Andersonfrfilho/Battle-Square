// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetAppearance.h"

#include "Balance/PetTypeCatalog.h"
#include "Balance/PetTypeIdentity.h"

namespace
{
	/**
	 * Cachorro não entra no sistema de dois eixos: ele não é escola nem
	 * elemento. É o pet do espelho de fixture, e existe para provar que a
	 * tabela de aparência chega à tela (L-045).
	 */
	const FString TipoCachorro = TEXT("Dog");
}

FPetAppearance FPetAppearance::ForType(const FString& PetType)
{
	FPetAppearance Aparencia;

	if (PetType.Equals(TipoCachorro, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.62f, 0.42f, 0.22f);
		Aparencia.CrestShape = EPetCrestShape::OrelhaCaida;
		Aparencia.CrestScale = FVector(0.13f, 0.13f, 0.34f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -72.0f);
		return Aparencia;
	}

	const FPetTypeCatalog& Catalogo = FPetTypeCatalog::Get();
	const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(PetType);

	const FPetSchoolDefinition* Escola = Catalogo.FindSchool(Identidade.School);
	const FPetElementDefinition* Elemento = Catalogo.FindElement(Identidade.Element);

	if (!Escola || !Elemento)
	{
		// Tipo irreconhecível fica NEUTRO, e neutro precisa ser VISÍVEL: o
		// ouro-palha não aparece em terreno nenhum, então um pet mal
		// cadastrado se destaca em vez de sumir no chão.
		Aparencia.AccentColor = FLinearColor(0.79f, 0.64f, 0.15f);
		Aparencia.CrestShape = EPetCrestShape::Orelha;
		return Aparencia;
	}

	// A SILHUETA é a ESCOLA: a forma diz o que aquele bicho FAZ com a magia
	// dele — dano, terreno ou atributo — e é a informação que decide o turno.
	Aparencia.CrestShape = Escola->Crest;
	Aparencia.CrestScale = Escola->CrestScale;

	// A COR e o TOMBO são o ELEMENTO, e são DOIS canais de propósito: quem não
	// distingue matiz lê a inclinação, e vice-versa. Um só deixaria metade dos
	// jogadores sem a informação.
	Aparencia.AccentColor = Elemento->Color;
	Aparencia.CrestRotation = FRotator(0.0f, 0.0f, Elemento->TiltDegrees);

	return Aparencia;
}
