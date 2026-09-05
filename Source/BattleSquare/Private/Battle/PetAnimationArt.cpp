// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetAnimationArt.h"

namespace
{
	/** As armaduras vistas no import, na ordem de quantas malhas cada uma cobre. */
	const TCHAR* const Armaduras[] = { TEXT("CharacterArmature"), TEXT("Rig_Medium") };

	/** Os nomes de acao PARADA, medidos nas familias de rig do import. */
	const TCHAR* const AcoesParadas[] = { TEXT("Idle"), TEXT("Flying_Idle") };
}

TArray<FString> PetAnimationArt::IdleCandidatesFor(const FString& MeshPath)
{
	TArray<FString> Candidatos;
	if (MeshPath.IsEmpty())
	{
		return Candidatos;
	}

	// O caminho vem na forma `/Pasta/SK_Nome.SK_Nome`; a animacao mora na MESMA
	// pasta, com o nome derivado da malha. Corta o sufixo do objeto para nao
	// duplica-lo.
	FString Pacote = MeshPath;
	int32 Ponto = INDEX_NONE;
	if (MeshPath.FindChar(TEXT('.'), Ponto))
	{
		Pacote = MeshPath.Left(Ponto);
	}

	int32 Barra = INDEX_NONE;
	if (!Pacote.FindLastChar(TEXT('/'), Barra))
	{
		return Candidatos;
	}
	const FString Pasta = Pacote.Left(Barra);
	const FString NomeDaMalha = Pacote.RightChop(Barra + 1);

	for (const TCHAR* Armadura : Armaduras)
	{
		for (const TCHAR* Acao : AcoesParadas)
		{
			const FString NomeDaAnim = FString::Printf(
				TEXT("%s_Anim_%s_%s"), *NomeDaMalha, Armadura, Acao);
			Candidatos.Add(FString::Printf(
				TEXT("%s/%s.%s"), *Pasta, *NomeDaAnim, *NomeDaAnim));
		}
	}

	return Candidatos;
}
