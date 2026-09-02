#include "World/IslandBakedPlan.h"

#include "Environment/CaveLabyrinth.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"
#include "Net/BattleSquareGameMode.h"
#include "World/AqueductLayout.h"
#include "World/TrailLayout.h"
#include "World/WorldBudget.h"

float UIslandBakedPlan::HeightAtCell(int32 Column, int32 Row) const
{
	const int32 Indice = Row * HeightGridSide + Column;
	return GroundHeightUnits.IsValidIndex(Indice) ? GroundHeightUnits[Indice] : 0.0f;
}

namespace IslandBakedPlan
{
	const TCHAR* AssetPath()
	{
		return TEXT("/Game/World/IslandBaked.IslandBaked");
	}

	int32 RiverSampleCount()
	{
		return 41;
	}

	int32 HeightGridSide()
	{
		return 180;
	}

	float ProgressAtSample(int32 Sample)
	{
		return static_cast<float>(Sample) / static_cast<float>(RiverSampleCount() - 1);
	}

	/**
	 * As cavernas do mundo, numa lista só.
	 *
	 * Elas vêm de dois lugares — as peças da ilha e as grutas da água — e o
	 * mundo não distingue as duas: caverna é caverna. Juntar aqui evita que
	 * cada consumidor lembre de somar as duas listas, que é o tipo de coisa
	 * que alguém esquece na terceira edição.
	 */
	TArray<IslandFeatureLayout::FFeaturePlacement> AllCaves()
	{
		TArray<IslandFeatureLayout::FFeaturePlacement> Cavernas;
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature == IslandFeatureLayout::EIslandFeature::Cave)
			{
				Cavernas.Add(Peca);
			}
		}
		Cavernas.Append(FreshWater::PlanGrottoes());
		return Cavernas;
	}

	int32 WorldScenerySeed()
	{
		const ABattleSquareGameMode* Padrao = GetDefault<ABattleSquareGameMode>();
		return Padrao ? Padrao->GetWorldScenerySeedForMap() : 0;
	}

	FIslandParameters GatherParameters()
	{
		const EIslandBiome Bioma = IslandGeography::IslandBiome();

		FIslandParameters Parametros;
		Parametros.LandRadiusUnits = IslandGeography::LandRadiusUnits();
		Parametros.CoastShape = static_cast<uint8>(IslandGeography::CoastShape());
		Parametros.Biome = static_cast<uint8>(Bioma);
		Parametros.ScenerySeed = WorldScenerySeed();
		Parametros.WaterCoverage = WorldBudget::WaterCoverage(Bioma);
		Parametros.GroveCount = WorldBudget::GroveCount(Bioma);
		Parametros.HiddenClearingCount = WorldBudget::HiddenClearingCount(Bioma);
		Parametros.BreederCount = WorldBudget::BreederCount(Bioma);
		Parametros.FarmsPerSettlement = WorldBudget::FarmsPerSettlement(Bioma);
		Parametros.TendedOrchardCount = WorldBudget::TendedOrchardCount(Bioma);
		Parametros.WildOrchardCount = WorldBudget::WildOrchardCount(Bioma);
		Parametros.RoadsideShopCount = WorldBudget::RoadsideShopCount(Bioma);
		Parametros.CampCount = WorldBudget::CampCount(Bioma);
		Parametros.RuinCount = WorldBudget::RuinCount(Bioma);
		Parametros.StraightGalleryShare = WorldBudget::StraightGalleryShare(Bioma);
		Parametros.GraveyardsPerSettlement = WorldBudget::GraveyardsPerSettlement(Bioma);
		Parametros.ForgottenGraveyardCount = WorldBudget::ForgottenGraveyardCount(Bioma);
		Parametros.ForestDensity = WorldBudget::ForestDensity(Bioma);
		return Parametros;
	}

	uint32 HashParameters(const FIslandParameters& Parametros)
	{
		// Percorre a REFLEXÃO em vez de listar campo a campo. Uma lista à parte
		// envelhece calada: o campo novo entra na struct, ninguém o acrescenta
		// aqui, e a guarda passa a aprovar um assado que já não corresponde.
		uint32 Resumo = 0;
		for (TFieldIterator<FProperty> Campo(FIslandParameters::StaticStruct());
			Campo; ++Campo)
		{
			const void* Valor = Campo->ContainerPtrToValuePtr<void>(&Parametros);
			Resumo = HashCombine(Resumo, Campo->GetValueTypeHash(Valor));
			// O NOME entra junto: sem ele, trocar dois campos de mesmo tipo e
			// mesmo valor de lugar não mudaria resumo nenhum.
			//
			// E entra como TEXTO, nunca como `FName`. O resumo de um `FName` é
			// o índice dele na tabela de nomes do PROCESSO, e essa tabela é
			// montada em ordem diferente a cada execução: o resumo saía
			// diferente ao assar e ao carregar, com os parâmetros idênticos.
			// Uma guarda que acusa divergência toda vez é uma guarda ignorada.
			Resumo = HashCombine(Resumo, GetTypeHash(Campo->GetName()));
		}
		return Resumo;
	}

	TArray<FString> DescribeParameterDivergence(
		const FIslandParameters& Assado, const FIslandParameters& Agora)
	{
		TArray<FString> Divergiram;
		for (TFieldIterator<FProperty> Campo(FIslandParameters::StaticStruct());
			Campo; ++Campo)
		{
			const void* DoAssado = Campo->ContainerPtrToValuePtr<void>(&Assado);
			const void* DeAgora = Campo->ContainerPtrToValuePtr<void>(&Agora);
			if (!Campo->Identical(DoAssado, DeAgora))
			{
				FString ValorAssado;
				FString ValorAgora;
				Campo->ExportTextItem_Direct(ValorAssado, DoAssado, nullptr, nullptr, PPF_None);
				Campo->ExportTextItem_Direct(ValorAgora, DeAgora, nullptr, nullptr, PPF_None);
				Divergiram.Add(FString::Printf(TEXT("%s: assado %s, agora %s"),
					*Campo->GetName(), *ValorAssado, *ValorAgora));
			}
		}
		return Divergiram;
	}

	void BakeInto(UIslandBakedPlan& Out)
	{
		Out.Parameters = GatherParameters();
		Out.ParameterHash = HashParameters(Out.Parameters);

		Out.HeightGridSide = HeightGridSide();
		Out.LandRadiusUnits = IslandGeography::LandRadiusUnits();

		const int32 Lado = Out.HeightGridSide;
		Out.GroundHeightUnits.Reset(Lado * Lado);
		for (int32 Linha = 0; Linha < Lado; ++Linha)
		{
			for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
			{
				const float X = ((static_cast<float>(Coluna) / (Lado - 1)) * 2.0f - 1.0f)
					* Out.LandRadiusUnits;
				const float Y = ((static_cast<float>(Linha) / (Lado - 1)) * 2.0f - 1.0f)
					* Out.LandRadiusUnits;
				Out.GroundHeightUnits.Add(IslandGeography::GroundHeightAt(FVector2D(X, Y)));
			}
		}

		Out.Rivers.Reset();
		for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
		{
			FBakedRiver Assado;
			Assado.Order = Curso.Order;
			Assado.bFlowsToTheSea = Curso.FlowsToTheSea();
			Assado.bHasLake = Curso.HasLake();
			Assado.bHasFall = Curso.HasFall();
			Assado.LakeCenterUnits =
				FreshWater::PointAtProgress(Curso, Curso.LakeAtProgress);
			Assado.FallCenterUnits =
				FreshWater::PointAtProgress(Curso, Curso.FallAtProgress);
			Assado.PlungePoolHalfWidthUnits = FreshWater::PlungePoolHalfWidthUnits(Curso);
			Assado.PlungePoolDepthUnits = FreshWater::PlungePoolDepthUnits(Curso);

			for (int32 Amostra = 0; Amostra < RiverSampleCount(); ++Amostra)
			{
				const float Onde = ProgressAtSample(Amostra);
				Assado.PointsUnits.Add(FreshWater::PointAtProgress(Curso, Onde));
				Assado.HalfWidthUnits.Add(FreshWater::HalfWidthAtProgress(Curso, Onde));
				Assado.bIsRapids.Add(FreshWater::IsRapidsAtProgress(Curso, Onde));
			}

			Out.Rivers.Add(MoveTemp(Assado));
		}

		Out.Brooks.Reset();
		for (const FreshWater::FBrook& Corrego : FreshWater::PlanBrooks())
		{
			FBakedPolyline Linha;
			Linha.PointsUnits = Corrego.PointsUnits;
			Out.Brooks.Add(MoveTemp(Linha));
		}

		Out.Springs.Reset();
		for (const FreshWater::FSpring& Fonte : FreshWater::PlanSprings())
		{
			FBakedSpring Assada;
			Assada.CenterUnits = Fonte.CenterUnits;
			Assada.PoolHalfWidthUnits = Fonte.PoolHalfWidthUnits;
			Out.Springs.Add(Assada);
		}

		Out.Trails.Reset();
		for (const FTrailRoute& Trilha : TrailLayout::Plan())
		{
			FBakedTrail Assada;
			Assada.PointsUnits = Trilha.PointsUnits;
			Assada.bFellBackToStraightLine = Trilha.bFellBackToStraightLine;
			for (const FVector2D& Ponto : Trilha.PointsUnits)
			{
				Assada.GroundHeightUnits.Add(IslandGeography::GroundHeightAt(Ponto));
			}
			Out.Trails.Add(MoveTemp(Assada));
		}

		Out.Crossings.Reset();
		for (const TrailLayout::FCrossing& Travessia : TrailLayout::Crossings())
		{
			FBakedCrossing Assada;
			Assada.CenterUnits = Travessia.CenterUnits;
			Assada.Kind = static_cast<uint8>(Travessia.Kind);
			Assada.DepthUnits = Travessia.DepthUnits;
			Out.Crossings.Add(Assada);
		}

		Out.GroundUses.Reset();
		for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
		{
			FBakedGroundUse Assada;
			Assada.Use = Mancha.Use;
			Assada.CenterUnits = Mancha.CenterUnits;
			Assada.HalfExtentUnits = Mancha.HalfExtentUnits;
			Assada.bYieldsWater = Mancha.bYieldsWater;
			Assada.Deity = Mancha.Deity;
			Out.GroundUses.Add(Assada);
		}

		Out.Caves.Reset();
		const int32 Semente = WorldScenerySeed();
		for (const IslandFeatureLayout::FFeaturePlacement& Caverna : AllCaves())
		{
			const int32 Lados = Caverna.CaveSide;
			const int32 Outro = (Caverna.CaveOtherSide > 0) ? Caverna.CaveOtherSide : Lados;
			const CaveLabyrinth::FCaveGrid Planta = CaveLabyrinth::Carve(Lados, Outro,
				IslandFeatureLayout::SeedForPlacement(Semente, Caverna));

			FBakedCave Assada;
			Assada.CenterUnits = Caverna.CenterUnits();
			Assada.Columns = Planta.Columns;
			Assada.Rows = Planta.Rows;
			Assada.EntranceColumn = Planta.EntranceColumn;
			Assada.Walls = Planta.Walls;
			Out.Caves.Add(MoveTemp(Assada));
		}

		Out.UnderwaterLinks.Reset();
		for (const FreshWater::FUnderwaterLink& Galeria : FreshWater::PlanUnderwaterLinks())
		{
			FBakedPolyline Linha;
			Linha.PointsUnits = Galeria.PointsUnits;
			Out.UnderwaterLinks.Add(MoveTemp(Linha));
		}

		Out.Aqueducts.Reset();
		for (const AqueductLayout::FAqueduct& Aqueduto : AqueductLayout::Plan())
		{
			FBakedPolyline Linha;
			Linha.PointsUnits = Aqueduto.PointsUnits;
			Out.Aqueducts.Add(MoveTemp(Linha));
		}
	}

	UIslandBakedPlan* Load()
	{
		return LoadObject<UIslandBakedPlan>(nullptr, AssetPath());
	}
}
