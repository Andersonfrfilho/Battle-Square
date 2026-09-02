// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Um TRAÇO DE BIOLOGIA: um valor de um eixo, e o que ele faz.
 *
 * A mesma forma serve aos quatro eixos — pele, porte, respiração e apoio — de
 * propósito. Quatro structs quase iguais dariam quatro leitores de JSON quase
 * iguais, e a quarta cópia é sempre a que esquece de ler um campo.
 */
struct BATTLESQUARE_API FPetBiologyTrait
{
	FString Name;

	/**
	 * QUANTO este traço soma à resistência de cada fluido, por nome de fluido.
	 *
	 * SOMA, e aceita negativo: negativo é FRAQUEZA, e é ela que faz a escolha
	 * de biologia ter custo. Sem fraqueza, toda biologia seria melhor que
	 * nenhuma, e o eixo viraria uma lista de bônus.
	 *
	 * A chave é o nome do fluido NO REGISTRO. Uma segunda grafia seria uma
	 * resistência que nunca casa e nunca acusa.
	 */
	TMap<FString, int32> FluidResists;

	/** Quanto este traço soma à firmeza contra a corrente, em partes por mil. */
	int32 FootingPerMille = 0;

	/**
	 * Submerge sem que a água funda cobre nada.
	 *
	 * Só a RESPIRAÇÃO usa; nos outros eixos fica falso e não decide nada. Vive
	 * no traço genérico em vez de num struct próprio pela mesma razão que os
	 * quatro eixos compartilham a forma: uma exceção não paga um segundo tipo.
	 */
	bool bBreathesUnderwater = false;
};

/**
 * A BIOLOGIA de um pet: um valor por eixo.
 *
 * Eixo vazio é eixo NÃO INFORMADO, e soma zero. Pet assinado antes desta
 * feature não pode mudar de resistência sem uma linha do cadastro dele ter
 * mudado — o dono só descobriria perdendo.
 */
struct BATTLESQUARE_API FPetBiology
{
	FString Skin;
	FString Build;
	FString Breathing;
	FString Limbs;

	bool IsEmpty() const
	{
		return Skin.IsEmpty() && Build.IsEmpty()
			&& Breathing.IsEmpty() && Limbs.IsEmpty();
	}
};

/**
 * O catálogo dos eixos de biologia, lido de `Config/PetBiology.json`.
 *
 * ## Por que EIXOS, e não um perfil nomeado
 *
 * Com perfil ("Réptil de deserto"), cada criatura nova pede um perfil novo, e
 * "Réptil de pântano" vira uma segunda entrada que repete tudo menos uma linha.
 * Com eixos, as combinações nascem do CRUZAMENTO — e é o cruzamento que faz
 * dois pets do mesmo elemento resistirem diferente, que é o aceite da tarefa.
 * Um perfil só chega às combinações que alguém cadastrou uma a uma.
 */
class BATTLESQUARE_API FPetBiologyCatalog
{
public:
	static const FPetBiologyCatalog& Get();

	/** Só para teste: instala um catálogo montado à mão. Nullptr desfaz. */
	static void OverrideForTesting(const FPetBiologyCatalog* Catalog);

	const FPetBiologyTrait* FindSkin(const FString& Name) const;
	const FPetBiologyTrait* FindBuild(const FString& Name) const;
	const FPetBiologyTrait* FindBreathing(const FString& Name) const;
	const FPetBiologyTrait* FindLimbs(const FString& Name) const;

	/**
	 * A soma dos quatro eixos para um fluido.
	 *
	 * UM ponto de soma, e não quatro chamadas espalhadas por quem precisa:
	 * espalhada, a terceira cópia esquece um eixo, e o pet resiste diferente
	 * conforme quem perguntou.
	 */
	int32 ResistFor(const FPetBiology& Biology, const FString& FluidName) const;

	/** A soma dos quatro eixos na firmeza contra a corrente. */
	int32 FootingFor(const FPetBiology& Biology) const;

	/** ALGUM eixo dá respiração debaixo d'água. */
	bool BreathesUnderwater(const FPetBiology& Biology) const;

	TArray<FPetBiologyTrait> Skins;
	TArray<FPetBiologyTrait> Builds;
	TArray<FPetBiologyTrait> Breathings;
	TArray<FPetBiologyTrait> Limbs;
};
