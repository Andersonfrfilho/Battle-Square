// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandFeatureLayout.h"

#include "Environment/CaveSystem.h"
#include "Environment/Volcano.h"
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
		 * na terra.
		 *
		 * Foi 4400 enquanto a ilha tinha 6000 de raio. A ilha passou a ter
		 * 20000 e os números ficaram — o resultado foi todo o relevo amontoado
		 * num miolo de um quinto do mapa, com quatro quintos de nada em volta.
		 * A 12000 a distância entre peças vizinhas é maior que a maior peça,
		 * e andar de uma à outra volta a ser viagem.
		 */
		/**
		 * Os anéis são FRAÇÕES do raio da terra, não unidades.
		 *
		 * Eram números absolutos — 12.000, 10.500, 15.000 — e ninguém notava
		 * porque a ilha sempre teve 20.000. Ao crescê-la para 100.000, tudo
		 * ficaria amontoado nos primeiros 17% do raio, com quase um quilômetro
		 * de nada em volta. Pareceria defeito de geração, e seria decisão
		 * antiga tomada quando só existia um tamanho.
		 *
		 * As frações abaixo dão EXATAMENTE os números de antes quando o raio é
		 * 20.000 — zero mudança no mundo de hoje, e escala no de amanhã.
		 *
		 * São FUNÇÕES, e não constantes. Um `const float` no escopo do
		 * namespace é calculado uma vez, no carregamento — e aí o raio
		 * configurado depois nunca chega nele. Foi exatamente o que aconteceu
		 * na primeira tentativa, e o teste da escala pegou.
		 */
		FORCEINLINE float DoRaio(float Fracao)
		{
			return IslandGeography::LandRadiusUnits() * Fracao;
		}

		FORCEINLINE float AnelDaBorda() { return DoRaio(0.60f); }

		/**
		 * A caverna grande vem mais para dentro, e encaixada num vão.
		 *
		 * Ela é a única peça larga demais para ter folga radial: no anel da
		 * borda a quina dela sairia na água. Puxá-la para dentro resolve a
		 * água e cria o outro problema — passa a raspar no anel dos campos —,
		 * e é por isso que o ângulo dela não é livre: 36° é o meio do vão
		 * entre os campos de 0° e 72°.
		 */
		FORCEINLINE float AnelDaCavernaGrande() { return DoRaio(0.525f); }
		constexpr float AnguloDaCavernaGrande = 36.0f;

		/**
		 * O vulcão mora sozinho, muito mais para fora que o resto.
		 *
		 * Ele é a peça mais larga, e continua fora do anel mesmo agora que o
		 * anel tem folga: sozinho num raio maior, ele é o marco que se avista
		 * de longe e diz de que lado fica o setor de fogo.
		 *
		 * 180° é o meio do setor de vulcão da geografia (144° a 216°), para o
		 * marco cair no bioma que ele anuncia em vez de perto da divisa.
		 */
		// A geografia é a dona: ela precisa do mesmo ponto para saber que o
		// chão ali é queimado.
		FORCEINLINE float AnelDoVulcao() { return IslandGeography::VolcanoRingUnits(); }
		FORCEINLINE float AnguloDoVulcao() { return IslandGeography::VolcanoAngleDegrees(); }

		/**
		 * A caverna pequena mora na orla, e é por isso que ela tem água.
		 *
		 * Ela estava no anel da borda com as outras, e ali dentro nenhuma das
		 * três podia ser caverna de mar: água salgada a oito mil unidades da
		 * praia é cenário mentindo sobre onde a pessoa está.
		 */
		FORCEINLINE float AnelDaCavernaDaAgua() { return DoRaio(0.85f); }

		/**
		 * A caverna média é a de lava, e por isso ela vive perto do vulcão.
		 *
		 * Estava a 150° no anel da borda, o que a punha a mais de sete mil
		 * unidades do monte — longe demais para o calor alcançar, e o resultado
		 * era uma ilha com três cavernas e nenhuma de lava.
		 *
		 * 160° e 13000 a põem a cerca de cinco mil do vulcão: dentro do calor,
		 * e ainda com mais de mil de folga sobre a soma das duas bases.
		 */
		FORCEINLINE float AnelDaCavernaDeLava() { return DoRaio(0.65f); }
		constexpr float AnguloDaCavernaDeLava = 160.0f;

		/**
		 * Até onde o calor do vulcão chega.
		 *
		 * Medido do centro do vulcão ao centro da caverna. Maior que isso e a
		 * caverna de lava ficaria longe demais do monte para alguém ligar as
		 * duas coisas olhando.
		 */
		FORCEINLINE float AlcanceDoCalor() { return IslandGeography::VolcanoHeatRadiusUnits(); }

		FFeaturePlacement Montanha(float AnguloEmGraus)
		{
			FFeaturePlacement Peca;
			Peca.Feature = EIslandFeature::WalkableMountain;
			Peca.AngleDegrees = AnguloEmGraus;
			Peca.RadiusUnits = AnelDaBorda();
			Peca.ClearanceUnits = MountainClearanceUnits();
			return Peca;
		}

		FFeaturePlacement Vulcao(float AnguloEmGraus, float RaioDoAnel)
		{
			FFeaturePlacement Peca;
			Peca.Feature = EIslandFeature::Volcano;
			Peca.AngleDegrees = AnguloEmGraus;
			Peca.RadiusUnits = RaioDoAnel;
			Peca.ClearanceUnits = VolcanoClearanceUnits();
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

	float VolcanoClearanceUnits()
	{
		return AVolcano::DefaultBaseRadiusUnits;
	}

	float CaveClearanceUnits(int32 Side)
	{
		// Meia-diagonal, e não meio-lado: o quadrado da caverna não aponta para
		// o vizinho, e o meio-lado deixaria a quina passar por dentro dele.
		return 0.5f * ACaveSystem::FootprintForSide(Side) * UE_SQRT_2;
	}

	namespace
	{
		/**
		 * Dá sabor às cavernas do plano, pelo LUGAR de cada uma.
		 *
		 * Escrever o sabor à mão ao lado do ângulo funcionaria hoje e mentiria
		 * na primeira vez que alguém movesse uma peça: o texto continuaria
		 * dizendo "lava" com o vulcão do outro lado da ilha.
		 */
		void TemperaAsCavernas(TArray<FFeaturePlacement>& Pecas)
		{
			const FFeaturePlacement* Vulcao = Pecas.FindByPredicate(
				[](const FFeaturePlacement& Peca)
				{
					return Peca.Feature == EIslandFeature::Volcano;
				});

			// A orla e um pouco antes dela: a caverna de mar precisa estar perto
			// o bastante da água para a água fazer sentido lá dentro.
			const float ComecoDaOrla = IslandGeography::LandRadiusUnits()
				- 2.0f * IslandGeography::BeachWidthUnits();

			for (FFeaturePlacement& Peca : Pecas)
			{
				if (Peca.Feature != EIslandFeature::Cave)
				{
					continue;
				}

				const bool bNoCalor = Vulcao != nullptr
					&& FVector2D::Distance(Peca.CenterUnits(), Vulcao->CenterUnits()) <= AlcanceDoCalor();

				if (bNoCalor)
				{
					Peca.CaveFlavor = ECaveFlavor::Lava;
				}
				else if (Peca.RadiusUnits >= ComecoDaOrla)
				{
					Peca.CaveFlavor = ECaveFlavor::Water;
				}
				else
				{
					Peca.CaveFlavor = ECaveFlavor::Dry;
				}
			}
		}
	}

	TArray<FFeaturePlacement> Plan()
	{
		// Três montanhas bem separadas, e as cavernas nos vãos entre elas: quem
		// sobe uma montanha vê a boca da seguinte, e é isso que faz o anel ser
		// um caminho em vez de três lugares soltos.
		TArray<FFeaturePlacement> Pecas;
		Pecas.Add(Caverna(AnguloDaCavernaGrande, AnelDaCavernaGrande(), ACaveSystem::LargeCaveSide));
		Pecas.Add(Montanha(90.0f));
		Pecas.Add(Caverna(AnguloDaCavernaDeLava, AnelDaCavernaDeLava(), ACaveSystem::MediumCaveSide));
		Pecas.Add(Montanha(210.0f));
		Pecas.Add(Caverna(270.0f, AnelDaCavernaDaAgua(), ACaveSystem::SmallCaveSide));
		Pecas.Add(Montanha(330.0f));
		Pecas.Add(Vulcao(AnguloDoVulcao(), AnelDoVulcao()));

		TemperaAsCavernas(Pecas);
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

uint32 IslandFeatureLayout::SeedForPlacement(int32 WorldSeed, const FFeaturePlacement& Placement)
{
	return static_cast<uint32>(WorldSeed)
		+ static_cast<uint32>(FMath::RoundToInt(Placement.AngleDegrees));
}
