// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"

/**
 * Quais SKILLS cada tipo de pet tem.
 *
 * Camuflar, voar e submergir entraram como ações universais por engano: os seis
 * tipos originais sempre foram universais, e três ações novas seguiram o padrão
 * sem que nada acusasse a diferença. Skill universal anula a identidade do pet
 * — se todo pet voa, voar não é característica de ninguém.
 *
 * DP-skill-01: vem do TIPO, num arquivo local, e não do registro de pet. O
 * registro chega de um espelho SQLite ASSINADO; acrescentar campo ali exigiria
 * mudar backend, esquema e o payload da assinatura, e nada disso é necessário
 * para o jogo já respeitar skills hoje.
 */
class BATTLESQUARE_API FPetSkillCatalog
{
public:
	/** Falha explícita se ausente ou malformado — nunca catálogo vazio calado. */
	static bool LoadFromJson(const FString& FilePath, FPetSkillCatalog& OutCatalog);

	/**
	 * Skills do tipo, ou lista VAZIA se ele não estiver no catálogo.
	 *
	 * DP-skill-04: ausência degrada para "só os seis tipos universais", nunca
	 * para crash e nunca para "todas as skills".
	 */
	TArray<EActionType> GetSkillsForType(const FString& PetType) const;

	/** Os seis da gramática do combate, que todo pet tem (DP-skill-03). */
	static const TArray<EActionType>& GetUniversalActions();

	/** Universais + skills do tipo: o que a tela deve oferecer. */
	TArray<EActionType> GetAvailableActionsForType(const FString& PetType) const;

	/** Converte o nome do arquivo de dados para a ação. Nenhuma = desconhecida. */
	static EActionType SkillFromName(const FString& SkillName);

private:
	TMap<FString, TArray<EActionType>> SkillsByType;
};
