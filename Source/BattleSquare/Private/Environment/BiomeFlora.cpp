// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/BiomeFlora.h"

namespace BiomeFlora
{
	/**
	 * FLORESTA — a referência. Folhosa em primeiro plano, copa redonda no
	 * fundo, pedra arredondada, três verdes empilhados.
	 *
	 * Fora dela, de propósito: `rock_tallA`/`rock_tallD` (agulha é vulcão),
	 * `tree_thin` (praia e pântano) e as três `tree_pineTall*_detailed`
	 * (geleira). Eram justamente essas que faziam a floresta parecer as
	 * outras.
	 */
	constexpr const TCHAR* ElencoDaFloresta[] = {
		TEXT("grass_large"), TEXT("grass_leafs"),
		TEXT("flower_redA"), TEXT("flower_yellowA"), TEXT("mushroom_red"),
		TEXT("plant_bushSmall"), TEXT("plant_bush"), TEXT("plant_bushLarge"),
		TEXT("stump_round"), TEXT("log"),
		TEXT("rock_smallA"), TEXT("rock_smallD"), TEXT("rock_largeA"), TEXT("rock_largeC"),
		TEXT("tree_oak"), TEXT("tree_blocks"), TEXT("tree_pineSmallA"), TEXT("tree_pineSmallC"),
		TEXT("tree_pineRoundA"), TEXT("tree_pineRoundC"), TEXT("tree_default"), TEXT("tree_tall"),
	};

	/**
	 * PRAIA — rampa clara com pouca coisa em pé. Nenhuma copa fechada: praia
	 * com dossel é floresta com areia.
	 *
	 * A árvore de beira não está aqui porque não sai da tabela de espécies —
	 * ela é o agrupamento `ShoreTrees`, inclinado para o mar.
	 */
	constexpr const TCHAR* ElencoDaPraia[] = {
		TEXT("grass_leafs"), TEXT("plant_bushSmall"), TEXT("log"), TEXT("rock_smallA"),
	};

	/** DESERTO — mesa e seixo sobre duna. Pedra CHAPADA, e nada verde. */
	constexpr const TCHAR* ElencoDoDeserto[] = {
		TEXT("plant_bushSmall"), TEXT("stump_round"),
		TEXT("rock_largeA"), TEXT("rock_smallD"),
	};

	/**
	 * GELEIRA — agulha escura contra o branco. Só conífera alta e estreita:
	 * carvalho na neve foi o defeito mais visível do elenco único.
	 */
	constexpr const TCHAR* ElencoDaGeleira[] = {
		TEXT("log"), TEXT("rock_largeC"), TEXT("rock_tallD"),
		TEXT("tree_pineTallA_detailed"), TEXT("tree_pineTallB_detailed"),
		TEXT("tree_pineTallC_detailed"), TEXT("tree_thin"),
	};

	/** VULCÃO — pedra sobre pedra, e é aqui que a agulha cabe. Nada vivo. */
	constexpr const TCHAR* ElencoDoVulcao[] = {
		TEXT("stump_round"), TEXT("log"),
		TEXT("rock_tallA"), TEXT("rock_tallD"), TEXT("rock_largeC"),
	};

	/**
	 * PÂNTANO — tronco fino e alto, copa rala, cogumelo. Cogumelo sim, flor
	 * não: flor de jardim na lama era o que fazia o pântano parecer um
	 * canteiro alagado.
	 */
	constexpr const TCHAR* ElencoDoPantano[] = {
		TEXT("grass_large"), TEXT("mushroom_red"),
		TEXT("plant_bush"), TEXT("plant_bushLarge"),
		TEXT("stump_round"), TEXT("log"), TEXT("rock_smallD"),
		TEXT("tree_thin"), TEXT("tree_tall"), TEXT("tree_blocks"),
	};

	namespace
	{
		/**
		 * O elenco cru de um bioma, como par ponteiro/tamanho.
		 *
		 * Um único `switch` para as duas funções públicas: duas listagens do
		 * mesmo elenco discordariam na primeira edição, e o sintoma seria o
		 * teste aprovando um elenco que a mata não planta.
		 */
		void ElencoCruDaFlora(EIslandBiome Bioma, const TCHAR* const*& OutNomes, int32& OutTotal)
		{
			switch (Bioma)
			{
			case EIslandBiome::Beach:
				OutNomes = ElencoDaPraia;
				OutTotal = UE_ARRAY_COUNT(ElencoDaPraia);
				return;
			case EIslandBiome::Desert:
				OutNomes = ElencoDoDeserto;
				OutTotal = UE_ARRAY_COUNT(ElencoDoDeserto);
				return;
			case EIslandBiome::Glacier:
				OutNomes = ElencoDaGeleira;
				OutTotal = UE_ARRAY_COUNT(ElencoDaGeleira);
				return;
			case EIslandBiome::Volcano:
				OutNomes = ElencoDoVulcao;
				OutTotal = UE_ARRAY_COUNT(ElencoDoVulcao);
				return;
			case EIslandBiome::Swamp:
				OutNomes = ElencoDoPantano;
				OutTotal = UE_ARRAY_COUNT(ElencoDoPantano);
				return;
			case EIslandBiome::Forest:
			default:
				// Floresta é o padrão porque é o bioma de casa: bioma novo sem
				// elenco escrito nasce parecendo a mata de sempre, que é
				// feio-conhecido em vez de chão pelado.
				OutNomes = ElencoDaFloresta;
				OutTotal = UE_ARRAY_COUNT(ElencoDaFloresta);
				return;
			}
		}
	}

	bool FitsBiome(const FString& SpeciesName, EIslandBiome Biome)
	{
		const TCHAR* const* Nomes = nullptr;
		int32 Total = 0;
		ElencoCruDaFlora(Biome, Nomes, Total);

		for (int32 Indice = 0; Indice < Total; ++Indice)
		{
			if (SpeciesName.Equals(Nomes[Indice], ESearchCase::CaseSensitive))
			{
				return true;
			}
		}

		return false;
	}

	TArray<FString> AllowedSpecies(EIslandBiome Biome)
	{
		const TCHAR* const* Nomes = nullptr;
		int32 Total = 0;
		ElencoCruDaFlora(Biome, Nomes, Total);

		TArray<FString> Elenco;
		Elenco.Reserve(Total);
		for (int32 Indice = 0; Indice < Total; ++Indice)
		{
			Elenco.Add(Nomes[Indice]);
		}

		return Elenco;
	}
}
