#include "World/IslandBakedPlan.h"

#include "Debug/BattleDebugScreen.h"
#include "Environment/CaveLabyrinth.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"
#include "Net/BattleSquareGameMode.h"
#include "World/AqueductLayout.h"
#include "World/TrailLayout.h"
#include "World/WorldBudget.h"

namespace FaixasDoTerreno
{
	/**
	 * A partir de que altura o terreno é CUME.
	 *
	 * Como fração, e não em unidades: número absoluto escolhido quando só
	 * existia um tamanho de ilha é a armadilha mais cara deste projeto — ela
	 * apareceu medida em sete lugares.
	 *
	 * A fração é do CHÃO MAIS ALTO QUE NÃO ESTÁ QUEIMADO, e não do mais alto de
	 * todos. Medido: o ponto mais alto da ilha é o vulcão (17.814), e as 36
	 * casas acima de 60% dele estavam TODAS dentro da mancha queimada — que é
	 * avaliada antes. A faixa de cume era inalcançável por construção: código
	 * declarava uma cor que o jogador nunca veria.
	 *
	 * Medir pelo chão pisável (8.127 fora da mancha) também dá ao cume o
	 * significado útil: o alto do relevo em que se pode estar. O topo do vulcão
	 * é rocha queimada, e isso é o certo — vulcão não é cume, é vulcão.
	 */
	constexpr float FracaoDoCume = 0.7f;
}

float UIslandBakedPlan::HeightAt(const FVector2D& Onde) const
{
	if (HeightGridSide < 2 || LandRadiusUnits <= 0.0f)
	{
		return 0.0f;
	}

	const float Casa = (2.0f * LandRadiusUnits) / (HeightGridSide - 1);
	const float EmCasas_X = (Onde.X + LandRadiusUnits) / Casa;
	const float EmCasas_Y = (Onde.Y + LandRadiusUnits) / Casa;

	// Presa à grade: fora dela não há chão, e extrapolar inventaria terra no
	// mar aberto — que é como uma vila já nasceu fora da ilha neste projeto.
	const int32 Coluna = FMath::Clamp(FMath::FloorToInt(EmCasas_X), 0, HeightGridSide - 2);
	const int32 Linha = FMath::Clamp(FMath::FloorToInt(EmCasas_Y), 0, HeightGridSide - 2);

	const float FracaoX = FMath::Clamp(EmCasas_X - Coluna, 0.0f, 1.0f);
	const float FracaoY = FMath::Clamp(EmCasas_Y - Linha, 0.0f, 1.0f);

	const float Baixo = FMath::Lerp(
		HeightAtCell(Coluna, Linha), HeightAtCell(Coluna + 1, Linha), FracaoX);
	const float Cima = FMath::Lerp(
		HeightAtCell(Coluna, Linha + 1), HeightAtCell(Coluna + 1, Linha + 1), FracaoX);

	return FMath::Lerp(Baixo, Cima, FracaoY);
}

float UIslandBakedPlan::CoastRadiusAt(float RumoRadianos) const
{
	if (CoastRadiusByDegree.Num() == 0)
	{
		return LandRadiusUnits;
	}

	const int32 Graus = CoastRadiusByDegree.Num();
	const float EmGraus = FMath::Fmod(
		FMath::RadiansToDegrees(RumoRadianos) + 360.0f, 360.0f);

	const int32 Antes = FMath::Clamp(FMath::FloorToInt(EmGraus), 0, Graus - 1);
	const int32 Depois = (Antes + 1) % Graus;

	// Interpola entre os graus vizinhos: saltar de grau em grau poria degraus
	// de costa onde a linha é lisa, e a faixa de praia herdaria os degraus.
	return FMath::Lerp(CoastRadiusByDegree[Antes], CoastRadiusByDegree[Depois],
		EmGraus - Antes);
}

const TCHAR* UIslandBakedPlan::BandDebugName(ETerrainBand Faixa)
{
	switch (Faixa)
	{
	case ETerrainBand::Praia:         return TEXT("praia");
	case ETerrainBand::Mata:          return TEXT("mata");
	case ETerrainBand::Barranco:      return TEXT("barranco");
	case ETerrainBand::RochaQueimada: return TEXT("rocha queimada");
	case ETerrainBand::Cume:          return TEXT("cume");
	default: break;
	}
	return TEXT("?");
}

