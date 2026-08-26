// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/TypeEffectivenessTable.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// T1: par presente retorna o percentual cadastrado; par ausente e
// "mesmo tipo contra si mesmo" (sem entrada explícita) retornam 100.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTypeEffectivenessTableGetPercentTest,
	"BattleSquare.Balance.TypeEffectivenessTable.GetPercent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTypeEffectivenessTableGetPercentTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Table;
	Table.SetPercent(TEXT("Fogo"), TEXT("Planta"), 150);
	Table.SetPercent(TEXT("Fogo"), TEXT("Agua"), 50);

	TestEqual(TEXT("Par cadastrado super efetivo"), Table.GetPercent(TEXT("Fogo"), TEXT("Planta")), 150);
	TestEqual(TEXT("Par cadastrado resistido"), Table.GetPercent(TEXT("Fogo"), TEXT("Agua")), 50);
	TestEqual(TEXT("Par ausente é neutro"), Table.GetPercent(TEXT("Fogo"), TEXT("Eletrico")), 100);
	TestEqual(TEXT("Mesmo tipo contra si mesmo, sem entrada explícita, é neutro"), Table.GetPercent(TEXT("Fogo"), TEXT("Fogo")), 100);

	return true;
}

namespace
{
	FString WriteFixtureFile(const FString& FileName, const FString& Contents)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TypeEffectivenessFixture"), FileName);
		FFileHelper::SaveStringToFile(Contents, *Path);
		return Path;
	}
}

// T2: arquivo válido carrega todos os pares; ausente e malformado
// retornam false sem crash.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTypeEffectivenessTableLoadFromJsonTest,
	"BattleSquare.Balance.TypeEffectivenessTable.LoadFromJson",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTypeEffectivenessTableLoadFromJsonTest::RunTest(const FString& Parameters)
{
	// Caso 1: arquivo válido.
	const FString ValidJson = TEXT(R"({
		"types": ["Fogo", "Agua", "Planta"],
		"effectiveness": [
			{ "attacker": "Fogo", "defender": "Planta", "percent": 150 },
			{ "attacker": "Fogo", "defender": "Agua", "percent": 50 },
			{ "attacker": "Agua", "defender": "Fogo", "percent": 150 }
		]
	})");
	const FString ValidPath = WriteFixtureFile(TEXT("valid.json"), ValidJson);

	FTypeEffectivenessTable Table;
	TestTrue(TEXT("Arquivo válido carrega com sucesso"), FTypeEffectivenessTable::LoadFromJson(ValidPath, Table));
	TestEqual(TEXT("Par Fogo->Planta carregado corretamente"), Table.GetPercent(TEXT("Fogo"), TEXT("Planta")), 150);
	TestEqual(TEXT("Par Fogo->Agua carregado corretamente"), Table.GetPercent(TEXT("Fogo"), TEXT("Agua")), 50);
	TestEqual(TEXT("Par Agua->Fogo carregado corretamente"), Table.GetPercent(TEXT("Agua"), TEXT("Fogo")), 150);
	TestEqual(TEXT("Par sem entrada continua neutro"), Table.GetPercent(TEXT("Planta"), TEXT("Agua")), 100);

	// Caso 2: arquivo ausente.
	FTypeEffectivenessTable MissingTable;
	const FString MissingPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TypeEffectivenessFixture"), TEXT("nao-existe.json"));
	TestFalse(TEXT("Arquivo ausente retorna false"), FTypeEffectivenessTable::LoadFromJson(MissingPath, MissingTable));

	// Caso 3: JSON malformado (sintaxe inválida).
	const FString MalformedPath = WriteFixtureFile(TEXT("malformed.json"), TEXT("{ isto nao e json valido"));
	FTypeEffectivenessTable MalformedTable;
	TestFalse(TEXT("JSON malformado retorna false, sem crash"), FTypeEffectivenessTable::LoadFromJson(MalformedPath, MalformedTable));

	// Caso 4: tipo novo reconhecido sem mudança de código (ESCALA-06) —
	// prova viva: um tipo que não existia no Caso 1 aparece aqui só
	// porque o JSON mudou.
	const FString NewTypeJson = TEXT(R"({
		"types": ["Fogo", "Eletrico"],
		"effectiveness": [
			{ "attacker": "Eletrico", "defender": "Fogo", "percent": 100 },
			{ "attacker": "Fogo", "defender": "Eletrico", "percent": 150 }
		]
	})");
	const FString NewTypePath = WriteFixtureFile(TEXT("new-type.json"), NewTypeJson);
	FTypeEffectivenessTable NewTypeTable;
	TestTrue(TEXT("Arquivo com tipo novo carrega com sucesso"), FTypeEffectivenessTable::LoadFromJson(NewTypePath, NewTypeTable));
	TestEqual(TEXT("Tipo novo (Eletrico) reconhecido sem rebuild"), NewTypeTable.GetPercent(TEXT("Fogo"), TEXT("Eletrico")), 150);

	return true;
}
