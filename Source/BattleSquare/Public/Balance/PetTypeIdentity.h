// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O tipo de um pet tem DOIS eixos, não um nome.
 *
 * `"Psiquica/Fogo"` é uma escola e um elemento. Sete nomes soltos exigiam uma
 * tabela de quarenta e duas relações que ninguém decora; dois eixos pequenos
 * se compõem — e a mesma multiplicação cobre doze tipos com duas tabelas de
 * quatro linhas.
 *
 * A ESCOLA diz o que a magia daquele pet FAZ (dano, terreno, atributo); o
 * ELEMENTO diz contra o que ela é forte. Um eixo é verbo, o outro é matéria.
 */
struct BATTLESQUARE_API FPetTypeIdentity
{
	/** "Fisica" | "Natural" | "Psiquica" — vazio quando não se reconhece. */
	FString School;

	/** "Fogo" | "Agua" | "Planta" | "Terra" — vazio quando não se reconhece. */
	FString Element;

	bool IsValid() const { return !School.IsEmpty() && !Element.IsEmpty(); }

	/** De volta para a forma canônica `Escola/Elemento`. */
	FString ToTypeString() const;

	/**
	 * Lê o tipo como ele vem do dado assinado.
	 *
	 * Aceita a forma nova (`"Natural/Fogo"`) e os nomes ANTIGOS de um eixo só
	 * (`"Fogo"`, `"Inseto"`, `"Magico"`…). Os antigos continuam valendo porque
	 * já foram ASSINADOS: recusá-los invalidaria pets que existem, e o dado
	 * assinado é justamente o que não se pode reescrever de fora.
	 */
	static FPetTypeIdentity Parse(const FString& PetType);

	static const TArray<FString>& AllSchools();
	static const TArray<FString>& AllElements();
};
