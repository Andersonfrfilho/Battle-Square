// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/RegionResidency.h"

#include "Environment/IslandGeography.h"
#include "World/WorldDiscovery.h"

namespace ResidenciaDeRegiao
{
	/** Números primos grandes, para coordenadas vizinhas não darem sementes vizinhas. */
	constexpr uint32 EmbaralhaColuna = 73856093u;
	constexpr uint32 EmbaralhaLinha = 19349663u;
}

float RegionResidency::ChunkSideUnits()
{
	// ABSOLUTO, e desacoplado da região de propósito.
	//
	// Bloco é granularidade de STREAMING: o que ele limita é quanto do mundo
	// está vivo de uma vez, e isso é decidido por desempenho, não pelo tamanho
	// da ilha. Região é resolução de MEMÓRIA e de mapa, e essa acompanha a
	// ilha. Transmite-se fino e lembra-se grosso.
	//
	// Eram amarrados — bloco = 8 regiões — e ao tornar a região proporcional o
	// bloco viraria 320 metros numa ilha de 1 km, com os nove vivos cobrindo
	// quase a ilha inteira. Aí o streaming deixaria de streamar.
	return ChunkSideDefaultUnits;
}

FIntPoint RegionResidency::ChunkAt(const FVector2D& PositionUnits)
{
	const float Lado = ChunkSideUnits();

	// Piso, e não truncamento: truncar faz -1 e +1 caírem no mesmo pedaço, e a
	// costura ficaria com o dobro da largura em cima do eixo.
	return FIntPoint(
		FMath::FloorToInt(PositionUnits.X / Lado),
		FMath::FloorToInt(PositionUnits.Y / Lado));
}

FVector2D RegionResidency::ChunkCenterUnits(const FIntPoint& Chunk)
{
	const float Lado = ChunkSideUnits();
	return FVector2D(
		(static_cast<float>(Chunk.X) + 0.5f) * Lado,
		(static_cast<float>(Chunk.Y) + 0.5f) * Lado);
}

bool RegionResidency::TouchesLand(const FIntPoint& Chunk)
{
	const float Lado = ChunkSideUnits();
	const float Esquerda = static_cast<float>(Chunk.X) * Lado;
	const float Baixo = static_cast<float>(Chunk.Y) * Lado;

	// O ponto do retângulo MAIS PERTO do centro da ilha, e não uma amostragem
	// de cantos. Amostrar erra justamente onde importa: a ponta da ilha que
	// entra por um lado do pedaço sem alcançar canto nenhum sairia sem mata, e
	// o jogador andaria até a praia por cima de nada.
	const FVector2D MaisPerto(
		FMath::Clamp(0.0f, Esquerda, Esquerda + Lado),
		FMath::Clamp(0.0f, Baixo, Baixo + Lado));

	return IslandGeography::IsOnLand(MaisPerto);
}

uint32 RegionResidency::ChunkSeed(uint32 WorldSeed, const FIntPoint& Chunk)
{
	uint32 Semente = WorldSeed;
	Semente = HashCombine(Semente, static_cast<uint32>(Chunk.X) * ResidenciaDeRegiao::EmbaralhaColuna);
	Semente = HashCombine(Semente, static_cast<uint32>(Chunk.Y) * ResidenciaDeRegiao::EmbaralhaLinha);
	return Semente;
}

RegionResidency::FResidencyChange RegionResidency::PlanChange(
	const TSet<FIntPoint>& LiveChunks, const FVector2D& PlayerPositionUnits)
{
	const FIntPoint Onde = ChunkAt(PlayerPositionUnits);

	TSet<FIntPoint> Devidos;
	for (int32 Coluna = -ResidentRadiusInChunks; Coluna <= ResidentRadiusInChunks; ++Coluna)
	{
		for (int32 Linha = -ResidentRadiusInChunks; Linha <= ResidentRadiusInChunks; ++Linha)
		{
			const FIntPoint Candidato(Onde.X + Coluna, Onde.Y + Linha);
			if (TouchesLand(Candidato))
			{
				Devidos.Add(Candidato);
			}
		}
	}

	FResidencyChange Mudanca;

	for (const FIntPoint& Devido : Devidos)
	{
		if (!LiveChunks.Contains(Devido))
		{
			Mudanca.ToBuild.Add(Devido);
		}
	}

	for (const FIntPoint& Vivo : LiveChunks)
	{
		if (!Devidos.Contains(Vivo))
		{
			Mudanca.ToDrop.Add(Vivo);
		}
	}

	// Ordem estável: a de um `TSet` depende da tabela de dispersão, e um plano
	// que muda de ordem entre execuções torna qualquer investigação de "por
	// que este pedaço nasceu antes daquele" uma leitura de sorte.
	const auto PorCoordenada = [](const FIntPoint& Uma, const FIntPoint& Outra)
	{
		return Uma.X != Outra.X ? Uma.X < Outra.X : Uma.Y < Outra.Y;
	};
	Mudanca.ToBuild.Sort(PorCoordenada);
	Mudanca.ToDrop.Sort(PorCoordenada);

	return Mudanca;
}
