// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetSkillCatalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString EscreverCatalogo(const FString& NomeDoArquivo, const FString& Conteudo)
	{
		const FString Caminho = FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("PetSkillFixture"), NomeDoArquivo);
		FFileHelper::SaveStringToFile(Conteudo, *Caminho);
		return Caminho;
	}
}

// Skill vem do TIPO, e tipos diferentes têm skills diferentes — é isso que faz
// a escolha do pet ter consequência. Se todo pet voasse, voar não seria
// característica de ninguém.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetSkillCatalogGivesDifferentSkillsPerTypeTest,
	"BattleSquare.Balance.PetSkillCatalog.DifferentSkillsPerType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetSkillCatalogGivesDifferentSkillsPerTypeTest::RunTest(const FString& Parameters)
{
	const FString Caminho = EscreverCatalogo(TEXT("valido.json"), TEXT(R"({
		"types": [
			{ "type": "Voador", "skills": ["voar"] },
			{ "type": "Sombra", "skills": ["camuflar", "submergir"] },
			{ "type": "Normal", "skills": [] }
		]
	})"));

	FPetSkillCatalog Catalogo;
	TestTrue(TEXT("Arquivo válido carrega"), FPetSkillCatalog::LoadFromJson(Caminho, Catalogo));

	const TArray<EActionType> Voador = Catalogo.GetSkillsForType(TEXT("Voador"));
	TestEqual(TEXT("Voador tem uma skill"), Voador.Num(), 1);
	TestTrue(TEXT("E ela é voar"), Voador.Contains(EActionType::Voar));
	TestFalse(TEXT("Voador NÃO camufla"), Voador.Contains(EActionType::Camuflar));

	const TArray<EActionType> Sombra = Catalogo.GetSkillsForType(TEXT("Sombra"));
	TestEqual(TEXT("Sombra tem duas"), Sombra.Num(), 2);
	TestFalse(TEXT("Sombra NÃO voa"), Sombra.Contains(EActionType::Voar));

	TestEqual(TEXT("Normal não tem skill nenhuma"),
		Catalogo.GetSkillsForType(TEXT("Normal")).Num(), 0);

	return true;
}

// DP-skill-03/04: os seis originais são de todo mundo, e tipo desconhecido
// degrada para eles — nunca para crash, nunca para "todas as skills".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetSkillCatalogAlwaysKeepsUniversalActionsTest,
	"BattleSquare.Balance.PetSkillCatalog.AlwaysKeepsUniversalActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetSkillCatalogAlwaysKeepsUniversalActionsTest::RunTest(const FString& Parameters)
{
	const FString Caminho = EscreverCatalogo(TEXT("universais.json"), TEXT(R"({
		"types": [ { "type": "Voador", "skills": ["voar"] } ]
	})"));

	FPetSkillCatalog Catalogo;
	FPetSkillCatalog::LoadFromJson(Caminho, Catalogo);

	const TArray<EActionType> Desconhecido = Catalogo.GetAvailableActionsForType(TEXT("TipoQueNaoExiste"));
	TestEqual(TEXT("Tipo desconhecido fica com os seis universais"),
		Desconhecido.Num(), FPetSkillCatalog::GetUniversalActions().Num());
	TestTrue(TEXT("E atacar está entre eles"), Desconhecido.Contains(EActionType::Atacar));
	TestFalse(TEXT("Mas não ganha skill de graça"), Desconhecido.Contains(EActionType::Voar));

	const TArray<EActionType> Voador = Catalogo.GetAvailableActionsForType(TEXT("Voador"));
	TestTrue(TEXT("Voador mantém os universais"), Voador.Contains(EActionType::Esquivar));
	TestTrue(TEXT("E acrescenta a skill dele"), Voador.Contains(EActionType::Voar));

	return true;
}

// Arquivo ausente ou malformado falha ALTO. Catálogo vazio silencioso deixaria
// todo pet sem skill sem ninguém entender por quê — e o mesmo padrão já vale
// para arenas e efetividade de tipo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetSkillCatalogFailsLoudlyTest,
	"BattleSquare.Balance.PetSkillCatalog.FailsLoudly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetSkillCatalogFailsLoudlyTest::RunTest(const FString& Parameters)
{
	FPetSkillCatalog Catalogo;

	TestFalse(TEXT("Arquivo ausente falha"),
		FPetSkillCatalog::LoadFromJson(TEXT("/caminho/que/nao/existe.json"), Catalogo));

	const FString Malformado = EscreverCatalogo(TEXT("malformado.json"), TEXT("{ isto nao e json"));
	TestFalse(TEXT("JSON malformado falha"),
		FPetSkillCatalog::LoadFromJson(Malformado, Catalogo));

	const FString SemTypes = EscreverCatalogo(TEXT("sem-types.json"), TEXT(R"({ "outra": [] })"));
	TestFalse(TEXT("JSON sem 'types' falha"),
		FPetSkillCatalog::LoadFromJson(SemTypes, Catalogo));

	// Nome de skill desconhecido é IGNORADO, não vira ação qualquer: um erro de
	// digitação daria ao pet uma habilidade que ninguém escreveu.
	const FString SkillErrada = EscreverCatalogo(TEXT("skill-errada.json"), TEXT(R"({
		"types": [ { "type": "Estranho", "skills": ["teletransporte"] } ]
	})"));
	TestTrue(TEXT("Arquivo em si é válido"),
		FPetSkillCatalog::LoadFromJson(SkillErrada, Catalogo));
	TestEqual(TEXT("Skill desconhecida não entra"),
		Catalogo.GetSkillsForType(TEXT("Estranho")).Num(), 0);

	return true;
}
