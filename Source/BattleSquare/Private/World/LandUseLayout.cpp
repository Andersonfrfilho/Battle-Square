// Copyright 2026 Anderson. All Rights Reserved.

#include "World/LandUseLayout.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"
#include "World/TrailLayout.h"
#include "Environment/IslandGeography.h"
#include "Environment/FreshWater.h"
#include "Battle/DeterministicSpread.h"

namespace
{
	/** Quantas fazendas cada vila sustenta. */
	constexpr int32 FazendasPorVila = 2;

	/** O tamanho de uma fazenda, em lotes de vila. */
	constexpr float FazendaEmLotes = 0.85f;

	/** Quantos bosques e quantas clareiras a ilha tem. */
	constexpr int32 QuantosBosques = 9;
	constexpr int32 QuantasClareiras = 6;

	constexpr float MenorBosque = 0.035f;
	constexpr float MaiorBosque = 0.075f;

	constexpr float MenorClareira = 0.012f;
	constexpr float MaiorClareira = 0.028f;

	/** No bosque nasce quase o dobro de árvore. */
	constexpr float DensidadeDoBosque = 1.85f;

	/**
	 * A clareira fechada mora LONGE da trilha, e é a regra dela.
	 *
	 * Clareira que a trilha corta é um alargamento do caminho. O que faz dela
	 * um lugar é ninguém passar por ali sem querer.
	 */
	constexpr float LongeDaTrilhaEmLarguras = 6.0f;

	FVector2D DoRumo(float Graus, float Distancia)
	{
		const float Radianos = FMath::DegreesToRadians(Graus);
		return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * Distancia;
	}

	/** O ponto de água doce mais perto — fonte ou rio. */
	FVector2D AguaMaisPerto(const FVector2D& Daqui)
	{
		FVector2D Melhor = Daqui;
		float Menor = TNumericLimits<float>::Max();

		for (const FreshWater::FSpring& Fonte : FreshWater::PlanSprings())
		{
			const float Ate = FVector2D::DistSquared(Daqui, Fonte.CenterUnits);
			if (Ate < Menor)
			{
				Menor = Ate;
				Melhor = Fonte.CenterUnits;
			}
		}

		for (const FreshWater::FBrook& Corrego : FreshWater::PlanBrooks())
		{
			for (const FVector2D& Ponto : Corrego.PointsUnits)
			{
				const float Ate = FVector2D::DistSquared(Daqui, Ponto);
				if (Ate < Menor)
				{
					Menor = Ate;
					Melhor = Ponto;
				}
			}
		}

		return Melhor;
	}

	TArray<FGroundUsePatch> Tracar()
	{
		TArray<FGroundUsePatch> Manchas;

		const float Lote = VillageLayout::PlotHalfExtentUnits();

		// AS FAZENDAS, encostadas nas vilas e VIRADAS PARA A ÁGUA.
		//
		// A direção não é sorteada: é a da água mais perto. Roçado longe do
		// rio é roçado que ninguém rega, e a fazenda estaria ali só porque o
		// gerador precisava pôr uma em algum lugar.
		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
			{
				// O cais não planta: ele é uma porta, e dar-lhe lavoura o
				// transformaria em destino — o que a spec da região proíbe.
				continue;
			}

			const FVector2D ParaAAgua =
				(AguaMaisPerto(Assentamento.CenterUnits) - Assentamento.CenterUnits).GetSafeNormal();
			const FVector2D DeLado(-ParaAAgua.Y, ParaAAgua.X);

			const float Clareira =
				VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind);
			const float Meia = Lote * FazendaEmLotes;

			for (int32 Qual = 0; Qual < FazendasPorVila; ++Qual)
			{
				const float ParaOnde = (Qual == 0) ? 1.0f : -1.0f;

				FGroundUsePatch Fazenda;
				Fazenda.Use = EGroundUse::Fazenda;
				Fazenda.HalfExtentUnits = Meia;
				Fazenda.CenterUnits = Assentamento.CenterUnits
					+ ParaAAgua * (Clareira + Meia)
					+ DeLado * (ParaOnde * Meia * 1.15f);

				Manchas.Add(Fazenda);
			}
		}

		// OS BOSQUES, espalhados pelo anel habitável.
		for (int32 Indice = 0; Indice < QuantosBosques; ++Indice)
		{
			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("bosque-fechado-%d"), Indice));

			const float Rumo = (360.0f * Indice / QuantosBosques)
				+ BattleSpread::Between(-16.0f, 16.0f, BattleSpread::Fraction(Semente, 0));
			const float Anel = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
				0.20f, 0.78f, BattleSpread::Fraction(Semente, 1));

			FGroundUsePatch Bosque;
			Bosque.Use = EGroundUse::Bosque;
			Bosque.CenterUnits = DoRumo(Rumo, Anel);
			Bosque.HalfExtentUnits = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
				MenorBosque, MaiorBosque, BattleSpread::Fraction(Semente, 2));

			Manchas.Add(Bosque);
		}

		return Manchas;
	}
}

