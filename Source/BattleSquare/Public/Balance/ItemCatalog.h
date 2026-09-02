// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A NATUREZA do item — o que acontece quando ele é usado.
 *
 * Vem do CADASTRO, e não do nome: adivinhar por nome ("tudo que termina em
 * poção é consumível") transformaria uma escolha de conteúdo numa convenção de
 * grafia, e a primeira exceção quebraria em silêncio.
 */
enum class EItemNature : uint8
{
	/** Age enquanto vestido. Usar não gasta. */
	Equipamento,

	/** Age uma vez; a quantidade cai. Zerou, sai da mochila. */
	Consumivel
};

/**
 * Um ITEM do catálogo.
 *
 * Espelha `FPetBiologyTrait` de propósito: os dois somam na MESMA conta
 * (`ComposeFluidResist`), e dar formas diferentes a coisas que se somam juntas
 * produziria dois leitores de JSON e duas maneiras de cadastrar — a segunda
 * sendo sempre a que esquece o override de teste.
 */
struct BATTLESQUARE_API FItemDefinition
{
	/** Estável, sem acento: vai para o SAVE, e renomear quebra mochila salva. */
	FString Id;

	/** O que o jogador lê. Renomear é barato; renomear o `Id` não é. */
	FString Name;

	EItemNature Nature = EItemNature::Equipamento;

	/**
	 * QUANTO este item soma à resistência de cada fluido, por nome de fluido.
	 *
	 * SÓ SOMA. Negativo é ignorado na composição, porque item que ENFRAQUECE é
	 * maldição — outra mecânica, com outra narração e outra forma de sair dela.
	 * Um item mal cadastrado não pode virar uma por acidente.
	 */
	TMap<FString, int32> FluidResists;

	/** Quanto soma à firmeza contra a corrente, em partes por mil. */
	int32 FootingPerMille = 0;
};

/** O catálogo de itens, lido de `Config/Items.json`. */
class BATTLESQUARE_API FItemCatalog
{
public:
	static const FItemCatalog& Get();

	/** Só para teste: instala um catálogo montado à mão. Nullptr desfaz. */
	static void OverrideForTesting(const FItemCatalog* Catalog);

	/** Pelo `Id`, que é o que o save guarda. Desconhecido devolve nulo. */
	const FItemDefinition* Find(const FString& Id) const;

	TArray<FItemDefinition> Items;
};
