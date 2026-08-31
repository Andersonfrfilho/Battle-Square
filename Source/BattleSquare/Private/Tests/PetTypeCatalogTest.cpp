// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetSkillCatalog.h"
#include "Balance/PetTypeCatalog.h"
#include "HAL/PlatformFileManager.h"
#include "Balance/PetTypeIdentity.h"
#include "Battle/PetAppearance.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString EscreverCatalogoDeTeste(const FString& NomeDoArquivo, const FString& Conteudo)
	{
		const FString Caminho = FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("PetTypeCatalogFixture"), NomeDoArquivo);
		FFileHelper::SaveStringToFile(Conteudo, *Caminho);
		return Caminho;
	}

	/** Devolve o catálogo ao do arquivo mesmo se o teste falhar no meio. */
	struct FCatalogoTemporario
	{
		explicit FCatalogoTemporario(const FPetTypeCatalog& Catalogo)
		{
			FPetTypeCatalog::OverrideForTesting(&Catalogo);
		}
		~FCatalogoTemporario() { FPetTypeCatalog::OverrideForTesting(nullptr); }
	};
}

// UM ELEMENTO NOVO É UMA LINHA, e nada de C++.
//
// É a garantia que esta feature inteira existe para dar, e ela se perde em
// silêncio: alguém acrescenta um `if` numa tabela de cor "só desta vez", e o
// próximo elemento volta a exigir quatro edições — das quais esquecer uma
// produz um tipo pela metade, que existe, luta e sai parecendo genérico.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNewElementNeedsNoCodeTest,
	"BattleSquare.Balance.PetTypeCatalog.NewElementNeedsNoCode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNewElementNeedsNoCodeTest::RunTest(const FString& Parameters)
{
	// Um elemento que NÃO existe em lugar nenhum do código.
	const FString Json = TEXT(R"({
		"schools": [
			{ "name": "Natural", "crest": "Chama", "crestScale": [0.18, 0.18, 0.46] }
		],
		"elements": [
			{ "name": "Vento", "color": [0.75, 0.90, 0.95], "tiltDegrees": -40, "skills": ["voar"] }
		]
	})");

	FPetTypeCatalog Catalogo;
	TestTrue(TEXT("O catálogo carrega"),
		FPetTypeCatalog::LoadFromJson(EscreverCatalogoDeTeste(TEXT("vento.json"), Json), Catalogo));

	const FCatalogoTemporario EmUso(Catalogo);

	// O tipo RESOLVE, sem nenhuma linha de C++ conhecer "Vento".
	const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(TEXT("Natural/Vento"));
	TestTrue(TEXT("O par é válido"), Identidade.IsValid());
	TestEqual(TEXT("O elemento novo é lido"), Identidade.Element, FString(TEXT("Vento")));

	// E chega à TELA com a cor e o tombo declarados — sem isso ele existiria
	// na regra e não no jogo, que é o defeito de L-041.
	const FPetAppearance Aparencia = FPetAppearance::ForType(TEXT("Natural/Vento"));
	TestTrue(TEXT("A cor declarada chega à aparência"),
		Aparencia.AccentColor.Equals(FLinearColor(0.75f, 0.90f, 0.95f), 0.001f));
	TestTrue(TEXT("O tombo declarado chega à aparência"),
		FMath::IsNearlyEqual(Aparencia.CrestRotation.Roll, -40.0f, 0.01f));

	// E a skill dele veio junto, do MESMO arquivo.
	const FPetElementDefinition* Vento = Catalogo.FindElement(TEXT("Vento"));
	TestTrue(TEXT("O elemento está no catálogo"), Vento != nullptr);
	TestTrue(TEXT("E carrega a skill declarada"),
		Vento && Vento->SkillNames.Contains(TEXT("voar")));

	return true;
}

// UMA ESCOLA NOVA também é uma linha.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNewSchoolNeedsNoCodeTest,
	"BattleSquare.Balance.PetTypeCatalog.NewSchoolNeedsNoCode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNewSchoolNeedsNoCodeTest::RunTest(const FString& Parameters)
{
	const FString Json = TEXT(R"({
		"schools": [
			{ "name": "Alquimia", "crest": "Antena", "crestScale": [0.05, 0.05, 0.52] }
		],
		"elements": [
			{ "name": "Fogo", "color": [0.95, 0.35, 0.05], "tiltDegrees": -6, "skills": [] }
		]
	})");

	FPetTypeCatalog Catalogo;
	TestTrue(TEXT("O catálogo carrega"),
		FPetTypeCatalog::LoadFromJson(EscreverCatalogoDeTeste(TEXT("alquimia.json"), Json), Catalogo));

	const FCatalogoTemporario EmUso(Catalogo);

	TestTrue(TEXT("A escola nova resolve"),
		FPetTypeIdentity::Parse(TEXT("Alquimia/Fogo")).IsValid());

	const FPetAppearance Aparencia = FPetAppearance::ForType(TEXT("Alquimia/Fogo"));
	TestTrue(TEXT("E a malha declarada chega à aparência"),
		Aparencia.CrestShape == EPetCrestShape::Antena);

	return true;
}

