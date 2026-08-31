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
	 * O miolo de mata, em unidades.
	 *
	 * É FIXO, e não fração: ele existe para caber os campos de treino, que têm
	 * tamanho próprio e não crescem com a ilha. Quem garante que cabe é
	 * `IslandGeographyTest`, não este comentário.
	 */
	constexpr float RaioDaCasa = 2600.0f;

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
		return FMath::Min(GeografiaDaIlha::RaioDaCasa,
			LandRadiusUnits() - BeachWidthUnits());
	}

	int32 SectorAt(const FVector2D& PositionUnits)
	{
		const float Graus = FMath::RadiansToDegrees(
			FMath::Atan2(PositionUnits.Y, PositionUnits.X));
		const float Voltado = Graus < 0.0f ? Graus + GeografiaDaIlha::GrausDaVolta : Graus;

		const float GrausPorSetor = GeografiaDaIlha::GrausDaVolta / SectorCount;
		return FMath::Clamp(FMath::FloorToInt(Voltado / GrausPorSetor), 0, SectorCount - 1);
	}

	EIslandBiome BiomeOfSector(int32 Sector)
	{
		const int32 Seguro = FMath::Clamp(Sector, 0, SectorCount - 1);
		return GeografiaDaIlha::BiomaDoSetor[Seguro];
	}

	EIslandBiome BiomeAt(const FVector2D& PositionUnits)
	{
		const float Distancia = PositionUnits.Size();

		// A ordem é a regra. Trocá-la põe deserto encostado no mar e glaciar
		// em cima dos campos de treino.
		if (Distancia <= HomeRadiusUnits())
		{
			return EIslandBiome::Forest;
		}
		if (Distancia >= LandRadiusUnits() - BeachWidthUnits())
		{
			return EIslandBiome::Beach;
		}

		const EIslandBiome DoSetor = BiomeOfSector(SectorAt(PositionUnits));

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
