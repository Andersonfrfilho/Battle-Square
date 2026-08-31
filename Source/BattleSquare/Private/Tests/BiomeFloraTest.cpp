// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/BiomeFlora.h"

#include "Environment/ForestBackdrop.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"

namespace
{
	/** Os seis biomas, para o teste que precisa varrer todos. */
	TArray<EIslandBiome> BiomasDaFlora()
	{
		return {
			EIslandBiome::Forest,
			EIslandBiome::Desert,
			EIslandBiome::Glacier,
			EIslandBiome::Volcano,
			EIslandBiome::Beach,
			EIslandBiome::Swamp,
		};
	}

	/** Só as árvores de um elenco — é nelas que a separação faltava. */
	TArray<FString> ArvoresDoElencoDaFlora(EIslandBiome Bioma)
	{
		TArray<FString> Arvores;
		for (const FString& Nome : BiomeFlora::AllowedSpecies(Bioma))
		{
			if (Nome.StartsWith(TEXT("tree_"), ESearchCase::CaseSensitive))
			{
				Arvores.Add(Nome);
			}
		}

		Arvores.Sort();
		return Arvores;
	}
}

/**
 * Nome de elenco que não existe na tabela de espécies não planta nada, e nada
 * avisa. É o modo de falhar que a lista de elenco introduz.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBiomeFloraCastHasNoInventedName,
	"BattleSquare.Environment.BiomeFlora.ElencoNaoTemNomeInventado",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBiomeFloraCastHasNoInventedName::RunTest(const FString& Parameters)
{
	const TArray<FString> Tabela = AForestBackdrop::SpeciesNames();
	TestTrue(TEXT("a tabela de espécies não está vazia"), Tabela.Num() > 0);

	for (const EIslandBiome Bioma : BiomasDaFlora())
	{
		const TArray<FString> Elenco = BiomeFlora::AllowedSpecies(Bioma);
		TestTrue(FString::Printf(TEXT("bioma %s tem elenco"),
			IslandGeography::BiomeDebugName(Bioma)), Elenco.Num() > 0);

		for (const FString& Nome : Elenco)
		{
			TestTrue(FString::Printf(TEXT("%s (bioma %s) existe na tabela de espécies"),
					*Nome, IslandGeography::BiomeDebugName(Bioma)),
				Tabela.Contains(Nome));

			// E o elenco tem de concordar com a pergunta que o plantio faz.
			TestTrue(FString::Printf(TEXT("%s cabe no bioma que o lista"), *Nome),
				BiomeFlora::FitsBiome(Nome, Bioma));
		}
	}

	return true;
}

/**
 * Elenco único com proporção diferente não faz lugar diferente. Este teste é
 * o que impede a volta do carvalho na geleira.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBiomeFloraTreesDifferPerBiome,
	"BattleSquare.Environment.BiomeFlora.ArvoresSeparadasPorBioma",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBiomeFloraTreesDifferPerBiome::RunTest(const FString& Parameters)
{
	const TArray<FString> DaFloresta = ArvoresDoElencoDaFlora(EIslandBiome::Forest);
	const TArray<FString> DaGeleira = ArvoresDoElencoDaFlora(EIslandBiome::Glacier);
	const TArray<FString> DoPantano = ArvoresDoElencoDaFlora(EIslandBiome::Swamp);

	TestTrue(TEXT("floresta tem árvore"), DaFloresta.Num() > 0);
	TestTrue(TEXT("geleira tem árvore"), DaGeleira.Num() > 0);
	TestTrue(TEXT("pântano tem árvore"), DoPantano.Num() > 0);

	// Nenhuma árvore em comum entre floresta e geleira: é o caso que o
	// jogador viu, e o que a separação existe para desfazer.
	for (const FString& Nome : DaFloresta)
	{
		TestFalse(FString::Printf(TEXT("%s da floresta não aparece na geleira"), *Nome),
			DaGeleira.Contains(Nome));
	}

	// Folhosa é da floresta; conífera alta é da geleira.
	TestTrue(TEXT("carvalho é da floresta"),
		BiomeFlora::FitsBiome(TEXT("tree_oak"), EIslandBiome::Forest));
	TestFalse(TEXT("carvalho NÃO é da geleira"),
		BiomeFlora::FitsBiome(TEXT("tree_oak"), EIslandBiome::Glacier));
	TestTrue(TEXT("conífera alta é da geleira"),
		BiomeFlora::FitsBiome(TEXT("tree_pineTallA_detailed"), EIslandBiome::Glacier));
	TestFalse(TEXT("conífera alta NÃO é da floresta"),
		BiomeFlora::FitsBiome(TEXT("tree_pineTallA_detailed"), EIslandBiome::Forest));

	// Agulha de pedra é vulcão, e sair dela da floresta foi metade do
	// conserto da mata de casa.
	TestTrue(TEXT("agulha de pedra é do vulcão"),
		BiomeFlora::FitsBiome(TEXT("rock_tallA"), EIslandBiome::Volcano));
	TestFalse(TEXT("agulha de pedra NÃO é da floresta"),
		BiomeFlora::FitsBiome(TEXT("rock_tallA"), EIslandBiome::Forest));

	// Flor de jardim na lama fazia o pântano parecer canteiro alagado.
	TestTrue(TEXT("cogumelo é do pântano"),
		BiomeFlora::FitsBiome(TEXT("mushroom_red"), EIslandBiome::Swamp));
	TestFalse(TEXT("flor NÃO é do pântano"),
		BiomeFlora::FitsBiome(TEXT("flower_redA"), EIslandBiome::Swamp));

	return true;
}

/** Deserto e vulcão sem árvore nenhuma — é o ponto dos dois. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBiomeFloraBareBiomesHaveNoTree,
	"BattleSquare.Environment.BiomeFlora.DesertoEVulcaoSemArvore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBiomeFloraBareBiomesHaveNoTree::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("deserto não tem árvore"),
		ArvoresDoElencoDaFlora(EIslandBiome::Desert).Num(), 0);
	TestEqual(TEXT("vulcão não tem árvore"),
		ArvoresDoElencoDaFlora(EIslandBiome::Volcano).Num(), 0);

	// E nem capim: deserto verde é o mesmo defeito por outro papel.
	TestFalse(TEXT("capim NÃO é do deserto"),
		BiomeFlora::FitsBiome(TEXT("grass_large"), EIslandBiome::Desert));
	TestFalse(TEXT("capim NÃO é do vulcão"),
		BiomeFlora::FitsBiome(TEXT("grass_large"), EIslandBiome::Volcano));

	return true;
}

/**
 * Espécie desconhecida fica de fora. Devolver `true` a espalharia calada por
 * todos os seis biomas — que é exatamente o estado de onde este arquivo veio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBiomeFloraRejectsUnknownSpecies,
	"BattleSquare.Environment.BiomeFlora.RecusaEspecieDesconhecida",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBiomeFloraRejectsUnknownSpecies::RunTest(const FString& Parameters)
{
	for (const EIslandBiome Bioma : BiomasDaFlora())
	{
		TestFalse(FString::Printf(TEXT("nome inventado fora do bioma %s"),
				IslandGeography::BiomeDebugName(Bioma)),
			BiomeFlora::FitsBiome(TEXT("tree_que_nao_existe"), Bioma));

		TestFalse(FString::Printf(TEXT("nome vazio fora do bioma %s"),
				IslandGeography::BiomeDebugName(Bioma)),
			BiomeFlora::FitsBiome(FString(), Bioma));
	}

	// Sensível a caixa: `Tree_Oak` não é `tree_oak`, e o caminho do asset é
	// montado com o nome literal. Aceitar as duas caixas plantaria nada e
	// diria que plantou.
	TestFalse(TEXT("caixa diferente não passa"),
		BiomeFlora::FitsBiome(TEXT("Tree_Oak"), EIslandBiome::Forest));

	return true;
}
