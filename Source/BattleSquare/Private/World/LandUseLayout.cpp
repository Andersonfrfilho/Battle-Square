// Copyright 2026 Anderson. All Rights Reserved.

#include "World/LandUseLayout.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"
#include "World/TrailLayout.h"
#include "World/WorldBudget.h"
#include "Environment/IslandGeography.h"
#include "Environment/FreshWater.h"
#include "Battle/DeterministicSpread.h"

namespace
{
	/** Quantas fazendas cada vila sustenta. */


	/** O tamanho de uma fazenda, em lotes de vila. */
	constexpr float FazendaEmLotes = 0.85f;

	/** Quantos criadouros, e o tamanho deles. Maiores que a fazenda: pet corre. */

	constexpr float CriadouroEmLotes = 1.25f;

	/** Loja, acampamento e pomar, em lotes de vila. */
	constexpr float LojaEmLotes = 0.30f;
	constexpr float AcampamentoEmLotes = 0.26f;
	constexpr float PomarEmLotes = 0.55f;

	/** Quantos decks, e o tamanho deles. */
	constexpr int32 QuantosDecks = 7;
	constexpr float DeckEmLotes = 0.22f;

	/** Quantos bosques e quantas clareiras a ilha tem. */



	constexpr float MenorBosque = 0.035f;
	constexpr float MaiorBosque = 0.075f;

	constexpr float MenorClareira = 0.012f;
	constexpr float MaiorClareira = 0.028f;

	/** No bosque nasce quase o dobro de árvore. */
	constexpr float DensidadeDoBosque = 1.85f;
	constexpr float DensidadeDoPomar = 2.2f;

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

	/** Nome específico: `Tracar` colidia com o de `TrailLayout` no unity build. */
	TArray<FGroundUsePatch> TracarUsoDoSolo()
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

			const int32 FazendasPorVila =
				WorldBudget::FarmsPerSettlement(IslandGeography::IslandBiome());
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

