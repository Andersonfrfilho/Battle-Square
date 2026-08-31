// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandGeography.h"

#include "Misc/ConfigCacheIni.h"

namespace GeografiaDaIlha
{
	/** O raio de hoje. Crescer a ilha é mudar ESTE número, e só ele. */
	constexpr float RaioDaTerraPadrao = 6000.0f;

	/** Seção e chave do `.ini`, ao lado das outras medidas do mundo. */
	const TCHAR* SecaoDoMundo = TEXT("/Script/BattleSquare.BattleSquareGameMode");
	const TCHAR* ChaveDoRaio = TEXT("WorldIslandRadiusUnits");

	/**
	 * A praia é FRAÇÃO do raio, não um número fixo.
	 *
	 * Com número fixo, a mesma faixa que é uma praia na ilha de hoje viraria
	 * um risco de areia quando a ilha crescer três vezes.
	 */
	constexpr float FracaoDaPraia = 0.08f;

	/**
	 * O pântano é mais largo que a praia porque ele é o que se ATRAVESSA.
	 *
	 * A praia é uma orla: passa-se por ela para chegar na água. O brejo é
	 * caminho, e uma faixa tão fina quanto a areia seria atravessada sem
	 * ninguém reparar que mudou de lugar.
	 */
	constexpr float FracaoDoPantano = 0.10f;

	/**
	 * O PISO do miolo de mata, em unidades.
	 *
	 * Fixo porque ele existe para caber os campos de treino, que têm tamanho
	 * próprio e não crescem com a ilha. Quem garante que cabe é
	 * `IslandGeographyTest`, não este comentário.
	 */
	constexpr float RaioMinimoDaCasa = 2600.0f;

	/**
	 * E a fração, que é quem manda quando a ilha é grande.
	 *
	 * Só o número fixo é o que fez a mata "desaparecer": os setores de bioma
	 * são fatias que se encontram no CENTRO, então fora do miolo eles chegam
	 * todos juntos. Com 2600 numa ilha de 20000, o jogador dava sessenta
	 * metros e já estava em rocha vulcânica — e o pedaço de mundo tem 6400 de
	 * lado, ou seja, nem UM pedaço inteiro de mata cabia no miolo.
	 *
	 * Trinta e cinco por cento dá 7000 na ilha de hoje: um punhado de pedaços
	 * de mata em toda direção antes da primeira divisa. A mata é a casa, e
	 * casa se atravessa andando, não em três passos.
	 */
	constexpr float FracaoDaCasa = 0.35f;

	constexpr float GrausDaVolta = 360.0f;

	/**
	 * Que bioma tem cada setor.
	 *
	 * Dois de mata porque a mata é a casa, e porque é o único bioma que hoje
	 * tem povoamento autoral — os outros nascem com chão e cor antes de terem
	 * plantas próprias.
	 *
	 * Glaciar entre vulcão e mata não é engano: é a Islândia, e é o arranjo em
	 * que a aurora tem onde aparecer sem ficar longe de tudo.
	 */
	const EIslandBiome BiomaDoSetor[IslandGeography::SectorCount] = {
		EIslandBiome::Forest,
		EIslandBiome::Desert,
		EIslandBiome::Volcano,
		EIslandBiome::Glacier,
		EIslandBiome::Forest,
	};
}

namespace IslandGeography
{
	float LandRadiusUnits()
	{
		float Configurado = GeografiaDaIlha::RaioDaTerraPadrao;
		if (GConfig)
		{
			GConfig->GetFloat(GeografiaDaIlha::SecaoDoMundo, GeografiaDaIlha::ChaveDoRaio,
				Configurado, GGameIni);
		}
		return Configurado > 0.0f ? Configurado : GeografiaDaIlha::RaioDaTerraPadrao;
	}

	float BeachWidthUnits()
	{
		return LandRadiusUnits() * GeografiaDaIlha::FracaoDaPraia;
	}

	float SwampWidthUnits()
	{
		return LandRadiusUnits() * GeografiaDaIlha::FracaoDoPantano;
	}

	float HomeRadiusUnits()
	{
		// Numa ilha pequena demais para ter miolo E borda, a casa cede: ela
		// nunca pode empurrar a praia para dentro do próprio centro.
		const float Pedido = FMath::Max(GeografiaDaIlha::RaioMinimoDaCasa,
			LandRadiusUnits() * GeografiaDaIlha::FracaoDaCasa);
		return FMath::Min(Pedido, LandRadiusUnits() - BeachWidthUnits());
	}

	int32 SectorAt(const FVector2D& PositionUnits)
	{
		const float Graus = FMath::RadiansToDegrees(
			FMath::Atan2(PositionUnits.Y, PositionUnits.X));
		const float Voltado = Graus < 0.0f ? Graus + GeografiaDaIlha::GrausDaVolta : Graus;

		const float GrausPorSetor = GeografiaDaIlha::GrausDaVolta / SectorCount;
		return FMath::Clamp(FMath::FloorToInt(Voltado / GrausPorSetor), 0, SectorCount - 1);
	}

