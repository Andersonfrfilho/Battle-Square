// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/ArenaLayoutCatalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString WriteArenaFixtureFile(const FString& FileName, const FString& Contents)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ArenaLayoutFixture"), FileName);
		FFileHelper::SaveStringToFile(Contents, *Path);
		return Path;
	}
}

// T6: arquivo válido carrega layouts nomeados; ausente/malformado
// retornam false; nome inexistente em GetLayoutByName não crasha.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaLayoutCatalogLoadAndLookupTest,
	"BattleSquare.Balance.ArenaLayoutCatalog.LoadAndLookup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FArenaLayoutCatalogLoadAndLookupTest::RunTest(const FString& Parameters)
{
	const FString ValidJson = TEXT(R"({
		"arenas": [
			{ "name": "Arena de Lava", "layout": [0,0,0,0,2,0,0,0,1] },
			{ "name": "Arena Neutra", "layout": [0,0,0,0,0,0,0,0,0] }
		]
	})");
	const FString ValidPath = WriteArenaFixtureFile(TEXT("valid.json"), ValidJson);

	FArenaLayoutCatalog Catalog;
	TestTrue(TEXT("Arquivo válido carrega com sucesso"), FArenaLayoutCatalog::LoadFromJson(ValidPath, Catalog));

	TArray<uint8> LavaLayout;
	TestTrue(TEXT("Arena de Lava encontrada pelo nome"), Catalog.GetLayoutByName(TEXT("Arena de Lava"), LavaLayout));
	TestEqual(TEXT("Layout tem 9 posições"), LavaLayout.Num(), BattleGridDefaultCellCount);
	TestEqual(TEXT("Casa central é Damage (valor 2)"), LavaLayout[4], static_cast<uint8>(2));
	TestEqual(TEXT("Última casa é Blocked (valor 1)"), LavaLayout[8], static_cast<uint8>(1));

	TArray<uint8> NotFoundLayout;
	TestFalse(TEXT("Nome inexistente retorna false, sem crash"), Catalog.GetLayoutByName(TEXT("Arena Inexistente"), NotFoundLayout));

	// Arquivo ausente.
	FArenaLayoutCatalog MissingCatalog;
	const FString MissingPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ArenaLayoutFixture"), TEXT("nao-existe.json"));
	TestFalse(TEXT("Arquivo ausente retorna false"), FArenaLayoutCatalog::LoadFromJson(MissingPath, MissingCatalog));

	// JSON malformado.
	const FString MalformedPath = WriteArenaFixtureFile(TEXT("malformed.json"), TEXT("{ nao e json valido"));
	FArenaLayoutCatalog MalformedCatalog;
	TestFalse(TEXT("JSON malformado retorna false, sem crash"), FArenaLayoutCatalog::LoadFromJson(MalformedPath, MalformedCatalog));

	// Layout com tamanho errado (diferente de 9) é rejeitado.
	const FString WrongSizeJson = TEXT(R"({ "arenas": [ { "name": "Quebrada", "layout": [0,0,0] } ] })");
	const FString WrongSizePath = WriteArenaFixtureFile(TEXT("wrong-size.json"), WrongSizeJson);
	FArenaLayoutCatalog WrongSizeCatalog;
	TestFalse(TEXT("Layout com tamanho diferente de 9 é rejeitado"), FArenaLayoutCatalog::LoadFromJson(WrongSizePath, WrongSizeCatalog));

	return true;
}

// Arena com dimensões DECLARADAS. Sem isto, todo layout teria de ser 3x3
// e um campo 4x6 abriria sempre neutro — a feature existiria no núcleo e
// não teria como chegar ao arquivo que descreve os campos.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaLayoutCatalogAcceptsDeclaredSizeTest,
	"BattleSquare.Balance.ArenaLayoutCatalog.AcceptsDeclaredSize",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FArenaLayoutCatalogAcceptsDeclaredSizeTest::RunTest(const FString& Parameters)
{
	const FString RetangularJson = TEXT(R"({
		"arenas": [
			{ "name": "Corredor", "columns": 4, "rows": 6,
			  "layout": [0,0,0,0, 0,0,0,0, 0,4,4,0, 0,4,4,0, 0,0,0,0, 0,0,0,0] }
		]
	})");

	FArenaLayoutCatalog Catalogo;
	TestTrue(TEXT("Arena 4x6 declarada carrega"),
		FArenaLayoutCatalog::LoadFromJson(
			WriteArenaFixtureFile(TEXT("retangular.json"), RetangularJson), Catalogo));

	TArray<uint8> Layout;
	TestTrue(TEXT("Corredor encontrado"), Catalogo.GetLayoutByName(TEXT("Corredor"), Layout));
	TestEqual(TEXT("Layout tem 24 casas"), Layout.Num(), 24);
	TestEqual(TEXT("Água na casa (1,2), índice 9"), Layout[9], static_cast<uint8>(4));

	// Comprimento que não bate com as dimensões declaradas é recusa, não
	// preenchimento: um layout curto deslocaria toda linha depois da
	// primeira, e a arena abriria com a água no lugar errado.
	const FString TruncadoJson = TEXT(R"({
		"arenas": [
			{ "name": "Curto", "columns": 4, "rows": 6, "layout": [0,0,0,0, 0,0,0,0] }
		]
	})");

	FArenaLayoutCatalog Recusado;
	TestFalse(TEXT("Layout curto para as dimensões declaradas é recusado"),
		FArenaLayoutCatalog::LoadFromJson(
			WriteArenaFixtureFile(TEXT("truncado.json"), TruncadoJson), Recusado));

	return true;
}
