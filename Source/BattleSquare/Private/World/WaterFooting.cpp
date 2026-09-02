// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WaterFooting.h"

#include "Environment/FreshWater.h"
#include "World/IslandBakedPlan.h"
#include "World/TrailLayout.h"

namespace ChaoMolhado
{
	/**
	 * Quanto o passo rende em cada chão.
	 *
	 * A tabela, num lugar só. Espalhada pelos chamadores, ela viraria três
	 * velocidades diferentes para a mesma água na terceira edição.
	 */
	constexpr float PassoEmTerra = 1.0f;
	constexpr float PassoNoVau = 0.55f;
	constexpr float PassoNoFundo = 0.25f;

	/**
	 * O pé encontra água quando está DENTRO da calha, não perto dela.
	 *
	 * Mede contra os SEGMENTOS do curso, nunca contra os pontos amostrados. A
	 * distância entre duas amostras é o comprimento do curso dividido por 40, e
	 * isso é MAIOR que a calha na maior parte da ilha: medindo por pontos, o
	 * meio de cada segmento sai seco, e um vau inteiro pode cair nessa fresta.
	 *
	 * Foi assim que uma travessia de vau apareceu em terra firme — a grade de
	 * medição era mais grossa que a coisa que ela precisava enxergar, que é o
	 * defeito que neste projeto já se disfarçou de quatro outros.
	 */
	bool DentroDaCalha(const TArray<FVector2D>& Pontos, const FVector2D& Onde,
		TFunctionRef<float(int32)> MeiaLarguraNoPonto, float& MeiaLarguraAchada)
	{
		bool bMolhou = false;
		for (int32 Ponto = 0; Ponto + 1 < Pontos.Num(); ++Ponto)
		{
			// A calha do trecho é a MAIOR das duas pontas: usar a menor
			// afinaria o rio justo onde ele engrossa.
			const float Meia =
				FMath::Max(MeiaLarguraNoPonto(Ponto), MeiaLarguraNoPonto(Ponto + 1));
			if (Meia <= 0.0f)
			{
				continue;
			}

			const FVector2D NoTrecho = FMath::ClosestPointOnSegment2D(
				Onde, Pontos[Ponto], Pontos[Ponto + 1]);

			if (FVector2D::DistSquared(NoTrecho, Onde) <= Meia * Meia)
			{
				MeiaLarguraAchada = FMath::Max(MeiaLarguraAchada, Meia);
				bMolhou = true;
			}
		}
		return bMolhou;
	}
}

namespace WaterFooting
{
	/**
	 * O valor de `TrailLayout::ECrossingKind::Vau` como número.
	 *
	 * O tipo viaja assado como `uint8` porque `ECrossingKind` mora dentro de um
	 * `namespace`, onde `UENUM` não existe. A conversão fica num lugar só.
	 */
	constexpr uint8 ETravessiaVau =
		static_cast<uint8>(TrailLayout::ECrossingKind::Vau);

	EWaterFooting At(const UIslandBakedPlan& Assado, const FVector2D& Onde)
	{
		// A água mais FUNDA ganha: quem está no encontro de um córrego com um
		// rio está no rio. Parar na primeira água encontrada faria o resultado
		// depender da ordem do assado, que é desempate disfarçado de regra.
		float MaiorMeiaLargura = 0.0f;

		for (const FBakedRiver& Curso : Assado.Rivers)
		{
			float Meia = 0.0f;
			if (ChaoMolhado::DentroDaCalha(Curso.PointsUnits, Onde,
				[&Curso](int32 Ponto)
				{
					return Curso.HalfWidthUnits.IsValidIndex(Ponto)
						? Curso.HalfWidthUnits[Ponto] : 0.0f;
				}, Meia))
			{
				MaiorMeiaLargura = FMath::Max(MaiorMeiaLargura, Meia);
			}

			// Lago e poço são água PARADA e funda — e é onde a queda cai.
			if (Curso.bHasLake
				&& FVector2D::Distance(Onde, Curso.LakeCenterUnits)
					<= FreshWater::LakeHalfWidthUnits())
			{
				MaiorMeiaLargura =
					FMath::Max(MaiorMeiaLargura, FreshWater::LakeHalfWidthUnits());
			}

			if (Curso.bHasFall
				&& FVector2D::Distance(Onde, Curso.FallCenterUnits)
					<= Curso.PlungePoolHalfWidthUnits)
			{
				MaiorMeiaLargura =
					FMath::Max(MaiorMeiaLargura, Curso.PlungePoolHalfWidthUnits);
			}
		}

		for (const FBakedBrook& Corrego : Assado.Brooks)
		{
			float Meia = 0.0f;
			if (ChaoMolhado::DentroDaCalha(Corrego.PointsUnits, Onde,
				[&Corrego](int32) { return Corrego.HalfWidthUnits; }, Meia))
			{
				MaiorMeiaLargura = FMath::Max(MaiorMeiaLargura, Meia);
			}
		}

		for (const FBakedSpring& Fonte : Assado.Springs)
		{
			if (FVector2D::Distance(Onde, Fonte.CenterUnits) <= Fonte.PoolHalfWidthUnits)
			{
				MaiorMeiaLargura = FMath::Max(MaiorMeiaLargura, Fonte.PoolHalfWidthUnits);
			}
		}

		if (MaiorMeiaLargura <= 0.0f)
		{
			return EWaterFooting::Seco;
		}

		// ONDE SE PASSA A PÉ, quem diz é o TRAÇADO — as travessias que ele
		// marcou como vau. Não é a largura.
		//
		// Eu tinha escrito "a largura decide" e MEDI: nenhum ponto de rio
		// saía vau, porque a meia-largura mínima é 481 e o limiar do a-pé é
		// 30% da calha do rio. O traçado escolhe o vau pela FUNDURA no ponto
		// (todas as 30 abaixo de 94 unidades), que é coisa que a largura não
		// diz — e um rio largo e raso existe.
		//
		// Consultar as travessias é ler a regra onde ela mora, em vez de
		// reconstruí-la aqui e ter duas que concordam até a primeira edição.
		for (const FBakedCrossing& Travessia : Assado.Crossings)
		{
			if (Travessia.Kind != static_cast<uint8>(ETravessiaVau))
			{
				continue;
			}

			if (FVector2D::DistSquared(Onde, Travessia.CenterUnits)
				<= MaiorMeiaLargura * MaiorMeiaLargura)
			{
				return EWaterFooting::Vau;
			}
		}

		// O CÓRREGO se atravessa a pé em qualquer ponto — é o que o separa do
		// rio, que precisa de obra. A largura decide AQUI, e só aqui.
		return FreshWater::NavigabilityForHalfWidth(MaiorMeiaLargura)
			== FreshWater::ENavigability::APe
			? EWaterFooting::Vau
			: EWaterFooting::Fundo;
	}

	float SpeedMultiplierFor(EWaterFooting Chao)
	{
		switch (Chao)
		{
		case EWaterFooting::Vau:   return ChaoMolhado::PassoNoVau;
		case EWaterFooting::Fundo: return ChaoMolhado::PassoNoFundo;
		default: break;
		}
		return ChaoMolhado::PassoEmTerra;
	}

	const TCHAR* DebugName(EWaterFooting Chao)
	{
		switch (Chao)
		{
		case EWaterFooting::Vau:   return TEXT("vau");
		case EWaterFooting::Fundo: return TEXT("agua funda");
		default: break;
		}
		return TEXT("seco");
	}
}
