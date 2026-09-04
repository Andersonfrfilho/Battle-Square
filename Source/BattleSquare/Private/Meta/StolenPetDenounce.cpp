// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/StolenPetDenounce.h"

bool StolenPetDenounce::ShouldWarn(const FObserverContext& Contexto)
{
	// Não é roubado: não há o que denunciar. A primeira porta, e a mais
	// simples — a maioria das batalhas para aqui.
	if (!Contexto.bCarrierPetIsStolen)
	{
		return false;
	}

	// QUEM PODE RECONHECER: a vítima (sempre) ou quem viu a lista. Um jogador
	// qualquer, que nunca viu o cartaz nem foi roubado, NÃO recebe aviso — é
	// o caso de assédio que a invariante 19 existe para barrar.
	if (!Contexto.bObserverIsVictim && !Contexto.bObserverSawWantedList)
	{
		return false;
	}

	// E o portador tem de ser RECONHECÍVEL como o procurado: quem trocou de
	// rosto despistou (decisão 23), e a marca deixou de identificá-lo. Sem
	// isto, o aviso perseguiria quem já se disfarçou — a marca vira perpétua,
	// que a própria spec proíbe.
	return CharacterRecognition::IsRecognized(
		Contexto.PosterAppearance, Contexto.CarrierAppearanceNow);
}
