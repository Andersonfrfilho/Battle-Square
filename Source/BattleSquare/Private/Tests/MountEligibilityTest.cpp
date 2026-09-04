// Copyright 2026 Anderson. All Rights Reserved.

#include "World/MountEligibility.h"
#include "Data/PetDataLoader.h"
#include "Misc/AutomationTest.h"

/**
 * MT4 — pet montavel monta, nao-montavel e recusado; e dado ANTIGO (sem o
 * campo) carrega como nao-montavel, nunca por acidente montavel.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMountEligibilityTest,
	"BattleSquare.World.Montaria.NemTodoPetMonta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountEligibilityTest::RunTest(const FString&)
{
	using namespace MountEligibility;

	// Marcado montavel: monta. Marcado nao: recusado.
	TestTrue(TEXT("pet montavel monta"), CanMount(true));
	TestFalse(TEXT("pet nao-montavel e recusado"), CanMount(false));

	// A recusa tem motivo para a tela.
	TestFalse(TEXT("o motivo da recusa nao e vazio"), RefusalReason().IsEmpty());

	// O CONTRAPESO (retrocompatibilidade): um registro recem-construido — o que
	// um save ANTIGO sem o campo vira — NAO e montavel por default. Nunca crasha,
	// nunca vira montavel por acidente.
	const FLoadedPetRecord Antigo;
	TestFalse(TEXT("dado antigo carrega como nao-montavel por default"),
		CanMount(Antigo.bMountable));

	// E um registro que DIZ montavel monta — o valor lido vale.
	FLoadedPetRecord Novo;
	Novo.bMountable = true;
	TestTrue(TEXT("dado que diz montavel monta"), CanMount(Novo.bMountable));

	return true;
}
