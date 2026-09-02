// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/PetAppearance.h"

/**
 * Um ELEMENTO: o que ele é, como se parece, e o que o corpo dele sabe fazer.
 *
 * Cor E inclinação, porque são os dois canais de silhueta — quem não distingue
 * matiz lê o tombo, e vice-versa. Declarar só um deixaria metade dos jogadores
 * sem a informação.
 */
struct BATTLESQUARE_API FPetElementDefinition
{
	FString Name;
	FLinearColor Color = FLinearColor::White;
	float TiltDegrees = 0.0f;

	/** Nomes de skill, como PetSkillCatalog os traduz ("voar", "camuflar"…). */
	TArray<FString> SkillNames;

	/**
	 * Os golpes deste elemento CONDUZEM pela água molhada.
	 *
	 * Declarado aqui, e não como um `if` no tradutor comparando com "Raio":
	 * um segundo elemento condutor passa a ser um campo no JSON, e não uma
	 * edição de código — que é a promessa deste arquivo desde o começo.
	 *
	 * Ausente é FALSO. Elemento que não diz nada não conduz, e é o inócuo.
	 */
	bool bConducts = false;

	/**
	 * QUANTO este elemento resiste a cada fluido, por nome de fluido.
	 *
	 * É o "traço" da criatura, e ele vem do ELEMENTO pela mesma razão que o
	 * Incorpóreo vem: o núcleo guarda o efeito (um número), a camada de fora
	 * guarda o significado (o elemento se chama Fogo, e por isso aguenta lava).
	 *
	 * Mapa, e não bitmask de traço: resistência é PORCENTAGEM POR FLUIDO, e um
	 * bit por fluido daria oito traços que só sabem dizer sim ou não.
	 */
	TMap<FString, int32> FluidResists;
};

/** Uma ESCOLA: o que a magia dela faz, e a malha que diz isso na tela. */
struct BATTLESQUARE_API FPetSchoolDefinition
{
	FString Name;
	EPetCrestShape Crest = EPetCrestShape::Orelha;
	FVector CrestScale = FVector(0.18f, 0.18f, 0.46f);
};

/** Nome de um eixo só que já foi ASSINADO, e o par que ele significa. */
struct BATTLESQUARE_API FPetLegacyTypeName
{
	FString Name;
	FString School;
	FString Element;
};

/**
 * TODO o sistema de tipos, num arquivo só.
 *
 * Acrescentar um elemento era editar dois arquivos C++ e dois JSON, e esquecer
 * um deles produzia um tipo pela metade — que existe, luta, e sai parecendo
 * genérico. Agora é UMA entrada em `Config/PetTypes.json`.
 *
 * O que continua sendo código, e por quê: a MALHA da crista (asset novo é
 * código), o atributo que uma magia mexe (`EBattleStat`, porque cada um tem
 * uma regra em `GetEffectiveStat`) e a propriedade de casa (`ECellProperty`,
 * mesma razão). Isso não é dado — é comportamento com nome.
 *
 * Arquivo ausente NÃO tem valor padrão embutido no C++: duas cópias da mesma
 * tabela concordam até a primeira edição, e este projeto já pagou por isso
 * três vezes (L-032, L-033). Sem o arquivo, os tipos não resolvem, os pets
 * saem neutros, e o painel diz que a carga falhou — visível, e não silencioso.
 */
class BATTLESQUARE_API FPetTypeCatalog
{
public:
	static bool LoadFromJson(const FString& FilePath, FPetTypeCatalog& OutCatalog);

	/**
	 * O catálogo em uso, carregado uma vez de `Config/PetTypes.json`.
	 *
	 * Cache de processo porque `ForType` é chamado ao nascer cada pet: reler o
	 * arquivo ali seria I/O por spawn.
	 */
	static const FPetTypeCatalog& Get();

	/** Troca o catálogo em uso. `nullptr` volta ao do arquivo. */
	static void OverrideForTesting(const FPetTypeCatalog* Catalog);

	const FPetElementDefinition* FindElement(const FString& Name) const;
	const FPetSchoolDefinition* FindSchool(const FString& Name) const;

	/** False quando o nome não é um legado conhecido. */
	bool ResolveLegacyName(const FString& Name, FString& OutSchool, FString& OutElement) const;

	const TArray<FPetElementDefinition>& GetElements() const { return Elements; }
	const TArray<FPetSchoolDefinition>& GetSchools() const { return Schools; }

	bool IsEmpty() const { return Elements.IsEmpty() && Schools.IsEmpty(); }

private:
	TArray<FPetElementDefinition> Elements;
	TArray<FPetSchoolDefinition> Schools;
	TArray<FPetLegacyTypeName> LegacyNames;
};
