// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ScenaryPalette.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

/**
 * Adocao de arte (PRONTOS.md §5) — trocar de pacote e trocar DADO: sem override
 * de config, o papel cai na primitiva (verde sem pacote, invariante 20); COM
 * override, usa o asset do pacote. E papel sem asset NUNCA fica invisivel.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtAdoptionByConfigTest,
	"BattleSquare.Environment.MalhaDeFora.AdocaoEDado",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArtAdoptionByConfigTest::RunTest(const FString&)
{
	using namespace ScenaryPalette;

	const TCHAR* Secao = TEXT("/Script/BattleSquare.Art");
	const TCHAR* Chave = RoleConfigKey(EScenaryRole::ForestTree);

	// Guarda o que houver para restaurar.
	FString Original;
	const bool bTinha = GConfig->GetString(Secao, Chave, Original, GGameIni);

	// SEM override: cai na primitiva. Verde sem pacote nenhum.
	GConfig->RemoveKey(Secao, Chave, GGameIni);
	const FString SemPacote = MeshPathForRole(EScenaryRole::ForestTree, EScenaryPrimitive::Cylinder);
	TestTrue(TEXT("sem pacote, o papel cai na primitiva"),
		SemPacote.StartsWith(TEXT("/Engine/BasicShapes/")));

	// COM override (adotar um pacote): usa o asset do pacote — troca de DADO.
	GConfig->SetString(Secao, Chave, TEXT("/Game/Quaternius/Trees/Tree_01.Tree_01"), GGameIni);
	const FString ComPacote = MeshPathForRole(EScenaryRole::ForestTree, EScenaryPrimitive::Cylinder);
	TestEqual(TEXT("com override, usa o asset do pacote"),
		ComPacote, FString(TEXT("/Game/Quaternius/Trees/Tree_01.Tree_01")));

	// Override VAZIO nao conta como adocao — cai na primitiva (nunca invisivel).
	GConfig->SetString(Secao, Chave, TEXT(""), GGameIni);
	TestTrue(TEXT("override vazio cai na primitiva, nunca invisivel"),
		MeshPathForRole(EScenaryRole::ForestTree, EScenaryPrimitive::Cylinder)
			.StartsWith(TEXT("/Engine/BasicShapes/")));

	// Restaura.
	if (bTinha) { GConfig->SetString(Secao, Chave, *Original, GGameIni); }
	else { GConfig->RemoveKey(Secao, Chave, GGameIni); }

	return true;
}
