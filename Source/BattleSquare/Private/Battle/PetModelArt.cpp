// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetModelArt.h"

#include "Misc/ConfigCacheIni.h"

namespace
{
	/** Teto de sanidade no id que vem do espelho, antes de virar chave. */
	constexpr int32 TamanhoMaximoDoNome = 80;
}

FString PetModelArt::ConfigKeyFor(const FString& PetName)
{
	if (PetName.IsEmpty() || PetName.Len() > TamanhoMaximoDoNome)
	{
		return FString();
	}

	// Peneira o que quebraria a chave ou abriria caminho: separador, ponto e
	// os delimitadores de secao do .ini. Acento PASSA — os pets se chamam
	// `Faisca`, `Vigilia`, `Vagalhao`, e recusa-los deixaria metade do catalogo
	// sem poder vestir modelo.
	for (const TCHAR Caractere : PetName)
	{
		const bool bProibido = Caractere == TEXT('/') || Caractere == TEXT('\\')
			|| Caractere == TEXT('.') || Caractere == TEXT('[') || Caractere == TEXT(']')
			|| Caractere == TEXT('=') || Caractere == TEXT(';');
		if (bProibido)
		{
			return FString();
		}
	}

	return FString::Printf(TEXT("Mesh_Pet_%s"), *PetName);
}

FString PetModelArt::MeshPathFor(const FString& PetName)
{
	const FString Chave = ConfigKeyFor(PetName);
	if (Chave.IsEmpty())
	{
		return FString();
	}

	FString Caminho;
	GConfig->GetString(TEXT("/Script/BattleSquare.Art"), *Chave, Caminho, GGameIni);
	return Caminho;
}
