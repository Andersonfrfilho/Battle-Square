// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/FreshWater.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/CaveSystem.h"
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

		/**
		 * A primeira distância tentada entre a queda e o centro da gruta, e o
		 * passo com que ela cresce quando a mais perto não serve.
		 *
		 * Começar perto e crescer é o que faz a gruta ficar ENCOSTADA na
		 * cachoeira: o primeiro lugar que serve ganha, e o primeiro lugar
		 * tentado é o mais próximo. Sorteando a distância, metade das grutas
		 * nasceria longe da única coisa que explica por que ela está ali.
		 */
		constexpr float PrimeiraDistanciaDaGruta = 1200.0f;
		constexpr float PassoDaDistanciaDaGruta = 240.0f;
		constexpr int32 DistanciasDaGruta = 12;

		/**
		 * Em quantas direções se procura ao redor da queda.
		 *
		 * Doze, como as horas de um relógio: perto do degrau o rio vem torto de
		 * tanto serpentear, e as duas perpendiculares à corrente — que era o
		 * que se tentava antes — apontam para a água em metade dos casos, porque
		 * a torção dá componente RADIAL à perpendicular e joga a gruta rio
		 * acima, dentro do lago.
		 */
		constexpr int32 RumosDaGruta = 12;

		/**
		 * A folga que um lugar precisa ter para ser aceito, em cima de caber.
		 *
		 * Sem ela a busca aceita o primeiro lugar que cabe, e "cabe" chegou a
		 * ser cinco unidades de margem: uma gruta encostada na calha, que
		 * qualquer mexida na serpentina do rio põe dentro da água. Caber não é
		 * critério; caber com sobra é.
		 */
		constexpr float FolgaDaGruta = 300.0f;

		/**
		 * O passo com que se percorre o rio ao medir a distância até a água.
		 *
		 * O curso é uma curva, e não há fórmula fechada para a distância de um
		 * ponto a ela: mede-se por amostragem. Passo curto custa tempo de
		 * planejamento; passo longo passa por cima de uma volta da serpentina.
		 */
		constexpr float PassoDeSondagemDoRio = 60.0f;
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

	namespace
	{
		/** A menor distância entre um ponto e a água doce de toda a ilha. */
		float MargemDaAgua(const TArray<FRiverCourse>& Cursos, const FVector2D& Ponto)
		{
			float Menor = TNumericLimits<float>::Max();
			for (const FRiverCourse& Curso : Cursos)
			{
				for (float Raio = Curso.SourceRadiusUnits; Raio <= Curso.MouthRadiusUnits;
					Raio += PassoDeSondagemDoRio)
				{
					const float Daqui = static_cast<float>(
						FVector2D::Distance(Ponto, PointAt(Curso, Raio))) - HalfWidthAt(Curso, Raio);
					Menor = FMath::Min(Menor, Daqui);
				}
			}

			return Menor;
		}
	}

	TArray<IslandFeatureLayout::FFeaturePlacement> PlanGrottoes()
	{
		TArray<IslandFeatureLayout::FFeaturePlacement> Grutas;

		const TArray<FRiverCourse> Cursos = Plan();
		const TArray<IslandFeatureLayout::FFeaturePlacement> PecasDaIlha = IslandFeatureLayout::Plan();
		const IslandFeatureLayout::FIslandBounds Limites;

		for (int32 Indice = 0; Indice < Cursos.Num(); ++Indice)
		{
			const FVector2D NaQueda = PointAt(Cursos[Indice], Cursos[Indice].FallRadiusUnits);

			IslandFeatureLayout::FFeaturePlacement Gruta;
			Gruta.Feature = IslandFeatureLayout::EIslandFeature::Cave;
			Gruta.CaveSide = ACaveSystem::GrottoCaveSide;
			Gruta.ClearanceUnits = IslandFeatureLayout::CaveClearanceUnits(Gruta.CaveSide);

			// Água, sempre, e sem passar pelo temperador do plano da ilha: ele
			// dá água a quem está perto da ORLA, e a gruta da cachoeira tem água
			// por causa da cachoeira. O motivo é outro, e o raio não o conhece.
			Gruta.CaveFlavor = ECaveFlavor::Water;

			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("gruta-da-queda-%d"), Indice));
			const int32 PrimeiroRumo = BattleSpread::Below(Semente, 0, RumosDaGruta);

			// O sorteio escolhe por onde COMEÇAR a procurar, não onde ficar. Era
			// ele que decidia o lado antes, e decidir sem olhar é como a gruta
			// ia para dentro do lago: a semente não sabe onde está a água.
			bool bAchou = false;
			for (int32 Tentativa = 0; Tentativa < DistanciasDaGruta && !bAchou; ++Tentativa)
			{
				const float Distancia = PrimeiraDistanciaDaGruta
					+ PassoDaDistanciaDaGruta * static_cast<float>(Tentativa);

				for (int32 Passo = 0; Passo < RumosDaGruta && !bAchou; ++Passo)
				{
					const float Rumo = 2.0f * PI
						* static_cast<float>((PrimeiroRumo + Passo) % RumosDaGruta)
						/ static_cast<float>(RumosDaGruta);

					const FVector2D Centro = NaQueda
						+ FVector2D(FMath::Cos(Rumo), FMath::Sin(Rumo)) * Distancia;

					Gruta.AngleDegrees = FMath::RadiansToDegrees(
						static_cast<float>(FMath::Atan2(Centro.Y, Centro.X)));
					Gruta.RadiusUnits = static_cast<float>(Centro.Size());

					// A folga entra ENGORDANDO a gruta para as perguntas, não
					// afrouxando as perguntas: assim quem decide se duas coisas
					// se tocam continua sendo o plano da ilha, uma vez só, e a
					// gruta não carrega uma segunda cópia dessas contas (L-032).
					IslandFeatureLayout::FFeaturePlacement Inflada = Gruta;
					Inflada.ClearanceUnits = Gruta.ClearanceUnits + FolgaDaGruta;

					if (MargemDaAgua(Cursos, Centro) <= Inflada.ClearanceUnits)
					{
						continue;
					}
					if (!IslandFeatureLayout::FitsOnLand(Inflada, Limites))
					{
						continue;
					}
					if (!IslandFeatureLayout::ClearsTrainingFields(Inflada, Limites))
					{
						continue;
					}

					bool bEncosta = false;
					for (const IslandFeatureLayout::FFeaturePlacement& Peca : PecasDaIlha)
					{
						bEncosta = bEncosta || IslandFeatureLayout::Overlaps(Inflada, Peca);
					}
					for (const IslandFeatureLayout::FFeaturePlacement& Outra : Grutas)
					{
						bEncosta = bEncosta || IslandFeatureLayout::Overlaps(Inflada, Outra);
					}
					if (bEncosta)
					{
						continue;
					}

					bAchou = true;
				}
			}

			// Sem lugar, sem gruta. Plantar onde não cabe devolveria o defeito
			// que a busca existe para não ter: caverna com a quina na água.
			if (bAchou)
			{
				Grutas.Add(Gruta);
			}
		}

		return Grutas;
	}
}