// Arquivo AUSENTE não vira tabela padrão embutida no C++.
//
// Um padrão em código seria a segunda cópia da mesma lista, e cópias
// concordam até a primeira edição — L-032 e L-033 foram exatamente isso. Sem
// o arquivo, o tipo não resolve e o pet sai VISIVELMENTE neutro.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissingCatalogHasNoCodeFallbackTest,
	"BattleSquare.Balance.PetTypeCatalog.MissingCatalogHasNoCodeFallback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMissingCatalogHasNoCodeFallbackTest::RunTest(const FString& Parameters)
{
	FPetTypeCatalog Ausente;
	TestFalse(TEXT("Arquivo inexistente não carrega"),
		FPetTypeCatalog::LoadFromJson(TEXT("/caminho/que/nao/existe.json"), Ausente));
	TestTrue(TEXT("E o catálogo continua vazio"), Ausente.IsEmpty());

	const FCatalogoTemporario EmUso(Ausente);

	TestFalse(TEXT("Com catálogo vazio, nenhum tipo resolve"),
		FPetTypeIdentity::Parse(TEXT("Natural/Fogo")).IsValid());

	// E o pet sai NEUTRO — visível, não invisível. Um pet da cor do chão é
	// tão ausente quanto um sem malha.
	const FPetAppearance Aparencia = FPetAppearance::ForType(TEXT("Natural/Fogo"));
	TestTrue(TEXT("O neutro é o ouro-palha, que nenhum terreno usa"),
		Aparencia.AccentColor.Equals(FLinearColor(0.79f, 0.64f, 0.15f), 0.001f));

	return true;
}

// O arquivo QUE O JOGO USA carrega, e declara o que o jogo espera.
//
// Os testes acima provam que o mecanismo funciona; este prova que o arquivo
// REAL está certo. Sem ele, o catálogo poderia estar quebrado com todos os
// outros testes verdes — que é a distância entre "o código funciona" e "o
// jogo funciona".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShippedTypeCatalogLoadsTest,
	"BattleSquare.Balance.PetTypeCatalog.ShippedCatalogLoads",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FShippedTypeCatalogLoadsTest::RunTest(const FString& Parameters)
{
	FPetTypeCatalog Catalogo;
	const FString Caminho = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetTypes.json"));
	if (!FPetTypeCatalog::LoadFromJson(Caminho, Catalogo))
	{
		AddError(TEXT("Config/PetTypes.json não carregou"));
		return false;
	}

	// Por NOME, e não por contagem. O número fixo quebrava a cada tipo novo
	// sem verificar nada que importasse — a pergunta útil é se cada eixo
	// declarado no arquivo chegou ao jogo, e não quantos são.
	const TCHAR* EscolasEsperadas[] = { TEXT("Fisica"), TEXT("Natural"),
		TEXT("Psiquica"), TEXT("Espiritual") };
	for (const TCHAR* Nome : EscolasEsperadas)
	{
		TestTrue(*FString::Printf(TEXT("A escola %s está no catálogo"), Nome),
			Catalogo.GetSchools().ContainsByPredicate(
				[Nome](const FPetSchoolDefinition& Escola) { return Escola.Name == Nome; }));
	}

	const TCHAR* ElementosEsperados[] = { TEXT("Fogo"), TEXT("Agua"), TEXT("Planta"),
		TEXT("Terra"), TEXT("Fantasma"), TEXT("Luz"), TEXT("Ar") };
	for (const TCHAR* Nome : ElementosEsperados)
	{
		TestTrue(*FString::Printf(TEXT("O elemento %s está no catálogo"), Nome),
			Catalogo.GetElements().ContainsByPredicate(
				[Nome](const FPetElementDefinition& Elemento) { return Elemento.Name == Nome; }));
	}

	// Cada elemento precisa dos DOIS canais: cor e tombo. Um elemento sem
	// tombo próprio seria invisível para quem não distingue matiz.
	for (const FPetElementDefinition& Elemento : Catalogo.GetElements())
	{
		for (const FPetElementDefinition& Outro : Catalogo.GetElements())
		{
			if (&Elemento == &Outro)
			{
				continue;
			}
			TestFalse(*FString::Printf(TEXT("%s e %s têm tombos diferentes"),
				*Elemento.Name, *Outro.Name),
				FMath::IsNearlyEqual(Elemento.TiltDegrees, Outro.TiltDegrees, 1.0f));
			TestFalse(*FString::Printf(TEXT("%s e %s têm cores diferentes"),
				*Elemento.Name, *Outro.Name),
				Elemento.Color.Equals(Outro.Color, 0.001f));
		}
	}

	// E os nomes ANTIGOS, que já foram assinados, continuam resolvendo.
	FString Escola;
	FString Elemento;
	TestTrue(TEXT("'Magico' ainda resolve"),
		Catalogo.ResolveLegacyName(TEXT("Magico"), Escola, Elemento));
	TestEqual(TEXT("E vira Psiquica"), Escola, FString(TEXT("Psiquica")));

	return true;
}

