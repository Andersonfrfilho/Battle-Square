// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetAnimationArt.h"
#include "Misc/AutomationTest.h"

/**
 * AR7 — a animação parada se acha por CONVENÇÃO DE NOME, sem Animation
 * Blueprint. O contrapeso: tentar VÁRIAS formas, porque as famílias de rig
 * nomeiam a ação diferente — adivinhar uma só deixaria famílias inteiras
 * paradas.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetIdleByConventionTest,
	"BattleSquare.Battle.PetArte.AnimacaoPorConvencao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetIdleByConventionTest::RunTest(const FString&)
{
	using PetAnimationArt::IdleCandidatesFor;

	const TArray<FString> Candidatos =
		IdleCandidatesFor(TEXT("/Game/Quaternius/Monsters/SK_Dragon.SK_Dragon"));

	TestTrue(TEXT("gera candidatos"), Candidatos.Num() > 0);

	// A FORMA MEDIDA no import: <malha>_Anim_<armadura>_<acao>, na MESMA pasta.
	TestTrue(TEXT("cobre a forma da familia voadora (Flying_Idle)"),
		Candidatos.Contains(TEXT("/Game/Quaternius/Monsters/SK_Dragon_Anim_CharacterArmature_Flying_Idle.SK_Dragon_Anim_CharacterArmature_Flying_Idle")));
	TestTrue(TEXT("cobre a forma das familias de chao (Idle)"),
		Candidatos.Contains(TEXT("/Game/Quaternius/Monsters/SK_Dragon_Anim_CharacterArmature_Idle.SK_Dragon_Anim_CharacterArmature_Idle")));

	// CONTRAPESO: a armadura tambem varia — o KayKit usa `Rig_Medium`, e
	// tentar so `CharacterArmature` deixaria os personagens dele parados.
	TestTrue(TEXT("cobre a armadura do KayKit"),
		Candidatos.ContainsByPredicate([](const FString& C)
		{
			return C.Contains(TEXT("Rig_Medium"));
		}));

	// A animacao mora na MESMA pasta da malha — nunca noutra.
	for (const FString& C : Candidatos)
	{
		TestTrue(TEXT("candidato fica na pasta da malha"),
			C.StartsWith(TEXT("/Game/Quaternius/Monsters/")));
	}

	// Malha vazia: nada a procurar, e nao inventa caminho.
	TestEqual(TEXT("sem malha, sem candidato"), IdleCandidatesFor(TEXT("")).Num(), 0);

	// Caminho sem pasta nao vira candidato torto.
	TestEqual(TEXT("caminho sem barra nao gera candidato"),
		IdleCandidatesFor(TEXT("SoUmNome")).Num(), 0);

	return true;
}