TArray<FGroundUsePatch> LandUseLayout::Plan()
{
	// Traçado uma vez: as clareiras precisam consultar as trilhas, e as
	// trilhas custam um Dijkstra sobre a ilha inteira. Isto é perguntado por
	// ÁRVORE plantada.
	static TArray<FGroundUsePatch> Manchas = []()
	{
		TArray<FGroundUsePatch> Tudo = Tracar();

		// AS CLAREIRAS FECHADAS, por último e só onde nenhuma trilha passa
		// perto. Elas dependem das trilhas, e as trilhas não dependem delas —
		// por isso vêm depois, e não no meio.
		const float Longe = TrailLayout::HalfWidthUnits() * LongeDaTrilhaEmLarguras;

		for (int32 Indice = 0; Indice < QuantasClareiras; ++Indice)
		{
			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("clareira-fechada-%d"), Indice));

			// Tenta vários rumos e fica com o primeiro que serve. Sorteando um
			// só, metade das clareiras cairia na beira de uma trilha e deixaria
			// de ser fechada — e "fechada" é a única coisa que ela é.
			for (int32 Tentativa = 0; Tentativa < 12; ++Tentativa)
			{
				const float Rumo = BattleSpread::Between(0.0f, 360.0f,
					BattleSpread::Fraction(Semente, Tentativa));
				const float Anel = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
					0.25f, 0.72f, BattleSpread::Fraction(Semente, Tentativa + 20));

				const FVector2D Onde = DoRumo(Rumo, Anel);

				bool bPertoDaTrilha = false;
				for (float Passo = 0.0f; Passo <= Longe && !bPertoDaTrilha; Passo += Longe * 0.25f)
				{
					for (int32 Lado = 0; Lado < 4; ++Lado)
					{
						const FVector2D Perto = Onde + DoRumo(90.0f * Lado, Passo);
						if (TrailLayout::IsOnTrail(Perto))
						{
							bPertoDaTrilha = true;
							break;
						}
					}
				}

				if (bPertoDaTrilha)
				{
					continue;
				}

				FGroundUsePatch Clareira;
				Clareira.Use = EGroundUse::ClareiraFechada;
				Clareira.CenterUnits = Onde;
				Clareira.HalfExtentUnits = IslandGeography::LandRadiusUnits()
					* BattleSpread::Between(MenorClareira, MaiorClareira,
						BattleSpread::Fraction(Semente, 40));

				Tudo.Add(Clareira);
				break;
			}
		}

		return Tudo;
	}();

	return Manchas;
}

EGroundUse LandUseLayout::UseAt(const FVector2D& PositionUnits)
{
	// A ordem responde ao empate: fazenda e clareira são VAZIOS, e vazio ganha
	// de bosque. Sem uma ordem, o mesmo ponto teria dois usos conforme a
	// ordem em que as manchas foram criadas.
	EGroundUse Achado = EGroundUse::Nenhum;

	for (const FGroundUsePatch& Mancha : Plan())
	{
		const FVector2D Daqui = PositionUnits - Mancha.CenterUnits;
		if (FMath::Abs(Daqui.X) > Mancha.HalfExtentUnits
			|| FMath::Abs(Daqui.Y) > Mancha.HalfExtentUnits)
		{
			continue;
		}

		if (Mancha.Use != EGroundUse::Bosque)
		{
			return Mancha.Use;
		}

		Achado = Mancha.Use;
	}

	return Achado;
}

bool LandUseLayout::BlocksPlanting(const FVector2D& PositionUnits)
{
	const EGroundUse Uso = UseAt(PositionUnits);
	return Uso == EGroundUse::Fazenda || Uso == EGroundUse::ClareiraFechada;
}

float LandUseLayout::PlantingDensityAt(const FVector2D& PositionUnits)
{
	switch (UseAt(PositionUnits))
	{
		case EGroundUse::Fazenda:
		case EGroundUse::ClareiraFechada:
			return 0.0f;

		case EGroundUse::Bosque:
			return DensidadeDoBosque;

		default:
			return 1.0f;
	}
}
