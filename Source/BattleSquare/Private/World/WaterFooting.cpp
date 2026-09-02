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
		// FORA DA COSTA É MAR, e mar é FUNDO.
		//
		// Isto faltava, e quem achou foi a prova de que "onde o pé está seco
		// não há fluido": fora da terra, `FluidAt` dizia água salgada e `At`
		// dizia seco — as duas perguntas discordando sobre o mesmo lugar. O
		// painel teria dito "seco em água salgada", e o jogador aprenderia a
		// não confiar nele.
		//
		// O mar não está no traçado como curso; ele é o que sobra depois que a
		// terra acaba, e por isso nenhum laço sobre rios jamais o encontraria.
		if (Onde.Size() > Assado.CoastRadiusAt(FMath::Atan2(Onde.Y, Onde.X)))
		{
			return EWaterFooting::Fundo;
		}

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

	EFluidKind FluidAt(const UIslandBakedPlan& Assado, const FVector2D& Onde)
	{
		// FORA DA COSTA É MAR. Ele não está no traçado como curso — é o que
		// sobra depois que a terra acaba —, então a pergunta se responde pela
		// linha da costa, que o assado tem.
		if (Onde.Size() > Assado.CoastRadiusAt(FMath::Atan2(Onde.Y, Onde.X)))
		{
			return EFluidKind::AguaSalgada;
		}

		// A água mais FUNDA ganha, pela mesma razão do `At`: parar na primeira
		// encontrada faria o resultado depender da ordem do assado, que é
		// desempate disfarçado de regra.
		float MaiorMeiaLargura = 0.0f;
		EFluidKind Achado = EFluidKind::Nenhum;

		for (const FBakedRiver& Curso : Assado.Rivers)
		{
			for (int32 Ponto = 0; Ponto + 1 < Curso.PointsUnits.Num(); ++Ponto)
			{
				const float Meia = FMath::Max(
					Curso.HalfWidthUnits.IsValidIndex(Ponto)
						? Curso.HalfWidthUnits[Ponto] : 0.0f,
					Curso.HalfWidthUnits.IsValidIndex(Ponto + 1)
						? Curso.HalfWidthUnits[Ponto + 1] : 0.0f);

				if (Meia <= 0.0f || Meia <= MaiorMeiaLargura)
				{
					continue;
				}

				const FVector2D NoTrecho = FMath::ClosestPointOnSegment2D(
					Onde, Curso.PointsUnits[Ponto], Curso.PointsUnits[Ponto + 1]);

				if (FVector2D::DistSquared(NoTrecho, Onde) <= Meia * Meia)
				{
					MaiorMeiaLargura = Meia;

					// O fluido do PONTO, não do curso: termal é propriedade da
					// posição, e um rio nasce fervendo e chega frio ao mar.
					Achado = Curso.FluidByPoint.IsValidIndex(Ponto)
						? static_cast<EFluidKind>(Curso.FluidByPoint[Ponto])
						: EFluidKind::AguaDoce;
				}
			}
		}

		for (const FBakedBrook& Corrego : Assado.Brooks)
		{
			float Meia = 0.0f;
			if (ChaoMolhado::DentroDaCalha(Corrego.PointsUnits, Onde,
				[&Corrego](int32) { return Corrego.HalfWidthUnits; }, Meia)
				&& Meia > MaiorMeiaLargura)
			{
				MaiorMeiaLargura = Meia;
				Achado = static_cast<EFluidKind>(Corrego.Fluid);
			}
		}

		for (const FBakedSpring& Fonte : Assado.Springs)
		{
			if (FVector2D::Distance(Onde, Fonte.CenterUnits) <= Fonte.PoolHalfWidthUnits
				&& Fonte.PoolHalfWidthUnits > MaiorMeiaLargura)
			{
				MaiorMeiaLargura = Fonte.PoolHalfWidthUnits;
				Achado = static_cast<EFluidKind>(Fonte.Fluid);
			}
		}

		return Achado;
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
