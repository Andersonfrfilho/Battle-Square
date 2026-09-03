// Copyright 2026 Anderson. All Rights Reserved.

#include "World/LandUseLayout.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"
#include "World/TrailLayout.h"
#include "World/WorldBudget.h"
#include "Environment/IslandGeography.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandFeatureLayout.h"
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

	/** Quantos poços artesianos, o tamanho, e até onde o lençol acompanha o rio. */
	constexpr int32 QuantosPocos = 12;
	constexpr float PocoEmLotes = 0.12f;

	/** Templo e ruína, em lotes de vila. A ruína é menor: dela sobrou pouco. */
	constexpr float TemploEmLotes = 0.34f;
	constexpr float RuinaEmLotes = 0.22f;
	constexpr float CemiterioEmLotes = 0.30f;
	constexpr float AlcanceDoLencol = 0.16f;

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

	/**
	 * O MERCADO-NEGRO: o tamanho da mancha, e o quanto ele fica da trilha.
	 *
	 * Ao lado do caminho, e não sobre ele: quem vende o que não se declara
	 * precisa de freguês e não pode ser visto do caminho. Mais longe que o
	 * acampamento, que não tem o que esconder.
	 */
	constexpr float MercadoEmLotes = 0.20f;
	constexpr float MercadoDaTrilhaEmLarguras = 4.5f;

	/**
	 * "BEM ESPALHADOS" É MEDIÇÃO: nenhum par mais perto que esta fração do raio.
	 *
	 * Em fração do raio, e nunca em unidades, porque o número absoluto
	 * escolhido quando só existia um tamanho é a armadilha mais cara deste
	 * projeto. Se a ilha crescer, eles continuam espalhados sem ninguém mexer
	 * aqui.
	 */
	constexpr float EntreMercadosEmRaios = 0.55f;


	/** E longe da vila: mercado-negro com vizinho é mercado. */
	constexpr float LongeDaVilaEmClareiras = 3.0f;

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

			const float Clareira =
				VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind);
			const float Meia = Lote * FazendaEmLotes;

			// O RAIO é fixo — a fazenda fica encostada na vila, fora da
			// clareira. O que se ESCOLHE é a direção.
			const float Raio = Clareira + Meia;

			// A DIREÇÃO SE PROCURA, e não se calcula pelo rumo da água mais
			// perto.
			//
			// Antes era `(agua - vila).Normalize()`, e ela falha quando a água
			// está DENTRO da clareira: a fazenda avançava o raio inteiro,
			// ATRAVESSAVA o córrego e parava do outro lado, mais longe da água
			// do que a própria vila. Medido na vila-academia: água a 580, vila
			// a 580, fazenda a 1692.
			//
			// O rumo da água mais perto responde "para que lado fica a água",
			// que é outra pergunta. A que importa é "de todos os lugares onde
			// a fazenda CABE, qual é o mais perto da água" — e essa se
			// responde procurando.
			//
			// A varredura é determinística e grossa de propósito: 72 rumos, um
			// a cada cinco graus. Fino demais só troca a fazenda de lugar por
			// unidades que ninguém vê, e o traçado inteiro é reproduzível.
			constexpr int32 RumosDaBusca = 72;

			auto MelhorRumo = [&](const TArray<FVector2D>& JaUsados) -> FVector2D
			{
				FVector2D Escolhido = FVector2D(1.0f, 0.0f);
				float MenorDistancia = TNumericLimits<float>::Max();

				for (int32 Qual = 0; Qual < RumosDaBusca; ++Qual)
				{
					const float Angulo = (2.0f * PI * Qual) / RumosDaBusca;
					const FVector2D Rumo(FMath::Cos(Angulo), FMath::Sin(Angulo));
					const FVector2D Onde = Assentamento.CenterUnits + Rumo * Raio;

					// Longe das fazendas já postas desta vila: duas fazendas
					// na mesma beira seriam uma fazenda com duas etiquetas.
					bool bEncostada = false;
					for (const FVector2D& Outra : JaUsados)
					{
						if (FVector2D::Distance(Onde, Outra) < Meia * 2.0f)
						{
							bEncostada = true;
							break;
						}
					}

					if (bEncostada)
					{
						continue;
					}

					const float AteAAgua =
						FVector2D::Distance(Onde, AguaMaisPerto(Onde));

					if (AteAAgua < MenorDistancia)
					{
						MenorDistancia = AteAAgua;
						Escolhido = Rumo;
					}
				}

				return Escolhido;
			};

			const int32 FazendasPorVila =
				WorldBudget::FarmsPerSettlement(IslandGeography::IslandBiome());

			TArray<FVector2D> Postas;
			for (int32 Qual = 0; Qual < FazendasPorVila; ++Qual)
			{
				const FVector2D Rumo = MelhorRumo(Postas);

				FGroundUsePatch Fazenda;
				Fazenda.Use = EGroundUse::Fazenda;
				Fazenda.HalfExtentUnits = Meia;
				Fazenda.CenterUnits = Assentamento.CenterUnits + Rumo * Raio;

				Postas.Add(Fazenda.CenterUnits);
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

		// OS TEMPLOS, cada um NO LUGAR QUE O SEU DEUS GOVERNA.
		//
		// A posição é a identidade dele. O do monte fica na saia da montanha, o
		// da água na cachoeira, o do fogo na beira da rocha queimada, o do
		// fundo numa caverna, e o de Mãe Natureza no bosque mais fechado.
		//
		// Quem vê um templo sabe de quem ele é pelo lugar — e é assim que um
		// panteão se ensina sem uma linha de texto.
		{
			auto PorTemplo = [&Manchas, Lote](EDeity Quem, const FVector2D& Onde)
			{
				if (!IslandGeography::IsOnLand(Onde))
				{
					return;
				}

				FGroundUsePatch Templo;
				Templo.Use = EGroundUse::Templo;
				Templo.CenterUnits = Onde;
				Templo.HalfExtentUnits = Lote * TemploEmLotes;
				Templo.Deity = Quem;
				Manchas.Add(Templo);
			};

			// PEDRA, na saia do primeiro monte.
			for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
			{
				if (Peca.Feature != IslandFeatureLayout::EIslandFeature::WalkableMountain)
				{
					continue;
				}

				const FVector2D NoCume = Peca.CenterUnits();
				PorTemplo(EDeity::Pedra,
					NoCume - NoCume.GetSafeNormal() * (Peca.ClearanceUnits + Lote));
				break;
			}

			// CORRENTE, na margem da primeira cachoeira.
			for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
			{
				if (!Curso.HasFall())
				{
					continue;
				}

				const FVector2D NaQueda =
					FreshWater::PointAtProgress(Curso, Curso.FallAtProgress);

				// PERPENDICULAR ao rio, e depois EMPURRADO até sair da lâmina.
				//
				// É a terceira vez que erro isto do mesmo jeito: afastar na
				// direção RADIAL parece óbvio e não é, porque a direção radial
				// pode correr ao longo da água. E mesmo o perpendicular falha na
				// curva, onde o ponto mais próximo do rio não é o perpendicular.
				//
				// Conferir e empurrar é a única forma que funciona, e já está
				// escrita duas vezes neste mundo — na subida da cachoeira e na
				// trilha de beira-rio.
				const FVector2D Adiante =
					FreshWater::PointAtProgress(Curso, Curso.FallAtProgress + 0.04f);
				const FVector2D AoLongo = (Adiante - NaQueda).GetSafeNormal();
				FVector2D DeLado(-AoLongo.Y, AoLongo.X);

				if (FVector2D::DotProduct(DeLado, NaQueda.GetSafeNormal()) < 0.0f)
				{
					DeLado = -DeLado;
				}

				FVector2D NoTemplo = NaQueda + DeLado
					* (FreshWater::HalfWidthAtProgress(Curso, Curso.FallAtProgress) + Lote * 1.6f);

				for (int32 Empurrao = 0; Empurrao < 8; ++Empurrao)
				{
					float Aonde = 0.0f;
					const float Ate = FreshWater::NearestOn(Curso, NoTemplo, Aonde);

					if (Ate > FreshWater::HalfWidthAtProgress(Curso, Aonde) + Lote * 0.5f)
					{
						break;
					}

					NoTemplo += DeLado * Lote;
				}

				PorTemplo(EDeity::Corrente, NoTemplo);
				break;
			}

			// BRASEIRO, na BEIRA da rocha queimada — não dentro dela.
			//
			// Dentro seria templo que ninguém alcança: o chão ali queima, e a
			// trilha não atravessa. Um templo é um lugar de ir.
			{
				const FVector2D DoVulcao = IslandGeography::VolcanoCenterUnits();
				PorTemplo(EDeity::Braseiro, DoVulcao - DoVulcao.GetSafeNormal()
					* (IslandGeography::VolcanoScorchedRadiusUnits() + Lote * 2.0f));
			}

			// ABISMO, na primeira caverna. É o único templo que não se vê de
			// fora, e é justamente o ponto dele.
			for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
			{
				if (Peca.Feature != IslandFeatureLayout::EIslandFeature::Cave)
				{
					continue;
				}

				PorTemplo(EDeity::Abismo, Peca.CenterUnits());
				break;
			}

		}

		// AS RUÍNAS, escondidas.
		//
		// Elas são o oposto do templo em tudo o que importa: ficam longe de
		// trilha e de vila, e achá-las é acidente. E é delas que vem a única
		// coisa que nenhuma outra peça deste mundo dá: PASSADO. Um lugar que já
		// foi importante e deixou de ser conta uma história que ninguém
		// precisou escrever.
		{
			const int32 Quantas = WorldBudget::RuinCount(IslandGeography::IslandBiome());

			for (int32 Indice = 0; Indice < Quantas; ++Indice)
			{
				const uint32 Semente = BattleSpread::SeedFromText(
					FString::Printf(TEXT("ruina-de-templo-%d"), Indice));

				for (int32 Tentativa = 0; Tentativa < 16; ++Tentativa)
				{
					const float Rumo = BattleSpread::Between(0.0f, 360.0f,
						BattleSpread::Fraction(Semente, Tentativa));
					const float Anel = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
						0.20f, 0.82f, BattleSpread::Fraction(Semente, Tentativa + 40));

					const FVector2D Onde = DoRumo(Rumo, Anel);

					if (!IslandGeography::IsOnLand(Onde) || TrailLayout::IsOnTrail(Onde))
					{
						continue;
					}

					// Longe de trilha E de vila: perto de qualquer uma das duas
					// ela deixa de ser esquecida, que é a única coisa que ela é.
					bool bPertoDeGente = false;
					for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
					{
						if (FVector2D::Distance(Onde, Assentamento.CenterUnits)
							< VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 3.0f)
						{
							bPertoDeGente = true;
							break;
						}
					}

					if (bPertoDeGente)
					{
						continue;
					}

					FGroundUsePatch Ruina;
					Ruina.Use = EGroundUse::Ruina;
					Ruina.CenterUnits = Onde;
					Ruina.HalfExtentUnits = Lote * RuinaEmLotes;
					Ruina.Deity = static_cast<EDeity>(
						BattleSpread::Below(Semente, 60, Pantheon::Count()));

					Manchas.Add(Ruina);
					break;
				}
			}
		}

		// OS CEMITÉRIOS DA VILA, e por que eles ficam do lado contrário da água.
		//
		// Não é superstição, é saneamento: ninguém enterra rio ACIMA de onde
		// bebe, e vilas fazem isso desde muito antes de saberem por quê. Aqui a
		// regra vira uma amarra que o gerador consegue verificar — o cemitério
		// sai na direção OPOSTA à da água mais perto.
		//
		// E o que ele diz a quem chega é curto: quem mora aqui já morreu aqui.
		// Uma vila sem cemitério é um acampamento.
		{
			const int32 Quantos = WorldBudget::GraveyardsPerSettlement(
				IslandGeography::IslandBiome());

			const TArray<FSettlementPlacement> Assentamentos = RegionLayout::Plan();

			for (int32 Qual = 0; Qual < Assentamentos.Num(); ++Qual)
			{
				const FSettlementPlacement& Assentamento = Assentamentos[Qual];
				const float Clareira =
					VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind);

				// DE ONDE VEM A ÁGUA desta vila. Sem achar nenhuma, não há lado
				// oposto a respeitar, e o cemitério não teria regra — então ele
				// não nasce, em vez de nascer em lugar arbitrário.
				FVector2D DaAgua = FVector2D::ZeroVector;
				float MaisPerto = TNumericLimits<float>::Max();

				for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
				{
					if (Curso.PointsUnits.Num() < 2)
					{
						continue;
					}

					float Aonde = 0.0f;
					const float Ate = FreshWater::NearestOn(
						Curso, Assentamento.CenterUnits, Aonde);

					if (Ate < MaisPerto)
					{
						MaisPerto = Ate;
						DaAgua = FreshWater::PointAtProgress(Curso, Aonde);
					}
				}

				if (MaisPerto == TNumericLimits<float>::Max())
				{
					continue;
				}

				const FVector2D ParaLongeDaAgua =
					(Assentamento.CenterUnits - DaAgua).GetSafeNormal();

				if (ParaLongeDaAgua.IsNearlyZero())
				{
					continue;
				}

				for (int32 Indice = 0; Indice < Quantos; ++Indice)
				{
					const uint32 Semente = BattleSpread::SeedFromText(FString::Printf(
						TEXT("cemiterio-%d-%d"), Qual, Indice));

					// PERTO, mas fora da clareira: cemitério dentro da praça é
					// praça, e do outro lado da ilha é romaria.
					const float Quanto = Clareira * BattleSpread::Between(
						1.4f, 2.1f, BattleSpread::Fraction(Semente, 0));

					// Um desvio pequeno em volta do rumo oposto, para dois
					// cemitérios da mesma vila não caírem um sobre o outro.
					const float Desvio = BattleSpread::Between(-34.0f, 34.0f,
						BattleSpread::Fraction(Semente, 1));

					const FVector2D Rumo = ParaLongeDaAgua.GetRotated(Desvio);
					const FVector2D Onde = Assentamento.CenterUnits + Rumo * Quanto;

					if (!IslandGeography::IsOnLand(Onde))
					{
						continue;
					}

					FGroundUsePatch Cemiterio;
					Cemiterio.Use = EGroundUse::Cemiterio;
					Cemiterio.CenterUnits = Onde;
					Cemiterio.HalfExtentUnits = Lote * CemiterioEmLotes;
					Manchas.Add(Cemiterio);
				}
			}
		}

		// E OS CEMITÉRIOS ESQUECIDOS, que não são os outros em tamanho menor.
		//
		// O da vila é serviço; este é VESTÍGIO — alguém foi enterrado onde não
		// há mais vila nenhuma. É o único que carrega história em vez de
		// função, e por isso ele nasce ao lado de uma RUÍNA: um templo caído
		// com um cemitério junto conta que ali houve gente, e conta melhor do
		// que qualquer placa contaria.
		{
			TArray<int32> Ruinas;
			for (int32 Indice = 0; Indice < Manchas.Num(); ++Indice)
			{
				if (Manchas[Indice].Use == EGroundUse::Ruina)
				{
					Ruinas.Add(Indice);
				}
			}

			const int32 Quantos = FMath::Min(Ruinas.Num(),
				WorldBudget::ForgottenGraveyardCount(IslandGeography::IslandBiome()));

			for (int32 Indice = 0; Indice < Quantos; ++Indice)
			{
				const FGroundUsePatch& Ruina = Manchas[Ruinas[Indice]];

				const uint32 Semente = BattleSpread::SeedFromText(
					FString::Printf(TEXT("cemiterio-esquecido-%d"), Indice));

				const float Rumo = BattleSpread::Between(0.0f, 360.0f,
					BattleSpread::Fraction(Semente, 0));

				// ENCOSTADO na ruína, não dentro dela: um cemitério que ocupa o
				// mesmo chão do templo apaga os dois.
				const FVector2D Onde = Ruina.CenterUnits
					+ DoRumo(Rumo, Ruina.HalfExtentUnits + Lote * CemiterioEmLotes * 1.3f);

				if (!IslandGeography::IsOnLand(Onde))
				{
					continue;
				}

				FGroundUsePatch Cemiterio;
				Cemiterio.Use = EGroundUse::CemiterioEsquecido;
				Cemiterio.CenterUnits = Onde;
				Cemiterio.HalfExtentUnits = Lote * CemiterioEmLotes * 0.8f;
				Cemiterio.Deity = Ruina.Deity;
				Manchas.Add(Cemiterio);
			}
		}

		// OS POÇOS ARTESIANOS, e a chance de cada um dar água.
		//
		// A chance NÃO é um sorteio no vácuo: ela sai de quão fundo está o
		// lençol, e o lençol acompanha duas coisas que já existem — a distância
		// da água de superfície e a altura do chão. Baixio perto do rio quase
		// sempre dá; alto e longe quase nunca.
		//
		// O sorteio decide só o caso duvidoso, e é determinístico: o mesmo poço
		// dá o mesmo resultado sempre. Poço que muda de resposta entre visitas
		// não é aposta, é bug.
		{
			const int32 Quantos = QuantosPocos;

			for (int32 Indice = 0; Indice < Quantos; ++Indice)
			{
				const uint32 Semente = BattleSpread::SeedFromText(
					FString::Printf(TEXT("poco-artesiano-%d"), Indice));

				const float Rumo = BattleSpread::Between(0.0f, 360.0f,
					BattleSpread::Fraction(Semente, 0));
				const float Anel = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
					0.10f, 0.80f, BattleSpread::Fraction(Semente, 1));

				const FVector2D Onde = DoRumo(Rumo, Anel);
				if (!IslandGeography::IsOnLand(Onde))
				{
					continue;
				}

				float AteAAgua = TNumericLimits<float>::Max();
				for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
				{
					float Aonde = 0.0f;
					AteAAgua = FMath::Min(AteAAgua,
						FreshWater::NearestOn(Curso, Onde, Aonde));
				}

				// Duas parcelas, e as duas contam contra: longe da água o
				// lençol é fundo, e alto do chão ele é mais fundo ainda.
				const float PelaDistancia = FMath::Clamp(
					1.0f - AteAAgua / (IslandGeography::LandRadiusUnits() * AlcanceDoLencol),
					0.0f, 1.0f);

				const float PelaAltura = FMath::Clamp(
					1.0f - IslandGeography::BedrockHeightAt(Onde)
						/ FMath::Max(1.0f, IslandGeography::LandRadiusUnits() * 0.03f),
					0.0f, 1.0f);

				const float Chance = FMath::Clamp(
					PelaDistancia * 0.65f + PelaAltura * 0.35f, 0.05f, 0.95f);

				FGroundUsePatch Poco;
				Poco.Use = EGroundUse::Poco;
				Poco.CenterUnits = Onde;
				Poco.HalfExtentUnits = Lote * PocoEmLotes;
				Poco.bYieldsWater = BattleSpread::Fraction(Semente, 2) < Chance;

				Manchas.Add(Poco);
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

		// O TEMPLO DE MÃE NATUREZA, no bosque mais fechado.
		//
		// Ele vem DEPOIS dos bosques, e essa ordem é a regra: o templo dela não
		// escolhe um lugar, ele RECONHECE um — onde a mata está mais viva. Posto
		// antes, ele procurava num mundo sem bosque nenhum e simplesmente não
		// nascia. Quatro templos apareceram no mapa em vez de cinco, e só a
		// contagem mostrou.
		{
			const FGroundUsePatch* MaisFechado = nullptr;
			for (const FGroundUsePatch& Mancha : Manchas)
			{
				if (Mancha.Use != EGroundUse::Bosque)
				{
					continue;
				}

				if (!MaisFechado || Mancha.HalfExtentUnits > MaisFechado->HalfExtentUnits)
				{
					MaisFechado = &Mancha;
				}
			}

			// DENTRO do bosque, e não no lugar dele.
			//
			// No centro exato o templo limpava o coração da mata — e o coração
			// é justamente o que Mãe Natureza governa. Um templo na floresta é
			// uma clareira DENTRO dela, não no lugar dela.
			const FVector2D NoBosque = MaisFechado
				? MaisFechado->CenterUnits
					+ DoRumo(37.0f, MaisFechado->HalfExtentUnits * 0.62f)
				: FVector2D::ZeroVector;

			if (MaisFechado && IslandGeography::IsOnLand(NoBosque))
			{
				FGroundUsePatch Templo;
				Templo.Use = EGroundUse::Templo;
				Templo.CenterUnits = NoBosque;
				Templo.HalfExtentUnits = Lote * TemploEmLotes;
				Templo.Deity = EDeity::MaeNatureza;
				Manchas.Add(Templo);
			}
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

		// OS MERCADOS-NEGROS, e "bem espalhados" é medição.
		//
		// Eles vêm por último pelo mesmo motivo das clareiras: dependem das
		// trilhas, e as trilhas não dependem deles.
		//
		// A ESCOLHA É POR AFASTAMENTO, não por sorteio. Sortear três pontos de
		// trilha e torcer para caírem longe é medir a sorte: com trilhas que
		// convergem nas vilas, dois vizinhos saem com frequência — e vizinhos é
		// exatamente o defeito que "bem espalhados" existe para impedir.
		//
		// Cada um é o candidato MAIS LONGE do que já foi posto (amostragem de
		// ponto mais distante). O primeiro é o mais longe de qualquer vila, que
		// é o mais escondido que a trilha oferece.
		{
			const int32 Quantos = FMath::Max(WorldBudget::BlackMarketFloor(),
				WorldBudget::BlackMarketCount(IslandGeography::IslandBiome()));

			const TArray<FSettlementPlacement>& Vilas = RegionLayout::Plan();

			TArray<FVector2D> Candidatos;
			for (const FTrailRoute& Trilha : TrailLayout::Plan())
			{
				for (const FVector2D& NoCaminho : Trilha.PointsUnits)
				{
					bool bPertoDeVila = false;
					for (const FSettlementPlacement& Vila : Vilas)
					{
						if (FVector2D::Distance(NoCaminho, Vila.CenterUnits)
							< VillageLayout::ClearingHalfExtentUnitsFor(Vila.Kind)
								* LongeDaVilaEmClareiras)
						{
							bPertoDeVila = true;
							break;
						}
					}

					if (!bPertoDeVila)
					{
						Candidatos.Add(NoCaminho);
					}
				}
			}

			TArray<FVector2D> Postos;
			while (Postos.Num() < Quantos && Candidatos.Num() > 0)
			{
				int32 Melhor = INDEX_NONE;
				float Maior = -1.0f;

				for (int32 Qual = 0; Qual < Candidatos.Num(); ++Qual)
				{
					float Perto = TNumericLimits<float>::Max();

					if (Postos.Num() == 0)
					{
						for (const FSettlementPlacement& Vila : Vilas)
						{
							Perto = FMath::Min(Perto, static_cast<float>(
								FVector2D::Distance(Candidatos[Qual], Vila.CenterUnits)));
						}
					}
					else
					{
						for (const FVector2D& Posto : Postos)
						{
							Perto = FMath::Min(Perto, static_cast<float>(
								FVector2D::Distance(Candidatos[Qual], Posto)));
						}
					}

					if (Perto > Maior)
					{
						Maior = Perto;
						Melhor = Qual;
					}
				}

				if (Melhor == INDEX_NONE)
				{
					break;
				}

				const FVector2D NoCaminho = Candidatos[Melhor];
				Candidatos.RemoveAt(Melhor);

				// O rumo do desvio sai da GEOMETRIA do ponto, nunca do índice
				// do laço: assim o mesmo mercado fica no mesmo lugar se alguém
				// reordenar o código (regra 5 da geração procedural).
				const uint32 Semente = BattleSpread::SeedFromText(FString::Printf(
					TEXT("mercado-negro-%d-%d"),
					FMath::RoundToInt(NoCaminho.X), FMath::RoundToInt(NoCaminho.Y)));

				// A REGRA VALE ONDE O MERCADO FICA, não onde a trilha passa.
				//
				// Foi assim que ele nasceu encostado numa vila: o ponto de
				// trilha estava longe, e o desvio de quatro larguras e meia o
				// trouxe de volta. Filtrar a candidata e não conferir o lugar
				// final é conferir o passo anterior ao que importa.
				constexpr int32 RumosDoDesvio = 8;
				for (int32 Tentativa = 0; Tentativa < RumosDoDesvio; ++Tentativa)
				{
					const float Rumo = BattleSpread::Between(0.0f, 360.0f,
						BattleSpread::Fraction(Semente, Tentativa));

					const FVector2D Onde = NoCaminho + DoRumo(Rumo,
						TrailLayout::HalfWidthUnits() * MercadoDaTrilhaEmLarguras);

					if (!IslandGeography::IsOnLand(Onde))
					{
						continue;
					}

					bool bPertoDeVila = false;
					for (const FSettlementPlacement& Vila : Vilas)
					{
						if (FVector2D::Distance(Onde, Vila.CenterUnits)
							< VillageLayout::ClearingHalfExtentUnitsFor(Vila.Kind)
								* LongeDaVilaEmClareiras)
						{
							bPertoDeVila = true;
							break;
						}
					}

					if (bPertoDeVila)
					{
						continue;
					}

					// E o afastamento é EXIGIDO aqui, não só medido no teste.
					// Limiar que só existe no teste é limiar que o gerador
					// pode furar — e ele furaria calado.
					bool bVizinhoDeMercado = false;
					for (const FVector2D& Posto : Postos)
					{
						if (FVector2D::Distance(Onde, Posto)
							< LandUseLayout::BlackMarketSpreadUnits())
						{
							bVizinhoDeMercado = true;
							break;
						}
					}

					if (bVizinhoDeMercado)
					{
						continue;
					}

					Postos.Add(Onde);

					FGroundUsePatch Mercado;
					Mercado.Use = EGroundUse::MercadoNegro;
					Mercado.CenterUnits = Onde;
					Mercado.HalfExtentUnits =
						VillageLayout::PlotHalfExtentUnits() * MercadoEmLotes;
					Tudo.Add(Mercado);
					break;
				}
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

float LandUseLayout::BlackMarketSpreadUnits()
{
	return IslandGeography::LandRadiusUnits() * EntreMercadosEmRaios;
}

bool LandUseLayout::BlocksPlanting(const FVector2D& PositionUnits)
{
	const EGroundUse Uso = UseAt(PositionUnits);
	return Uso == EGroundUse::Fazenda || Uso == EGroundUse::Criadouro
		|| Uso == EGroundUse::ClareiraFechada || Uso == EGroundUse::Loja
		|| Uso == EGroundUse::Acampamento || Uso == EGroundUse::Deck || Uso == EGroundUse::Poco || Uso == EGroundUse::Templo;
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
		case EGroundUse::Poco:
		case EGroundUse::Templo:
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
