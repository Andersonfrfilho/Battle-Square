// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetAppearance.h"

#include "Balance/PetTypeIdentity.h"

namespace
{
	/**
	 * Os nomes de tipo do jogo NÃO moram mais aqui: quem os conhece é
	 * FPetTypeIdentity, e ele é quem traduz tanto a forma nova
	 * (`Natural/Fogo`) quanto os nomes antigos já assinados.
	 *
	 * Cachorro sobra porque não é tipo de jogo — é o pet do espelho de
	 * fixture, e ele existe para provar que a aparência chega à tela.
	 */
	const FString TipoCachorro = TEXT("Dog");
}

FPetAppearance FPetAppearance::ForType(const FString& PetType)
{
	FPetAppearance Aparencia;

	// Cachorro é do espelho de fixture e não entra no sistema de dois eixos:
	// ele não é um elemento nem uma escola, é o pet que existe para provar que
	// a tabela de aparência chega à tela (L-045).
	if (PetType.Equals(TipoCachorro, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.62f, 0.42f, 0.22f);
		Aparencia.CrestShape = EPetCrestShape::OrelhaCaida;
		Aparencia.CrestScale = FVector(0.13f, 0.13f, 0.34f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -72.0f);
		return Aparencia;
	}

	const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(PetType);
	if (!Identidade.IsValid())
	{
		// Tipo irreconhecível fica NEUTRO, e neutro precisa ser visível: o
		// ouro-palha não aparece em terreno nenhum, então um pet mal
		// cadastrado se destaca em vez de sumir no chão.
		Aparencia.AccentColor = FLinearColor(0.79f, 0.64f, 0.15f);
		Aparencia.CrestShape = EPetCrestShape::Orelha;
		return Aparencia;
	}

	// --- A SILHUETA é a ESCOLA -----------------------------------------
	//
	// A forma diz o que aquele bicho FAZ com a magia dele — dano, terreno ou
	// atributo — e é a informação que decide o turno. Ela vai para o contorno
	// porque o olho separa três contornos num bicho em movimento, e não separa
	// doze matizes.
	if (Identidade.School == TEXT("Fisica"))
	{
		Aparencia.CrestShape = EPetCrestShape::Cristal;
		Aparencia.CrestScale = FVector(0.20f, 0.20f, 0.22f);
	}
	else if (Identidade.School == TEXT("Psiquica"))
	{
		Aparencia.CrestShape = EPetCrestShape::Orbe;
		Aparencia.CrestScale = FVector(0.15f, 0.15f, 0.15f);
	}
	else
	{
		Aparencia.CrestShape = EPetCrestShape::Chama;
		Aparencia.CrestScale = FVector(0.18f, 0.18f, 0.46f);
	}

	// --- A COR é o ELEMENTO --------------------------------------------
	//
	// Fogo é laranja porque fogo é laranja. Este eixo não se aprende, se
	// reconhece — e é por isso que ele fica com a cor, que é o canal que
	// funciona antes de qualquer explicação.
	// O TOMBO também é o elemento, e isto não é enfeite.
	//
	// O teste que este arquivo tinha desde o começo dizia por quê: quem não
	// distingue as formas ainda distingue a cor, e VICE-VERSA — uma só das
	// duas deixa metade dos jogadores sem a informação. Com escola na malha e
	// elemento só na cor, o daltônico perderia o elemento inteiro.
	//
	// Quatro inclinações da mesma malha se separam em silhueta pura, e o
	// jogador lê o elemento sem depender de matiz nenhuma.
	if (Identidade.Element == TEXT("Fogo"))
	{
		Aparencia.AccentColor = FLinearColor(0.95f, 0.35f, 0.05f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -6.0f);   // ereto, sobe
	}
	else if (Identidade.Element == TEXT("Agua"))
	{
		Aparencia.AccentColor = FLinearColor(0.10f, 0.70f, 0.90f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -52.0f);  // deitado, escorre
	}
	else if (Identidade.Element == TEXT("Planta"))
	{
		Aparencia.AccentColor = FLinearColor(0.20f, 0.78f, 0.25f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -28.0f);  // aberto, cresce
	}
	else // Terra
	{
		// Púrpura-pedra, e não marrom: marrom é a cor do CHÃO, e um pet da
		// cor do chão é tão invisível quanto um sem malha.
		Aparencia.AccentColor = FLinearColor(0.55f, 0.30f, 0.72f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -80.0f);  // quase rente, pesa
	}

	return Aparencia;
}
