// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/FreshWater.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"

namespace FreshWater
{
	namespace
	{
		/**
		 * A calha do rio comum.
		 *
		 * Larga o bastante para ser um rio visto de cima, e estreita o bastante
		 * para ainda se atravessar a pé: um rio que corta a ilha em duas sem
		 * ponte não é paisagem, é muro.
		 */
		constexpr float MeiaCalhaDoRio = 170.0f;

		/** O lago no auge: sete vezes o rio, o que já é lago e não poça. */
		constexpr float MeiaCalhaDoLago = 1250.0f;

		/**
		 * Sobre quantas unidades de raio o rio engorda até virar lago.
		 *
		 * A transição é longa de propósito. Alargamento curto lê como um
		 * quadrado grudado no rio; longo lê como a água parando.
		 */
		constexpr float AlcanceDoLago = 1700.0f;

		/** Metade do comprimento da queda, medido no raio. */
		constexpr float MeiaQueda = 240.0f;

		/** Da margem até o meio da trilha. */
		constexpr float AfastamentoDaTrilha = 260.0f;

		/** Metade da largura da trilha de beira-rio. */
		constexpr float MeiaTrilha = 95.0f;

		/**
		 * Onde a nascente fica, medida em saias de monte.
		 *
		 * No cume não pode: o monte é sólido e a água sairia de dentro da
		 * pedra. Na base também não: o rio começaria já na planície e ninguém
		 * ligaria uma coisa à outra. Meio caminho na encosta é o único ponto em
		 * que se vê o monte atrás da nascente.
		 */
		constexpr float SaiaDaNascente = 0.6f;

		/** Quanto o rumo abre para cada lado, no mínimo e no máximo. */
		constexpr float MenorSerpente = 0.045f;
		constexpr float MaiorSerpente = 0.085f;

		/** Quantas curvas completas da nascente à foz. */
		constexpr float MenosCurvas = 1.15f;
		constexpr float MaisCurvas = 1.85f;

		/** Onde o lago alaga, em fração do percurso. */
		constexpr float PrimeiroLago = 0.32f;
		constexpr float UltimoLago = 0.46f;

		/** Quanto depois do fim do lago a água despenca. */
		constexpr float MenorDegrau = 300.0f;
		constexpr float MaiorDegrau = 1100.0f;
	}

	float RiverHalfWidthUnits() { return MeiaCalhaDoRio; }
	float LakeHalfWidthUnits() { return MeiaCalhaDoLago; }
	float FallHalfLengthUnits() { return MeiaQueda; }
	float TrailOffsetUnits() { return AfastamentoDaTrilha; }
	float TrailHalfWidthUnits() { return MeiaTrilha; }

	TArray<FRiverCourse> Plan()
	{
		TArray<FRiverCourse> Cursos;
		const float Foz = IslandGeography::LandRadiusUnits();

		int32 Indice = 0;
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature != IslandFeatureLayout::EIslandFeature::WalkableMountain)
			{
				continue;
			}

			FRiverCourse Curso;
			Curso.SourceRadiusUnits = Peca.RadiusUnits + Peca.ClearanceUnits * SaiaDaNascente;
			Curso.MouthRadiusUnits = Foz;
			Curso.BearingRadians = FMath::DegreesToRadians(Peca.AngleDegrees);

			// A semente sai do ÍNDICE, não do ângulo: ângulo é float, e float
			// virando semente é a receita de o rio mudar de curva porque alguém
			// mexeu na terceira casa decimal do anel dos montes.
			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("rio-da-montanha-%d"), Indice));

			Curso.MeanderRadians = BattleSpread::Between(MenorSerpente, MaiorSerpente,
				BattleSpread::Fraction(Semente, 0));
			Curso.MeanderTurns = BattleSpread::Between(MenosCurvas, MaisCurvas,
				BattleSpread::Fraction(Semente, 1));

			const float Percurso = FMath::Max(0.0f, Curso.MouthRadiusUnits - Curso.SourceRadiusUnits);
			Curso.LakeRadiusUnits = Curso.SourceRadiusUnits + Percurso * BattleSpread::Between(
				PrimeiroLago, UltimoLago, BattleSpread::Fraction(Semente, 2));

			// A queda vem SEMPRE depois do lago, e por soma em vez de sorteio
			// independente: sorteados à parte, um dia sairia uma cachoeira no
			// meio do lago, que é água caindo dentro de água parada.
			Curso.FallRadiusUnits = Curso.LakeRadiusUnits + AlcanceDoLago + BattleSpread::Between(
				MenorDegrau, MaiorDegrau, BattleSpread::Fraction(Semente, 3));

			Cursos.Add(Curso);
			++Indice;
		}

		return Cursos;
	}

	FVector2D PointAt(const FRiverCourse& Course, float RadiusUnits)
	{
		const float Percurso = Course.MouthRadiusUnits - Course.SourceRadiusUnits;
		const float Andado = (Percurso > KINDA_SMALL_NUMBER)
			? (RadiusUnits - Course.SourceRadiusUnits) / Percurso
			: 0.0f;

		const float Rumo = Course.BearingRadians + Course.MeanderRadians
			* FMath::Sin(2.0f * PI * Course.MeanderTurns * Andado);

		return FVector2D(FMath::Cos(Rumo) * RadiusUnits, FMath::Sin(Rumo) * RadiusUnits);
	}

	float HalfWidthAt(const FRiverCourse& Course, float RadiusUnits)
	{
		const float Distancia = FMath::Abs(RadiusUnits - Course.LakeRadiusUnits);
		if (Distancia >= AlcanceDoLago)
		{
			return MeiaCalhaDoRio;
		}

		const float Subida = 1.0f - Distancia / AlcanceDoLago;
		const float Suave = Subida * Subida * (3.0f - 2.0f * Subida);
		return MeiaCalhaDoRio + (MeiaCalhaDoLago - MeiaCalhaDoRio) * Suave;
	}

	bool IsFallAt(const FRiverCourse& Course, float RadiusUnits)
	{
		return FMath::Abs(RadiusUnits - Course.FallRadiusUnits) <= MeiaQueda;
	}
}
