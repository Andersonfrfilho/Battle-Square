// Copyright 2026 Anderson. All Rights Reserved.

#include "World/AqueductLayout.h"
#include "World/VillageLayout.h"
#include "World/PlanReentryGuard.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandGeography.h"

namespace
{
	/** Quantos pontos descrevem a calha. */
	constexpr int32 TrechosDoAqueduto = 24;

	/** A calha é estreita: ela transporta, não alaga. */
	constexpr float FracaoDaCalha = 0.0016f;

	/**
	 * A captação precisa estar mais alta que a cidade — e por uma margem.
	 *
	 * Empatar não serve: água só corre com desnível, e um aqueduto com queda de
	 * meio metro em quinhentos é um cano cheio de água parada.
	 */
	constexpr float DesnivelMinimo = 220.0f;
}

float AqueductLayout::ThirstyBeyondUnits(ESettlementKind Kind)
{
	// Duas clareiras: é o mesmo limite que o teste da regra usa, e ele quer
	// dizer "dá para ir buscar água a pé todo dia".
	return VillageLayout::ClearingHalfExtentUnitsFor(Kind) * 2.0f;
}

float AqueductLayout::HalfWidthUnits()
{
	return IslandGeography::LandRadiusUnits() * FracaoDaCalha;
}

const TArray<AqueductLayout::FAqueduct>& AqueductLayout::Plan()
{
	static TArray<FAqueduct> Guardado = []()
	{
		const FPlanReentryGuard Guarda(TEXT("AqueductLayout::Plan"));

		TArray<FAqueduct> Aquedutos;

		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			// O cais não bebe: ele é uma porta na praia, não um lugar de morar.
			if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
			{
				continue;
			}

			const float NaVila =
				IslandGeography::GroundHeightAt(Assentamento.CenterUnits);

			float MaisPerto = TNumericLimits<float>::Max();
			FVector2D DaCaptacao = FVector2D::ZeroVector;
			bool bAchouPerto = false;

			// Procura a água mais próxima QUE AINDA ESTÁ ACIMA. A mais próxima
			// de todas não serve se estiver embaixo: aqueduto desce.
			for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
			{
				for (int32 Passo = 0; Passo <= 20; ++Passo)
				{
					const FVector2D Onde =
						FreshWater::PointAtProgress(Curso, static_cast<float>(Passo) / 20.0f);

					const float Ate = static_cast<float>(
						FVector2D::Distance(Onde, Assentamento.CenterUnits));

					if (Ate < ThirstyBeyondUnits(Assentamento.Kind))
					{
						bAchouPerto = true;
					}

					if (Ate >= MaisPerto
						|| IslandGeography::GroundHeightAt(Onde) < NaVila + DesnivelMinimo)
					{
						continue;
					}

					MaisPerto = Ate;
					DaCaptacao = Onde;
				}
			}

			// Vila com água perto não tem aqueduto, e não precisa. Obra que não
			// resolve nada é obra que ninguém explica.
			if (bAchouPerto || MaisPerto == TNumericLimits<float>::Max())
			{
				continue;
			}

			FAqueduct Aqueduto;
			Aqueduto.Serves = Assentamento.Kind;
			Aqueduto.DropUnits = IslandGeography::GroundHeightAt(DaCaptacao) - NaVila;

			// Reta da captação à vila, com os pontos amostrados no terreno.
			//
			// Reta de propósito, e é a única coisa deste mundo que é: aqueduto
			// é OBRA, e obra de água escolhe o caminho curto porque cada metro
			// a mais é altura perdida. É o contraste com a trilha que faz as
			// duas se lerem — uma serpenteia porque segue o chão, a outra corta
			// porque vence o chão.
			for (int32 Passo = 0; Passo <= TrechosDoAqueduto; ++Passo)
			{
				Aqueduto.PointsUnits.Add(FMath::Lerp(DaCaptacao, Assentamento.CenterUnits,
					static_cast<float>(Passo) / TrechosDoAqueduto));
			}

			Aquedutos.Add(MoveTemp(Aqueduto));
		}

		return Aquedutos;
	}();

	return Guardado;
}
