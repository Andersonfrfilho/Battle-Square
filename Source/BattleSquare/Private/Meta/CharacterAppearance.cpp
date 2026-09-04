// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/CharacterAppearance.h"

int32 CharacterRecognition::MatchingTraits(
	const FCharacterAppearance& Cartaz, const FCharacterAppearance& Agora)
{
	int32 Batem = 0;
	Batem += (Cartaz.HairStyle == Agora.HairStyle) ? 1 : 0;
	Batem += (Cartaz.HairColor == Agora.HairColor) ? 1 : 0;
	Batem += (Cartaz.Outfit == Agora.Outfit) ? 1 : 0;
	Batem += (Cartaz.Face == Agora.Face) ? 1 : 0;
	return Batem;
}

bool CharacterRecognition::IsRecognized(
	const FCharacterAppearance& Cartaz, const FCharacterAppearance& Agora)
{
	// O ROSTO É A ÂNCORA: sem ele igual, não há reconhecimento — trocar de
	// rosto é o disfarce que a decisão 23 aceita ("a ponto de não ser
	// identificado"). Roupa e cabelo novos com o mesmo rosto NÃO enganam.
	if (Cartaz.Face != Agora.Face)
	{
		return false;
	}

	// Mas rosto sozinho não condena: precisa de mais um traço batendo, senão
	// dois desconhecidos de rosto parecido seriam a mesma pessoa. Como o rosto
	// já conta 1, exige-se ao menos 2 no total.
	return MatchingTraits(Cartaz, Agora) >= 2;
}