	EIslandBiome IslandBiome()
	{
		// UMA ILHA, UM BIOMA.
		//
		// A ilha era fatiada em seis setores de pizza, e cada bioma ficava com
		// 0,39 km² — um deserto desse tamanho não é deserto, é caixa de areia.
		// Com uma ilha por bioma, cada um passa a ter a ilha inteira: 6 km².
		//
		// E é o que faz o resto do desenho fechar. A fronteira exige o ranking,
		// o que faz sentido ENTRE ilhas e seria travar a caminhada para o norte
		// dentro de uma. As espécies são próprias da região, o que só significa
		// alguma coisa se a região for um lugar inteiro.
		FString Escrito;
		if (GConfig && GConfig->GetString(GeografiaDaIlha::SecaoDoMundo,
			TEXT("WorldIslandBiome"), Escrito, GGameIni))
		{
			const FName Nome(*Escrito);
			if (Nome == TEXT("Desert"))  { return EIslandBiome::Desert; }
			if (Nome == TEXT("Glacier")) { return EIslandBiome::Glacier; }
			if (Nome == TEXT("Volcano")) { return EIslandBiome::Volcano; }
			if (Nome == TEXT("Swamp"))   { return EIslandBiome::Swamp; }
		}

		// A mata é o padrão, e é a ilha que existe hoje.
		return EIslandBiome::Forest;
	}

	EIslandBiome BiomeOfSector(int32 Sector)
	{
		// O setor deixou de decidir bioma: a ilha inteira é um só. A função
		// continua existindo porque o MAPA e o clima ainda perguntam por
		// posição, e mudar todas as chamadas de uma vez seria trocar duas
		// coisas no mesmo passo.
		(void)Sector;
		return IslandBiome();
	}

	float VolcanoAngleDegrees() { return 180.0f; }

	float VolcanoRingUnits() { return LandRadiusUnits() * 0.75f; }

	FVector2D VolcanoCenterUnits()
	{
		const float Radianos = FMath::DegreesToRadians(VolcanoAngleDegrees());
		return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * VolcanoRingUnits();
	}

	float VolcanoHeatRadiusUnits() { return LandRadiusUnits() * 0.30f; }

	float VolcanoScorchedRadiusUnits() { return LandRadiusUnits() * 0.10f; }

	EIslandBiome BiomeAt(const FVector2D& PositionUnits)
	{
		const float Distancia = PositionUnits.Size();


		// A ordem é a regra. Trocá-la põe deserto encostado no mar e glaciar
		// em cima dos campos de treino.
		// O MIOLO é o bioma da ilha, e não mata fixa. Numa ilha de geleira, a
		// casa do jogador é geleira — ele nasce no lugar onde vive.
		if (Distancia <= HomeRadiusUnits())
		{
			return IslandBiome();
		}
		if (Distancia >= LandRadiusUnits() - BeachWidthUnits())
		{
			return EIslandBiome::Beach;
		}

		// O chão em volta da cratera é queimado em qualquer ilha: o vulcão é
		// marco, não bioma. Vem DEPOIS da praia — a areia da borda é o que a
		// pessoa vê primeiro, e lava chegando no mar é outra coisa.
		if (FVector2D::Distance(PositionUnits, VolcanoCenterUnits()) <= VolcanoScorchedRadiusUnits())
		{
			return EIslandBiome::Volcano;
		}

		const EIslandBiome DoSetor = IslandBiome();

		// Brejo é mata que não drena. Onde o setor já é seco, a mesma faixa
		// baixa continua sendo o que o setor diz: encostar o deserto no mar
		// dá salina, não pântano.
		const bool bAtrasDaPraia = Distancia
			>= LandRadiusUnits() - BeachWidthUnits() - SwampWidthUnits();
		if (bAtrasDaPraia && DoSetor == EIslandBiome::Forest)
		{
			return EIslandBiome::Swamp;
		}

		return DoSetor;
	}

	EScenaryClimate ClimateOf(EIslandBiome Biome)
	{
		switch (Biome)
		{
		case EIslandBiome::Desert:  return EScenaryClimate::Desert;
		case EIslandBiome::Volcano: return EScenaryClimate::Desert;
		case EIslandBiome::Glacier: return EScenaryClimate::Cold;
		case EIslandBiome::Beach:   return EScenaryClimate::Mild;
		case EIslandBiome::Swamp:   return EScenaryClimate::Humid;
		case EIslandBiome::Forest:  break;
		}

		// Sem `default:`, e é o ponto. Com ele, o pântano teria nascido
		// temperado sem ninguém escrever nada errado — foi assim que o vulcão
		// quase nasceu caverna. Bioma novo sem caso aqui não compila.
		return EScenaryClimate::Temperate;
	}

	EScenaryClimate ClimateAt(const FVector2D& PositionUnits)
	{
		return ClimateOf(BiomeAt(PositionUnits));
	}

	EScenaryClimate SectorClimateAt(const FVector2D& PositionUnits)
	{
		return ClimateOf(BiomeOfSector(SectorAt(PositionUnits)));
	}

	bool IsOnLand(const FVector2D& PositionUnits)
	{
		return PositionUnits.Size() <= LandRadiusUnits();
	}

	const TCHAR* BiomeDebugName(EIslandBiome Biome)
	{
		switch (Biome)
		{
		case EIslandBiome::Desert:  return TEXT("deserto");
		case EIslandBiome::Volcano: return TEXT("vulcao");
		case EIslandBiome::Glacier: return TEXT("geleira");
		case EIslandBiome::Beach:   return TEXT("praia");
		case EIslandBiome::Swamp:   return TEXT("pantano");
		case EIslandBiome::Forest:  break;
		}

		return TEXT("mata");
	}
}
