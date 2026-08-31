// Copyright 2026 Anderson. All Rights Reserved.

#include "World/VillageLayout.h"
#include "Environment/RegionResidency.h"
#include "World/RegionLayout.h"

namespace
{
	/**
	 * A vila ocupa cerca de um TERÇO do bloco.
	 *
	 * Sobra floresta em volta, que é o resto do bloco 0,0 — e é ela que faz a
	 * vila ser um lugar DENTRO de um mundo, em vez de o mundo inteiro.
	 */
	constexpr float FracaoDoBloco = 0.34f;

	/**
	 * A folga entre o último prédio e a primeira árvore.
	 *
	 * Uma vila com mata encostada na parede parece engolida; com folga, ela
	 * parece aberta na mata — que é o que uma clareira é.
	 */
	constexpr float FolgaDaClareiraUnidades = 260.0f;

	/** Meio-lado de um prédio comum. */
	constexpr float MeioLadoDaCasa = 200.0f;

	/** Os que importam são MAIORES: tamanho é hierarquia sem precisar de placa. */
	constexpr float MeioLadoDoCentro = 260.0f;
	constexpr float MeioLadoDaArena = 270.0f;

	/** A praça é chata e larga: ela é chão, não prédio. */
	constexpr float MeioLadoDaPraca = 250.0f;
	constexpr float AlturaDaPraca = 20.0f;

	constexpr float AlturaDaCasa = 380.0f;
	constexpr float AlturaDoCentro = 520.0f;
	constexpr float AlturaDaArena = 460.0f;
	constexpr float AlturaDoMarco = 300.0f;

	/**
	 * O lote muda de tamanho com o tipo.
	 *
	 * A cidade grande precisa caber a arena da região ao lado de tudo o que as
	 * vilas têm; o posto de fronteira é um portão e duas guaritas. Um lote só
	 * para os dois deixaria a cidade apertada ou o posto vazio.
	 */
	constexpr float EscalaDaVila = 1.0f;
	constexpr float EscalaDaCidade = 1.7f;
	// 0.42 não cabia: as guaritas ficavam com a parede de fora do lote. O
// número saiu de medir a soma real (portão + guarita + folga), não de
// escolher uma fração bonita.
constexpr float EscalaDoPosto = 0.55f;

	FVillagePlacement Por(EVillageBuilding Predio, float X, float Y,
		float MeioLado, float Altura)
	{
		FVillagePlacement Peca;
		Peca.Building = Predio;
		Peca.OffsetUnits = FVector2D(X, Y);
		Peca.HalfExtentUnits = FVector2D(MeioLado, MeioLado);
		Peca.HeightUnits = Altura;
		return Peca;
	}
}

float VillageLayout::PlotHalfExtentUnits()
{
	return RegionResidency::ChunkSideUnits() * FracaoDoBloco * 0.5f;
}

float VillageLayout::PlotHalfExtentUnitsFor(ESettlementKind Kind)
{
	const float Base = PlotHalfExtentUnits();

	switch (Kind)
	{
		case ESettlementKind::CidadeGrande:     return Base * EscalaDaCidade;
		case ESettlementKind::PostoDeFronteira: return Base * EscalaDoPosto;
		default:                                return Base * EscalaDaVila;
	}
}

bool VillageLayout::FitsInPlotFor(ESettlementKind Kind, const FVillagePlacement& Placement)
{
	const float Metade = PlotHalfExtentUnitsFor(Kind);
	return FMath::Abs(Placement.OffsetUnits.X) + Placement.HalfExtentUnits.X <= Metade
		&& FMath::Abs(Placement.OffsetUnits.Y) + Placement.HalfExtentUnits.Y <= Metade;
}

float VillageLayout::ClearingHalfExtentUnitsFor(ESettlementKind Kind)
{
	return PlotHalfExtentUnitsFor(Kind) + FolgaDaClareiraUnidades;
}

TArray<FVillagePlacement> VillageLayout::PlanFor(ESettlementKind Kind)
{
	// A vila inicial É o traçado base: ela foi desenhada primeiro, e os outros
	// tipos se descrevem como diferenças dela. Assim "o que muda de uma vila
	// para a outra" fica legível no código em vez de espalhado em quatro
	// funções que se parecem.
	TArray<FVillagePlacement> Pecas = Plan();

	switch (Kind)
	{
		case ESettlementKind::VilaInicial:
			// Sem academia, e é a decisão mais importante daqui.
			break;

		case ESettlementKind::VilaDaAcademia:
			// A academia toma o lugar da Escola: a escola é de casa, e ter as
			// duas aqui apagaria o motivo de voltar à vila inicial.
			Pecas.RemoveAll([](const FVillagePlacement& Peca)
				{ return Peca.Building == EVillageBuilding::Escola; });
			Pecas.Add(Por(EVillageBuilding::Academia,
				700.0f, 0.0f, MeioLadoDoCentro, AlturaDoCentro));
			break;

		case ESettlementKind::VilaDoMercado:
			Pecas.RemoveAll([](const FVillagePlacement& Peca)
				{ return Peca.Building == EVillageBuilding::Escola; });
			Pecas.Add(Por(EVillageBuilding::Mercado,
				700.0f, 0.0f, MeioLadoDoCentro, AlturaDoCentro));
			break;

		case ESettlementKind::CidadeGrande:
			// A cidade tem TUDO — é o que a spec exige para a região ser
			// autossuficiente: quem nasce aqui não visita outra região antes
			// de vencer esta.
			Pecas.Add(Por(EVillageBuilding::Academia,
				-1500.0f, -700.0f, MeioLadoDoCentro, AlturaDoCentro));
			Pecas.Add(Por(EVillageBuilding::Mercado,
				1500.0f, -700.0f, MeioLadoDoCentro, AlturaDoCentro));
			Pecas.Add(Por(EVillageBuilding::Casa, -1500.0f, 700.0f, MeioLadoDaCasa, AlturaDaCasa));
			Pecas.Add(Por(EVillageBuilding::Casa, 1500.0f, 700.0f, MeioLadoDaCasa, AlturaDaCasa));
			break;

		case ESettlementKind::PostoDeFronteira:
			// O posto não é vila: é uma porta. Tudo o que faz uma vila ser
			// lugar de ficar sai fora, e sobra o portão com duas guaritas.
			Pecas.Empty();
			Pecas.Add(Por(EVillageBuilding::Portao, 0.0f, 0.0f, 300.0f, 560.0f));
			Pecas.Add(Por(EVillageBuilding::Casa, -460.0f, 0.0f, 110.0f, 300.0f));
			Pecas.Add(Por(EVillageBuilding::Casa, 460.0f, 0.0f, 110.0f, 300.0f));
			break;
	}

	return Pecas;
}

