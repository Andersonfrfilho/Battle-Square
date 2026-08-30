// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetTypeIdentity.h"

#include "Balance/PetTypeCatalog.h"

namespace
{
	/** Casa sem diferenciar caixa e devolve a grafia CANÔNICA do catálogo. */
	bool Reconhecer(const TArray<FString>& Conhecidos, const FString& Bruto, FString& Fora)
	{
		for (const FString& Conhecido : Conhecidos)
		{
			if (Bruto.Equals(Conhecido, ESearchCase::IgnoreCase))
			{
				Fora = Conhecido;
				return true;
			}
		}
		return false;
	}
}

const TArray<FString>& FPetTypeIdentity::AllSchools()
{
	// Reconstruído a cada chamada em vez de guardado num static: o catálogo
	// pode ser trocado (teste, recarga), e um cache aqui devolveria a lista
	// antiga sem nada indicar que ela ficou velha.
	static TArray<FString> Nomes;
	Nomes.Reset();
	for (const FPetSchoolDefinition& Escola : FPetTypeCatalog::Get().GetSchools())
	{
		Nomes.Add(Escola.Name);
	}
	return Nomes;
}

const TArray<FString>& FPetTypeIdentity::AllElements()
{
	static TArray<FString> Nomes;
	Nomes.Reset();
	for (const FPetElementDefinition& Elemento : FPetTypeCatalog::Get().GetElements())
	{
		Nomes.Add(Elemento.Name);
	}
	return Nomes;
}

FString FPetTypeIdentity::ToTypeString() const
{
	return IsValid() ? FString::Printf(TEXT("%s/%s"), *School, *Element) : FString();
}

FPetTypeIdentity FPetTypeIdentity::Parse(const FString& PetType)
{
	FPetTypeIdentity Identidade;

	FString Esquerda;
	FString Direita;
	if (PetType.Split(TEXT("/"), &Esquerda, &Direita))
	{
		// Reconhecer os dois lados SEPARADAMENTE, e não aceitar o par inteiro
		// por confiança: "Natural/Telepatia" precisa falhar no elemento em vez
		// de virar um tipo que existe e não bate com nada.
		Reconhecer(AllSchools(), Esquerda.TrimStartAndEnd(), Identidade.School);
		Reconhecer(AllElements(), Direita.TrimStartAndEnd(), Identidade.Element);
		return Identidade;
	}

	FPetTypeCatalog::Get().ResolveLegacyName(
		PetType.TrimStartAndEnd(), Identidade.School, Identidade.Element);
	return Identidade;
}