		// OS CRIADOUROS, longe de tudo.
		//
		// A regra é a distância: perto de assentamento não é criadouro, é
		// quintal. Eles ficam a pelo menos três clareiras de qualquer vila, e
		// isso é o que faz chegar lá ser uma viagem.
		const int32 QuantosCriadouros = WorldBudget::BreederCount(IslandGeography::IslandBiome());
		for (int32 Indice = 0; Indice < QuantosCriadouros; ++Indice)
		{
			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("criadouro-de-pets-%d"), Indice));

			for (int32 Tentativa = 0; Tentativa < 14; ++Tentativa)
			{
				const float Rumo = BattleSpread::Between(0.0f, 360.0f,
					BattleSpread::Fraction(Semente, Tentativa));
				const float Anel = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
					0.30f, 0.80f, BattleSpread::Fraction(Semente, Tentativa + 30));

				const FVector2D Onde = DoRumo(Rumo, Anel);

				bool bPertoDeAlguem = false;
				for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
				{
					const float Longe =
						VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 3.0f;

					if (FVector2D::Distance(Onde, Assentamento.CenterUnits) < Longe)
					{
						bPertoDeAlguem = true;
						break;
					}
				}

				if (bPertoDeAlguem || !IslandGeography::IsOnLand(Onde))
				{
					continue;
				}

				FGroundUsePatch Criadouro;
				Criadouro.Use = EGroundUse::Criadouro;
				Criadouro.CenterUnits = Onde;
				Criadouro.HalfExtentUnits = Lote * CriadouroEmLotes;
				Manchas.Add(Criadouro);
				break;
			}
		}

		// AS LOJAS, nos CRUZAMENTOS de trilha.
		//
		// Elas não são espalhadas: elas são POSTAS onde dois caminhos se
		// cruzam, porque é onde passa gente das duas direções. Achar o
		// cruzamento é a única parte cara, e ela é uma varredura de pontos de
		// trilhas diferentes que se aproximam.
		{
			const TArray<FTrailRoute>& Trilhas = TrailLayout::Plan();
			const float Encosta = TrailLayout::HalfWidthUnits() * 2.2f;

			int32 Postas = 0;
			const int32 Quantas = WorldBudget::RoadsideShopCount(IslandGeography::IslandBiome());

			for (int32 Uma = 0; Uma < Trilhas.Num() && Postas < Quantas; ++Uma)
			{
				for (int32 Outra = Uma + 1; Outra < Trilhas.Num() && Postas < Quantas; ++Outra)
				{
					bool bAchou = false;

					for (int32 A = 0; A < Trilhas[Uma].PointsUnits.Num() && !bAchou; A += 6)
					{
						for (int32 B = 0; B < Trilhas[Outra].PointsUnits.Num(); B += 6)
						{
							const FVector2D Aqui = Trilhas[Uma].PointsUnits[A];
							if (FVector2D::Distance(Aqui, Trilhas[Outra].PointsUnits[B]) > Encosta)
							{
								continue;
							}

							// Longe das vilas: dentro da clareira já é a vila,
							// e ali a loja seria só mais um prédio dela.
							bool bNaVila = false;
							for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
							{
								if (FVector2D::Distance(Aqui, Assentamento.CenterUnits)
									< VillageLayout::ClearingHalfExtentUnitsFor(
										Assentamento.Kind) * 1.5f)
								{
									bNaVila = true;
									break;
								}
							}

							if (bNaVila)
							{
								continue;
							}

							FGroundUsePatch Loja;
							Loja.Use = EGroundUse::Loja;
							Loja.CenterUnits = Aqui;
							Loja.HalfExtentUnits = Lote * LojaEmLotes;
							Manchas.Add(Loja);

							++Postas;
							bAchou = true;
							break;
						}
					}
				}
			}
		}

		// OS ACAMPAMENTOS: perto da trilha e longe da vila, as duas coisas.
		{
			const TArray<FTrailRoute>& Trilhas = TrailLayout::Plan();
			const int32 Quantos = WorldBudget::CampCount(IslandGeography::IslandBiome());

			for (int32 Indice = 0; Indice < Quantos && Trilhas.Num() > 0; ++Indice)
			{
				const uint32 Semente = BattleSpread::SeedFromText(
					FString::Printf(TEXT("acampamento-%d"), Indice));

				const FTrailRoute& Trilha =
					Trilhas[BattleSpread::Below(Semente, 0, Trilhas.Num())];
				if (Trilha.PointsUnits.Num() < 3)
				{
					continue;
				}

				const FVector2D NoCaminho = Trilha.PointsUnits[
					BattleSpread::Below(Semente, 1, Trilha.PointsUnits.Num())];

				bool bPertoDeVila = false;
				for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
				{
					if (FVector2D::Distance(NoCaminho, Assentamento.CenterUnits)
						< VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 2.5f)
					{
						bPertoDeVila = true;
						break;
					}
				}

				if (bPertoDeVila)
				{
					continue;
				}

				// Ao LADO da trilha, não em cima: acampar no meio do caminho é
				// atrapalhar quem passa.
				const float Rumo = BattleSpread::Between(0.0f, 360.0f,
					BattleSpread::Fraction(Semente, 2));

				FGroundUsePatch Acampamento;
				Acampamento.Use = EGroundUse::Acampamento;
				Acampamento.CenterUnits = NoCaminho
					+ DoRumo(Rumo, TrailLayout::HalfWidthUnits() * 2.4f);
				Acampamento.HalfExtentUnits = Lote * AcampamentoEmLotes;
				Manchas.Add(Acampamento);
			}
		}

		// OS POMARES CUIDADOS, colados nas fazendas: é a mesma pessoa que cuida
		// dos dois, e pomar leva anos — ele só existe onde alguém ficou.
		{
			TArray<FVector2D> Lavouras;
			for (const FGroundUsePatch& Mancha : Manchas)
			{
				if (Mancha.Use == EGroundUse::Fazenda)
				{
					Lavouras.Add(Mancha.CenterUnits);
				}
			}

			const int32 Quantos =
				WorldBudget::TendedOrchardCount(IslandGeography::IslandBiome());

			for (int32 Indice = 0; Indice < Quantos && Lavouras.Num() > 0; ++Indice)
			{
				const uint32 Semente = BattleSpread::SeedFromText(
					FString::Printf(TEXT("pomar-cuidado-%d"), Indice));

				const FVector2D DaLavoura =
					Lavouras[BattleSpread::Below(Semente, 0, Lavouras.Num())];
				const float Rumo = BattleSpread::Between(0.0f, 360.0f,
					BattleSpread::Fraction(Semente, 1));

				FGroundUsePatch Pomar;
				Pomar.Use = EGroundUse::Pomar;
				Pomar.HalfExtentUnits = Lote * PomarEmLotes;
				Pomar.CenterUnits = DaLavoura
					+ DoRumo(Rumo, Lote * (FazendaEmLotes + PomarEmLotes) * 1.1f);
				Manchas.Add(Pomar);
			}
		}

		// E OS SELVAGENS, longe de tudo: achá-los é sorte, e é o que faz valer
		// a pena sair da trilha.
		{
			const int32 Quantos = WorldBudget::WildOrchardCount(IslandGeography::IslandBiome());

			for (int32 Indice = 0; Indice < Quantos; ++Indice)
			{
				const uint32 Semente = BattleSpread::SeedFromText(
					FString::Printf(TEXT("pomar-selvagem-%d"), Indice));

				const float Rumo = BattleSpread::Between(0.0f, 360.0f,
					BattleSpread::Fraction(Semente, 0));
				const float Anel = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
					0.25f, 0.82f, BattleSpread::Fraction(Semente, 1));

				const FVector2D Onde = DoRumo(Rumo, Anel);
				if (!IslandGeography::IsOnLand(Onde) || TrailLayout::IsOnTrail(Onde))
				{
					continue;
				}

				FGroundUsePatch Pomar;
				Pomar.Use = EGroundUse::PomarSelvagem;
				Pomar.CenterUnits = Onde;
				Pomar.HalfExtentUnits = Lote * PomarEmLotes * 0.85f;
				Manchas.Add(Pomar);
			}
		}

		// OS DECKS, na margem de água que aguenta barco grande.
		//
		// A água deste mundo virou caminho — há balsa, barco grande e pequeno,
		// e rio que liga tudo. Sem lugar de atracar, o barco não tem de onde
		// sair e a navegabilidade fica sendo tabela que ninguém usa.
		{
			int32 Postos = 0;

			for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
			{
				if (Postos >= QuantosDecks)
				{
					break;
				}

				const float Meia = FreshWater::HalfWidthAtProgress(Curso, 0.5f);
				if (FreshWater::NavigabilityForHalfWidth(Meia)
					!= FreshWater::ENavigability::BarcoGrande)
				{
					continue;
				}

				// Na MARGEM, afastado pela lâmina: deck dentro da água é barco
				// afundado, e o achatamento do chão não vale sobre o rio.
				const FVector2D NoRio = FreshWater::PointAtProgress(Curso, 0.5f);
				const FVector2D Adiante = FreshWater::PointAtProgress(Curso, 0.56f);
				const FVector2D AoLongo = (Adiante - NoRio).GetSafeNormal();
				const FVector2D DeLado(-AoLongo.Y, AoLongo.X);

				FGroundUsePatch Deck;
				Deck.Use = EGroundUse::Deck;
				Deck.HalfExtentUnits = Lote * DeckEmLotes;
				Deck.CenterUnits = NoRio + DeLado * (Meia + Deck.HalfExtentUnits);

				if (!IslandGeography::IsOnLand(Deck.CenterUnits))
				{
					continue;
				}

				Manchas.Add(Deck);
				++Postos;
			}
		}

		// OS BOSQUES, espalhados pelo anel habitável.
		const int32 QuantosBosques = WorldBudget::GroveCount(IslandGeography::IslandBiome());
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
		TArray<FGroundUsePatch> Tudo = TracarUsoDoSolo();

		// AS CLAREIRAS FECHADAS, por último e só onde nenhuma trilha passa
		// perto. Elas dependem das trilhas, e as trilhas não dependem delas —
		// por isso vêm depois, e não no meio.
		const float Longe = TrailLayout::HalfWidthUnits() * LongeDaTrilhaEmLarguras;

		const int32 QuantasClareiras =
			WorldBudget::HiddenClearingCount(IslandGeography::IslandBiome());
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
	return Uso == EGroundUse::Fazenda || Uso == EGroundUse::Criadouro
		|| Uso == EGroundUse::ClareiraFechada || Uso == EGroundUse::Loja
		|| Uso == EGroundUse::Acampamento || Uso == EGroundUse::Deck;
}

float LandUseLayout::PlantingDensityAt(const FVector2D& PositionUnits)
{
	switch (UseAt(PositionUnits))
	{
		case EGroundUse::Fazenda:
		case EGroundUse::Criadouro:
		case EGroundUse::ClareiraFechada:
		case EGroundUse::Loja:
		case EGroundUse::Acampamento:
		case EGroundUse::Deck:
			return 0.0f;

		// POMAR é mata PLANTADA: densidade alta, e é o que o distingue do
		// bosque no olho — árvore em fileira, não em tufo.
		case EGroundUse::Pomar:
		case EGroundUse::PomarSelvagem:
			return DensidadeDoPomar;

		case EGroundUse::Bosque:
			return DensidadeDoBosque;

		default:
			return 1.0f;
	}
}