// UMA lista de elementos, e só uma.
//
// As skills viviam num arquivo à parte que repetia os nomes dos elementos —
// a segunda cópia da mesma lista. Cópias concordam até a primeira edição, e
// aqui a discordância seria um elemento existindo para a COR e não para a
// SKILL: o pet aparece certo e não sabe fazer nada, sem nada apontando a causa.
//
// Este teste falha se alguém recriar o arquivo separado, e é de propósito: a
// duplicação volta por conveniência, não por decisão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillsComeFromTheOneTypeCatalogTest,
	"BattleSquare.Balance.PetTypeCatalog.SkillsComeFromTheOneCatalog",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSkillsComeFromTheOneTypeCatalogTest::RunTest(const FString& Parameters)
{
	// O arquivo separado NÃO existe mais. Se voltar, há duas listas de novo.
	TestFalse(TEXT("Config/PetSkills.json não existe — a lista é uma só"),
		FPlatformFileManager::Get().GetPlatformFile().FileExists(
			*FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetSkills.json"))));

	FPetTypeCatalog Tipos;
	if (!FPetTypeCatalog::LoadFromJson(
		FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetTypes.json")), Tipos))
	{
		AddError(TEXT("Config/PetTypes.json não carregou"));
		return false;
	}

	const FPetSkillCatalog Skills = FPetSkillCatalog::FromTypeCatalog(Tipos);

	// VOAR MIGROU do fogo para o AR, e a razão é simples: fogo voando sempre
	// foi convenção de dragão, não característica de fogo. O ar é o dono
	// natural, e o fogo ganhou `incendiar` — porque nenhum elemento fica sem.
	TestTrue(TEXT("Ar voa"),
		Skills.GetSkillsForType(TEXT("Natural/Ar")).Contains(EActionType::Voar));
	TestFalse(TEXT("E o fogo não voa mais"),
		Skills.GetSkillsForType(TEXT("Natural/Fogo")).Contains(EActionType::Voar));
	TestTrue(TEXT("O fogo incendeia"),
		Skills.GetSkillsForType(TEXT("Natural/Fogo")).Contains(EActionType::Incendiar));
	TestTrue(TEXT("Água submerge"),
		Skills.GetSkillsForType(TEXT("Natural/Agua")).Contains(EActionType::Submergir));
	TestTrue(TEXT("Planta camufla"),
		Skills.GetSkillsForType(TEXT("Fisica/Planta")).Contains(EActionType::Camuflar));

	// A skill é do ELEMENTO, não da escola: dois pets de fogo voam pelo mesmo
	// motivo, que é serem os dois de fogo.
	TestEqual(TEXT("Psiquica/Fogo tem a mesma skill de Natural/Fogo"),
		Skills.GetSkillsForType(TEXT("Psiquica/Fogo")).Num(),
		Skills.GetSkillsForType(TEXT("Natural/Fogo")).Num());

	// A TERRA ESCAVA. Ela era o único elemento sem skill, e a ausência estava
	// declarada aqui como se fosse decisão — mas era buraco: fogo voa, água
	// submerge, planta camufla, fantasma atravessa, luz ilumina, e o pet de
	// terra não tinha nada de seu. Num sistema de trabalhos ele seria o pior
	// elemento por omissão.
	TestTrue(TEXT("Terra escava"),
		Skills.GetSkillsForType(TEXT("Fisica/Terra")).Contains(EActionType::Escavar));

	// E a regra que substitui a ausência: NENHUM elemento fica sem skill. É
	// mais forte que o teste antigo, porque o antigo travava justamente o
	// buraco em vez de proibi-lo.
	for (const FPetElementDefinition& Elemento : Tipos.GetElements())
	{
		const FString Tipo = FString::Printf(TEXT("Fisica/%s"), *Elemento.Name);
		TestTrue(*FString::Printf(TEXT("O elemento %s tem ao menos uma skill"), *Elemento.Name),
			Skills.GetSkillsForType(Tipo).Num() > 0);

		// Skill NÃO substitui a gramática: os universais continuam sendo de
		// todo pet, e o que o elemento dá vem POR CIMA deles.
		TestTrue(*FString::Printf(TEXT("%s mantém as ações universais"), *Elemento.Name),
			Skills.GetAvailableActionsForType(Tipo).Num()
				> FPetSkillCatalog::GetUniversalActions().Num());
	}

	return true;
}