int32 UIslandBakedPlan::SacredAt(const FVector2D& Onde) const
{
	// O alcance sai da meia-extensão da PRÓPRIA mancha, com folga.
	constexpr float FolgaDoAlcance = 3.0f;

	int32 MaisPerto = INDEX_NONE;
	float MenorDistancia = TNumericLimits<float>::Max();

	for (int32 Qual = 0; Qual < GroundUses.Num(); ++Qual)
	{
		const FBakedGroundUse& Mancha = GroundUses[Qual];
		if (Mancha.Use != EGroundUse::Templo && Mancha.Use != EGroundUse::Ruina)
		{
			continue;
		}

		const float Distancia = FVector2D::Distance(Mancha.CenterUnits, Onde);
		if (Distancia > Mancha.HalfExtentUnits * FolgaDoAlcance)
		{
			continue;
		}

		if (Distancia < MenorDistancia)
		{
			MenorDistancia = Distancia;
			MaisPerto = Qual;
		}
	}

	return MaisPerto;
}

float UIslandBakedPlan::HighestWalkableHeightUnits() const
{
	// Calculado UMA vez e guardado: `BandAt` é perguntado por casa da grade ao
	// construir o relevo, e varrer 32.400 alturas a cada pergunta seria varrer
	// a ilha inteira trinta e duas mil vezes.
	if (CachedHighestWalkable >= 0.0f)
	{
		return CachedHighestWalkable;
	}

	const int32 Lado = HeightGridSide;
	float MaisAlto = 0.0f;

	for (int32 Linha = 0; Linha < Lado; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
		{
			const float X = ((static_cast<float>(Coluna) / (Lado - 1)) * 2.0f - 1.0f)
				* LandRadiusUnits;
			const float Y = ((static_cast<float>(Linha) / (Lado - 1)) * 2.0f - 1.0f)
				* LandRadiusUnits;

			// Fora da mancha queimada: é o chão em que se pode estar.
			if (FVector2D::Distance(FVector2D(X, Y), Parameters.VolcanoCenterUnits)
				<= Parameters.VolcanoScorchedRadiusUnits)
			{
				continue;
			}

			MaisAlto = FMath::Max(MaisAlto, HeightAtCell(Coluna, Linha));
		}
	}

	CachedHighestWalkable = MaisAlto;
	return MaisAlto;
}

