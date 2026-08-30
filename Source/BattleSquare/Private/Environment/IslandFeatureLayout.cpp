// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandFeatureLayout.h"

#include "Environment/CaveSystem.h"
#include "Environment/WalkableMountain.h"

namespace IslandFeatureLayout
{
	namespace
	{
		/**
		 * O anel em que quase tudo mora.
		 *
		 * Longe o bastante para o raio da peça não alcançar os campos de treino
		 * em ângulo nenhum, e perto o bastante para a peça inteira ainda cair
		 * na terra. Entre esses dois limites não sobra faixa larga: a ilha tem
		 * sessenta metros de raio e o miolo já está ocupado.
		 */
		constexpr float AnelDaBorda = 4400.0f;

		/**
		 * A caverna grande vem mais para dentro, e encaixada num vão.
		 *
		 * Ela é a única peça larga demais para ter folga radial: no anel da
		 * borda a quina dela sairia na água. Puxá-la para dentro resolve a
		 * água e cria o outro problema — passa a raspar no anel dos campos —,
		 * e é por isso que o ângulo dela não é livre: 36° é o meio do vão
		 * entre os campos de 0° e 72°.
		 */
		constexpr float AnelDaCavernaGrande = 3900.0f;
		constexpr float AnguloDaCavernaGrande = 36.0f;

		FFeaturePlacement Montanha(float AnguloEmGraus)
		{
			FFeaturePlacement Peca;
			Peca.Feature = EIslandFeature::WalkableMountain;
			Peca.AngleDegrees = AnguloEmGraus;
			Peca.RadiusUnits = AnelDaBorda;
			Peca.ClearanceUnits = MountainClearanceUnits();
			return Peca;
		}

		FFeaturePlacement Caverna(float AnguloEmGraus, float RaioDoAnel, int32 Lado)
		{
			FFeaturePlacement Peca;
			Peca.Feature = EIslandFeature::Cave;
			Peca.AngleDegrees = AnguloEmGraus;
			Peca.RadiusUnits = RaioDoAnel;
			Peca.ClearanceUnits = CaveClearanceUnits(Lado);
			Peca.CaveSide = Lado;
			return Peca;
		}
	}

	FVector2D FFeaturePlacement::CenterUnits() const
	{
		const float Radianos = FMath::DegreesToRadians(AngleDegrees);
		return FVector2D(FMath::Cos(Radianos) * RadiusUnits, FMath::Sin(Radianos) * RadiusUnits);
	}

	float MountainClearanceUnits()
	{
		return AWalkableMountain::DefaultBaseRadiusUnits;
	}

	float CaveClearanceUnits(int32 Side)
	{
		// Meia-diagonal, e não meio-lado: o quadrado da caverna não aponta para
		// o vizinho, e o meio-lado deixaria a quina passar por dentro dele.
		return 0.5f * ACaveSystem::FootprintForSide(Side) * UE_SQRT_2;
	}

	TArray<FFeaturePlacement> Plan()
	{
		// Três montanhas bem separadas, e as cavernas nos vãos entre elas: quem
		// sobe uma montanha vê a boca da seguinte, e é isso que faz o anel ser
		// um caminho em vez de três lugares soltos.
		TArray<FFeaturePlacement> Pecas;
		Pecas.Add(Caverna(AnguloDaCavernaGrande, AnelDaCavernaGrande, ACaveSystem::LargeCaveSide));
		Pecas.Add(Montanha(90.0f));
		Pecas.Add(Caverna(150.0f, AnelDaBorda, ACaveSystem::MediumCaveSide));
		Pecas.Add(Montanha(210.0f));
		Pecas.Add(Caverna(270.0f, AnelDaBorda, ACaveSystem::SmallCaveSide));
		Pecas.Add(Montanha(330.0f));
		return Pecas;
	}

	bool FitsOnLand(const FFeaturePlacement& Placement, const FIslandBounds& Bounds)
	{
		return Placement.RadiusUnits + Placement.ClearanceUnits <= Bounds.LandRadiusUnits;
	}

	bool ClearsTrainingFields(const FFeaturePlacement& Placement, const FIslandBounds& Bounds)
	{
		if (Bounds.TrainingFieldCount <= 0)
		{
			return true;
		}

		// Os campos são cinco DISCOS, não um anel cheio. Medir contra o anel
		// inteiro reprovaria a caverna grande, que existe justamente porque
		// entre dois campos há vão de sobra.
		const float Exigido = Placement.ClearanceUnits + Bounds.TrainingFieldRadiusUnits;
		const FVector2D Centro = Placement.CenterUnits();

		for (int32 Indice = 0; Indice < Bounds.TrainingFieldCount; ++Indice)
		{
			const float Angulo = (2.0f * PI * static_cast<float>(Indice))
				/ static_cast<float>(Bounds.TrainingFieldCount);
			const FVector2D Campo(
				FMath::Cos(Angulo) * Bounds.TrainingRingRadiusUnits,
				FMath::Sin(Angulo) * Bounds.TrainingRingRadiusUnits);

			if (FVector2D::Distance(Centro, Campo) < Exigido)
			{
				return false;
			}
		}

		return true;
	}

	bool Overlaps(const FFeaturePlacement& First, const FFeaturePlacement& Second)
	{
		const float Exigido = First.ClearanceUnits + Second.ClearanceUnits;
		return FVector2D::Distance(First.CenterUnits(), Second.CenterUnits()) < Exigido;
	}
}