TArray<FVillagePlacement> VillageLayout::Plan()
{
	TArray<FVillagePlacement> Pecas;

	// A PRAÇA no meio. O poste de anúncios vive nela, e ele precisa ser a
	// primeira coisa que se encontra ao entrar — um poste num canto é um poste
	// que ninguém lê.
	Pecas.Add(Por(EVillageBuilding::Praca, 0.0f, 0.0f, MeioLadoDaPraca, AlturaDaPraca));

	// Os dois que TIRAM REGRA DO CONSOLE ficam de frente para a praça, um de
	// cada lado. São a razão de a vila existir; escondê-los atrás de casas
	// repetiria o defeito que eles vieram consertar.
	Pecas.Add(Por(EVillageBuilding::CentroDeRecuperacao,
		-700.0f, 0.0f, MeioLadoDoCentro, AlturaDoCentro));
	Pecas.Add(Por(EVillageBuilding::Escola,
		700.0f, 0.0f, MeioLadoDaCasa, AlturaDaCasa));

	// A ARENA ao norte, afastada: ela é o motivo de VOLTAR, não de ficar. E
	// atrás dela não passa trilha — quem vai à arena vai de propósito.
	Pecas.Add(Por(EVillageBuilding::Arena, 0.0f, -780.0f, MeioLadoDaArena, AlturaDaArena));

	// O MARCO ao sul, na boca da trilha que sai para a mata: é o último ponto
	// da vila para quem parte, e o primeiro para quem volta.
	Pecas.Add(Por(EVillageBuilding::Marco, 0.0f, 820.0f, 100.0f, AlturaDoMarco));

	// CASAS nas quinas, fechando o quadro. Elas não têm função; existem para a
	// vila se ler de fora, e é por isso que ficam na BORDA — quem chega pela
	// mata vê telhado antes de ver praça.
	Pecas.Add(Por(EVillageBuilding::Casa, -650.0f, -650.0f, MeioLadoDaCasa, AlturaDaCasa));
	Pecas.Add(Por(EVillageBuilding::Casa, 650.0f, -650.0f, MeioLadoDaCasa, AlturaDaCasa));
	Pecas.Add(Por(EVillageBuilding::Casa, -650.0f, 650.0f, MeioLadoDaCasa, AlturaDaCasa));
	Pecas.Add(Por(EVillageBuilding::Casa, 650.0f, 650.0f, MeioLadoDaCasa, AlturaDaCasa));

	return Pecas;
}

bool VillageLayout::HasBuilding(const TArray<FVillagePlacement>& Placements,
	EVillageBuilding Building)
{
	return Placements.ContainsByPredicate(
		[Building](const FVillagePlacement& Peca) { return Peca.Building == Building; });
}

bool VillageLayout::Overlaps(const FVillagePlacement& First, const FVillagePlacement& Second)
{
	const FVector2D Distancia(
		FMath::Abs(First.OffsetUnits.X - Second.OffsetUnits.X),
		FMath::Abs(First.OffsetUnits.Y - Second.OffsetUnits.Y));

	return Distancia.X < (First.HalfExtentUnits.X + Second.HalfExtentUnits.X)
		&& Distancia.Y < (First.HalfExtentUnits.Y + Second.HalfExtentUnits.Y);
}

bool VillageLayout::FitsInPlot(const FVillagePlacement& Placement)
{
	const float Metade = PlotHalfExtentUnits();
	return FMath::Abs(Placement.OffsetUnits.X) + Placement.HalfExtentUnits.X <= Metade
		&& FMath::Abs(Placement.OffsetUnits.Y) + Placement.HalfExtentUnits.Y <= Metade;
}

float VillageLayout::ClearingHalfExtentUnits()
{
	return PlotHalfExtentUnits() + FolgaDaClareiraUnidades;
}

bool VillageLayout::BlocksPlanting(const FVector2D& WorldXY)
{
	// Agora são VÁRIOS assentamentos, e é por isso que esta função sempre
	// recebeu posição de mundo em vez de um deslocamento: a mata é plantada
	// por pedaço, e cada pedaço só sabe onde ELE está.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		const float Metade = ClearingHalfExtentUnitsFor(Assentamento.Kind);
		if (FMath::Abs(WorldXY.X - Assentamento.CenterUnits.X) <= Metade
			&& FMath::Abs(WorldXY.Y - Assentamento.CenterUnits.Y) <= Metade)
		{
			return true;
		}
	}

	return false;
}