ETerrainBand UIslandBakedPlan::BandAt(const FVector2D& Onde) const
{
	// A ROCHA QUEIMADA vem primeiro: o vulcão queima o que já estava ali,
	// seja mata, barranco ou praia.
	if (Parameters.VolcanoScorchedRadiusUnits > 0.0f
		&& FVector2D::Distance(Onde, Parameters.VolcanoCenterUnits)
			<= Parameters.VolcanoScorchedRadiusUnits)
	{
		return ETerrainBand::RochaQueimada;
	}

	const int32 Lado = HeightGridSide;
	if (Lado >= 2 && LandRadiusUnits > 0.0f)
	{
		const float Casa = (2.0f * LandRadiusUnits) / (Lado - 1);
		const int32 Coluna = FMath::Clamp(
			FMath::RoundToInt((Onde.X + LandRadiusUnits) / Casa), 0, Lado - 1);
		const int32 Linha = FMath::Clamp(
			FMath::RoundToInt((Onde.Y + LandRadiusUnits) / Casa), 0, Lado - 1);

		const float MaisAlto = HighestWalkableHeightUnits();

		// O CUME antes do barranco: o alto de uma escarpa ainda é alto.
		if (MaisAlto > 0.0f
			&& HeightAtCell(Coluna, Linha)
				>= MaisAlto * FaixasDoTerreno::FracaoDoCume)
		{
			return ETerrainBand::Cume;
		}
	}

	const float Raio = Onde.Size();

	if (Raio >= Parameters.BluffInnerRadiusUnits
		&& Raio <= Parameters.BluffOuterRadiusUnits
		&& Parameters.BluffOuterRadiusUnits > 0.0f)
	{
		return ETerrainBand::Barranco;
	}

	// A PRAIA mede da COSTA para dentro, nunca do centro para fora: a ilha
	// deixou de ser um círculo, e uma faixa a distância fixa do centro cairia
	// no mar numa reentrância.
	const float NaCosta = CoastRadiusAt(FMath::Atan2(Onde.Y, Onde.X));
	if (Raio >= NaCosta - Parameters.BeachWidthUnits)
	{
		return ETerrainBand::Praia;
	}

	return ETerrainBand::Mata;
}

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

	/**
	 * DE QUE FLUIDO é a água num ponto da ilha.
	 *
	 * A tabela do mundo, num lugar só — assada uma vez, para o jogo não
	 * perguntar ao gerador o que o assado já deveria saber.
	 *
	 * A ordem é decisão: TERMAL vence, porque a água quente do vulcão continua
	 * quente mesmo correndo por dentro do pântano; e o pântano vence a doce,
	 * porque é ele que carrega o barro.
	 */
	/**
	 * O rumo do fluxo entre dois pontos do curso, como `EBattleDirection`.
	 *
	 * Usa `GetDirectionTowards`, que é o inverso já existente da tabela de
	 * direções — o comentário dele diz por quê: "uma segunda cópia da relação
	 * direção<->deslocamento já produziu um defeito neste projeto".
	 *
	 * O EIXO DA LINHA É INVERTIDO em relação ao Y do mundo: na grade, `Cima` é
	 * linha MENOS UM. Ignorar isso faria a água correr para o lado errado, que
	 * é exatamente o defeito de "Baixo andava para a direita" deste projeto.
	 */
	uint8 RumoDoFluxo(const FVector2D& De, const FVector2D& Para)
	{
		const FVector2D Passo = Para - De;
		if (Passo.IsNearlyZero())
		{
			return static_cast<uint8>(EBattleDirection::Nenhuma);
		}

		// Só o SINAL importa, e é o que `GetDirectionTowards` consome.
		const int32 EmColuna = FMath::IsNearlyZero(Passo.X, 1.0f)
			? 0 : (Passo.X > 0.0f ? 1 : -1);

		// Y do mundo cresce para cima; a LINHA da grade cresce para baixo.
		const int32 EmLinha = FMath::IsNearlyZero(Passo.Y, 1.0f)
			? 0 : (Passo.Y > 0.0f ? -1 : 1);

		return static_cast<uint8>(GetDirectionTowards(EmColuna, EmLinha));
	}

	/** A força do fluxo em partes por mil, presa no que um byte guarda. */
	uint8 ForcaDoFluxo(float DecliveDoLeito)
	{
		return static_cast<uint8>(
			FMath::Clamp(FMath::RoundToInt(DecliveDoLeito * 1000.0f), 0, 255));
	}

	EFluidKind FluidoDaAguaEm(const FVector2D& Onde)
	{
		if (FreshWater::IsThermalAt(Onde))
		{
			return EFluidKind::AguaTermal;
		}

		if (IslandGeography::BiomeAt(Onde) == EIslandBiome::Swamp)
		{
			return EFluidKind::AguaDePantano;
		}

		return EFluidKind::AguaDoce;
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
		Parametros.BeachWidthUnits = IslandGeography::BeachWidthUnits();
		Parametros.BluffInnerRadiusUnits = IslandGeography::BluffInnerRadiusUnits();
		Parametros.BluffOuterRadiusUnits = IslandGeography::BluffOuterRadiusUnits();
		Parametros.VolcanoCenterUnits = IslandGeography::VolcanoCenterUnits();
		Parametros.VolcanoScorchedRadiusUnits =
			IslandGeography::VolcanoScorchedRadiusUnits();
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

		Out.CoastRadiusByDegree.Reset(360);
		for (int32 Grau = 0; Grau < 360; ++Grau)
		{
			Out.CoastRadiusByDegree.Add(IslandGeography::LandRadiusAt(
				FMath::DegreesToRadians(static_cast<float>(Grau))));
		}

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
				const FVector2D NoPonto = FreshWater::PointAtProgress(Curso, Onde);
				Assado.PointsUnits.Add(NoPonto);
				Assado.HalfWidthUnits.Add(FreshWater::HalfWidthAtProgress(Curso, Onde));

				// A FUNDURA no mesmo laço da largura, e por ponto: duas
				// varreduras do mesmo curso poderiam divergir na amostragem, e
				// aí largura e fundura falariam de pontos diferentes.
				Assado.DepthUnits.Add(FreshWater::DepthAtProgress(Curso, Onde));
				Assado.bIsRapids.Add(FreshWater::IsRapidsAtProgress(Curso, Onde));
				Assado.FluidByPoint.Add(static_cast<uint8>(FluidoDaAguaEm(NoPonto)));

				// O RUMO olha para o PRÓXIMO ponto: o fluxo vai daqui para lá.
				// No último não há próximo, e ele herda o rumo do trecho
				// anterior — a foz corre para onde o rio vinha correndo, e
				// zerar ali faria a última casa de todo rio virar água parada.
				const float Adiante = ProgressAtSample(
					FMath::Min(Amostra + 1, RiverSampleCount() - 1));
				const FVector2D NoSeguinte = FreshWater::PointAtProgress(Curso, Adiante);

				Assado.FlowDirectionByPoint.Add(Amostra + 1 < RiverSampleCount()
					? RumoDoFluxo(NoPonto, NoSeguinte)
					: (Assado.FlowDirectionByPoint.Num() > 0
						? Assado.FlowDirectionByPoint.Last()
						: static_cast<uint8>(EBattleDirection::Nenhuma)));

				Assado.FlowStrengthByPoint.Add(
					ForcaDoFluxo(FreshWater::BedGradientAtProgress(Curso, Onde)));
			}

			Out.Rivers.Add(MoveTemp(Assado));
		}

		Out.Brooks.Reset();
		for (const FreshWater::FBrook& Corrego : FreshWater::PlanBrooks())
		{
			FBakedBrook Assado;
			Assado.PointsUnits = Corrego.PointsUnits;
			Assado.HalfWidthUnits = Corrego.HalfWidthUnits;
			Assado.Fluid = static_cast<uint8>(FluidoDaAguaEm(
				Corrego.PointsUnits.Num() > 0
					? Corrego.PointsUnits[Corrego.PointsUnits.Num() / 2]
					: FVector2D::ZeroVector));
			Out.Brooks.Add(MoveTemp(Assado));
		}

		Out.Springs.Reset();
		for (const FreshWater::FSpring& Fonte : FreshWater::PlanSprings())
		{
			FBakedSpring Assada;
			Assada.CenterUnits = Fonte.CenterUnits;
			Assada.PoolHalfWidthUnits = Fonte.PoolHalfWidthUnits;
			Assada.Fluid = static_cast<uint8>(FluidoDaAguaEm(Fonte.CenterUnits));
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
			FBakedAqueduct Assado;
			Assado.PointsUnits = Aqueduto.PointsUnits;
			Assado.DropUnits = Aqueduto.DropUnits;
			Out.Aqueducts.Add(MoveTemp(Assado));
		}
	}

	UIslandBakedPlan* Load()
	{
		return LoadObject<UIslandBakedPlan>(nullptr, AssetPath());
	}

	UIslandBakedPlan* LoadedOrNull()
	{
		// `FindObject`, e nunca `LoadObject`: procurar não toca em disco.
		return FindObject<UIslandBakedPlan>(nullptr, AssetPath());
	}

	UIslandBakedPlan* LoadForWorld()
	{
		UIslandBakedPlan* Assado = Load();
		if (!Assado)
		{
			const FString Recado = FString::Printf(
				TEXT("MUNDO: o assado nao existe em %s — rode ./Tools/bake_island.sh"),
				AssetPath());
			UE_LOG(LogTemp, Error, TEXT("%s"), *Recado);
			FBattleDebugScreen::Show(Recado, 30.0f, FColor::Red, 770);
			return nullptr;
		}

		const FIslandParameters Agora = GatherParameters();
		if (Assado->ParameterHash == HashParameters(Agora))
		{
			return Assado;
		}

		// O resumo diz que divergiu; os nomes dizem no quê. Sem os nomes, a
		// pessoa recebe "reasse" e vai procurar sozinha o que já dava para ler.
		const TArray<FString> Divergiram =
			DescribeParameterDivergence(Assado->Parameters, Agora);

		const FString Recado = FString::Printf(
			TEXT("MUNDO: o assado e de outra configuracao — %s. Rode ./Tools/bake_island.sh"),
			Divergiram.Num() > 0
				? *FString::Join(Divergiram, TEXT("; "))
				: TEXT("o resumo nao bate, mas nenhum parametro difere (formato mudou?)"));

		UE_LOG(LogTemp, Error, TEXT("%s"), *Recado);
		FBattleDebugScreen::Show(Recado, 30.0f, FColor::Red, 770);

		return nullptr;
	}
}
