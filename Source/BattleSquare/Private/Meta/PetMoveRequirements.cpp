// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetMoveRequirements.h"

#include "Data/BattleDataTranslator.h"
#include "Meta/PetAttributeProgression.h"

namespace
{
	// Os nomes que o dado assinado usa. Vivem numa constante e não soltos no
	// switch porque o teste precisa citá-los, e literal repetido em dois
	// lugares concorda até a primeira edição (code-standart.md §16).
	const FString AtributoMusculatura = TEXT("musculature");
	const FString AtributoPersonalidade = TEXT("personality");
	const FString AtributoCamuflagem = TEXT("camouflage");
	const FString AtributoVoo = TEXT("flight");
	const FString AtributoSubsolo = TEXT("underground");

	/** Valor atual do atributo pedido, ou ausente quando o nome é desconhecido. */
	bool TryGetAttributeValue(const FString& Nome, const FOwnedPetInstance& Instance, int32& OutValor)
	{
		if (Nome == AtributoMusculatura) { OutValor = Instance.Musculature; return true; }
		if (Nome == AtributoPersonalidade) { OutValor = Instance.Personality; return true; }
		if (Nome == AtributoCamuflagem)
		{
			OutValor = Instance.SkillProficiency[FPetAttributeProgression::Camouflage];
			return true;
		}
		if (Nome == AtributoVoo)
		{
			OutValor = Instance.SkillProficiency[FPetAttributeProgression::Flight];
			return true;
		}
		if (Nome == AtributoSubsolo)
		{
			OutValor = Instance.SkillProficiency[FPetAttributeProgression::Underground];
			return true;
		}
		return false;
	}
}

bool FPetMoveRequirements::IsMet(const FString& RequiredAttribute, int32 RequiredValue,
	const FOwnedPetInstance& Instance)
{
	// Valor zero ou negativo é golpe SEM requisito, qualquer que seja o nome:
	// exigir "musculatura ao menos 0" é não exigir nada, e tratar isso como
	// requisito faria um golpe depender de um atributo que ele não usa.
	if (RequiredValue <= 0)
	{
		return true;
	}

	int32 ValorAtual = 0;
	if (!TryGetAttributeValue(RequiredAttribute, Instance, ValorAtual))
	{
		return true;
	}

	return ValorAtual >= RequiredValue;
}

void FPetMoveRequirements::ApplyToPresentation(FPetPresentationInfo& Presentation,
	const FOwnedPetInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	for (int32 Indice = 0; Indice < Presentation.MoveUnlocked.Num(); ++Indice)
	{
		const FString Atributo = Presentation.MoveRequiresAttribute.IsValidIndex(Indice)
			? Presentation.MoveRequiresAttribute[Indice]
			: FString();
		const int32 Valor = Presentation.MoveRequiresValue.IsValidIndex(Indice)
			? Presentation.MoveRequiresValue[Indice]
			: 0;

		Presentation.MoveUnlocked[Indice] = IsMet(Atributo, Valor, *Instance);
	}
}
