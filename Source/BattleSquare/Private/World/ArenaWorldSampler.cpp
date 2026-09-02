// Copyright 2026 Anderson. All Rights Reserved.

#include "World/ArenaWorldSampler.h"

#include "World/IslandBakedPlan.h"
#include "World/WaterFooting.h"

#include "Battle/BattleArena.h"
#include "Environment/ForestBackdrop.h"
#include "Environment/WorldBoundaryWater.h"
#include "World/WorldObstacleBreaking.h"
#include "World/WorldTrainingField.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"

namespace
{
	/**
	 * Quantas casas de raio a coleta varre em volta do encontro.
	 *
	 * Generoso de propósito: a grade pode ser 4x6, e varrer só o suficiente
	 * para 3x3 deixaria as bordas de um campo grande sempre vazias — o
	 * tabuleiro pareceria uma clareira redonda no meio da mata, sempre.
	 */
	constexpr int32 RaioEmCasas = 4;
}

void FArenaWorldSampler::Collect(const UWorld* World, const FVector& Around,
	TArray<FWorldFeatureSample>& OutFeatures)
{
	OutFeatures.Reset();
	if (!World)
	{
		return;
	}

	const float TamanhoDaCasa = GetDefault<ABattleArena>()->CellSize;
	const float Alcance = TamanhoDaCasa * RaioEmCasas;
	const float AlcanceAoQuadrado = Alcance * Alcance;

	// SÓ O QUE TEM CORPO vira casa bloqueada, e quem decide isso é
	// StartingHealthFor — a MESMA função que decide o que dá para derrubar.
	// Uma segunda lista de "o que é sólido" concordaria com ela até a
	// primeira edição, e o sintoma seria uma casa bloqueada por uma flor.
	for (TActorIterator<AForestBackdrop> It(World); It; ++It)
	{
		const AForestBackdrop* Mata = *It;
		const TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Grupos =
			Mata->GetSpeciesClusters();

		for (int32 Grupo = 0; Grupo < Grupos.Num(); ++Grupo)
		{
			const UHierarchicalInstancedStaticMeshComponent* Agrupamento = Grupos[Grupo];
			if (!Agrupamento || !Mata->IsSolidSpecies(Grupo))
			{
				continue;
			}

			for (int32 Instancia = 0; Instancia < Agrupamento->GetInstanceCount(); ++Instancia)
			{
				FTransform Onde;
				if (!Agrupamento->GetInstanceTransform(Instancia, Onde, /*bWorldSpace=*/true))
				{
					continue;
				}

				const FVector Posicao = Onde.GetLocation();
				if (FVector::DistSquared2D(Posicao, Around) <= AlcanceAoQuadrado)
				{
					OutFeatures.Add({ Posicao, EWorldFeatureKind::Solid });
				}
			}
		}
	}

	for (TActorIterator<AWorldTrainingField> It(World); It; ++It)
	{
		const FVector Posicao = It->GetActorLocation();
		if (FVector::DistSquared2D(Posicao, Around) <= AlcanceAoQuadrado)
		{
			OutFeatures.Add({ Posicao, EWorldFeatureKind::TrainingGround });
		}
	}

	// A água é ANEL, não objeto: perguntar "onde está o ator da água" daria um
	// ponto no centro do mundo, que é justamente onde não há água nenhuma. Por
	// isso ela é amostrada por PONTO — para cada casa em volta, esta casa está
	// molhada? É a mesma pergunta que o jogador faz com o pé.
	for (TActorIterator<AWorldBoundaryWater> It(World); It; ++It)
	{
		const AWorldBoundaryWater* Agua = *It;
		const FVector Centro = Agua->GetActorLocation();

		for (int32 DeltaColuna = -RaioEmCasas; DeltaColuna <= RaioEmCasas; ++DeltaColuna)
		{
			for (int32 DeltaLinha = -RaioEmCasas; DeltaLinha <= RaioEmCasas; ++DeltaLinha)
			{
				const FVector Ponto(
					Around.X + DeltaColuna * TamanhoDaCasa,
					Around.Y + DeltaLinha * TamanhoDaCasa,
					Around.Z);

				const float Distancia = FVector::Dist2D(Ponto, Centro);
				if (Distancia > Agua->WaterRadiusUnits)
				{
					continue;
				}

				// A MARGEM é uma faixa de uma casa: sem ela, a borda da água
				// seria um corte seco entre chão e rio, e a poça — que é o
				// terreno mais interessante da beira — nunca apareceria.
				if (Distancia >= Agua->ShoreRadiusUnits)
				{
					// DE QUE FLUIDO é esta água, perguntado ao assado — que é
					// quem sabe, e ponto a ponto: a água na saia do vulcão é
					// termal e a mesma água rio abaixo é doce.
					//
					// Aqui, e não na metade pura: colher exige o mundo, e é
					// esta a metade que o tem.
					uint8 Fluido = static_cast<uint8>(EFluidKind::AguaDoce);
					if (const UIslandBakedPlan* Assado = IslandBakedPlan::LoadedOrNull())
					{
						Fluido = static_cast<uint8>(WaterFooting::FluidAt(
							*Assado, FVector2D(Ponto)));
					}

					OutFeatures.Add({ Ponto,
						Distancia < Agua->ShoreRadiusUnits + TamanhoDaCasa
							? EWorldFeatureKind::Shore
							: EWorldFeatureKind::DeepWater,
						Fluido });
				}
			}
		}
	}
}
