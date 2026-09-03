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

FText FPetMoveRequirements::GetAttributeLabel(const FString& Attribute)
{
	if (Attribute == AtributoMusculatura)
	{
		return NSLOCTEXT("PetAttributes", "Musculatura", "Musculatura");
	}
	if (Attribute == AtributoPersonalidade)
	{
		return NSLOCTEXT("PetAttributes", "Personalidade", "Personalidade");
	}
	if (Attribute == AtributoCamuflagem)
	{
		return NSLOCTEXT("PetAttributes", "Camuflagem", "Camuflagem");
	}
	if (Attribute == AtributoVoo)
	{
		return NSLOCTEXT("PetAttributes", "Voo", "Voo");
	}
	if (Attribute == AtributoSubsolo)
	{
		return NSLOCTEXT("PetAttributes", "Subsolo", "Subsolo");
	}

	// Cru, de propósito — ver o comentário no header.
	return FText::FromString(Attribute);
}

FText FPetMoveRequirements::DescribeRequirement(const FString& Attribute, int32 RequiredValue)
{
	// Mesmo critério de IsMet: valor não-positivo é ausência de requisito, e
	// descrever "exige Musculatura 0" mostraria uma trava que não existe.
	if (RequiredValue <= 0)
	{
		return FText::GetEmpty();
	}

	return FText::Format(
		NSLOCTEXT("PetAttributes", "ExigeAtributo", "exige {Atributo} {Valor}"),
		FFormatNamedArguments{
			{ TEXT("Atributo"), GetAttributeLabel(Attribute) },
			{ TEXT("Valor"), FText::AsNumber(RequiredValue) },
		});
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

const TArray<FString>& FPetMoveRequirements::AllAttributeNames()
{
	static const TArray<FString> Nomes = {
		TEXT("musculature"), TEXT("personality"),
		TEXT("camouflage"), TEXT("flight"), TEXT("underground"),
	};
	return Nomes;
}
