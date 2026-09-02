// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldBudget.h"

namespace
{
	/**
	 * A tabela inteira num lugar só.
	 *
	 * Ler as colunas de cima a baixo tem de contar a história do bioma. Se uma
	 * linha não faz sentido lida assim, o número está errado — foi assim que
	 * "bosque no deserto" apareceu antes de existir código para plantá-lo.
	 */
	struct FOrcamento
	{
		float Agua = 0.06f;
		float MataDensa = 1.0f;

		int32 Bosques = 9;
		int32 Clareiras = 6;
		int32 Criadouros = 4;

		int32 FazendasPorVila = 2;
		int32 PomaresCuidados = 3;
		int32 PomaresSelvagens = 5;

		int32 Lojas = 4;
		int32 Acampamentos = 6;
		int32 Ruinas = 4;
		float GaleriaReta = 0.15f;
		int32 CemiteriosPorAssentamento = 1;
		int32 CemiteriosEsquecidos = 1;
	};

	FOrcamento Do(EIslandBiome Biome)
	{
		FOrcamento Orcamento;

		switch (Biome)
		{
		case EIslandBiome::Swamp:
			// Terra que não drena: muita água, mata fechada, e ninguém planta.
			Orcamento.Agua = 0.14f;
			Orcamento.MataDensa = 1.35f;
			Orcamento.FazendasPorVila = 1;
			Orcamento.PomaresCuidados = 1;
			Orcamento.PomaresSelvagens = 7;
			Orcamento.Acampamentos = 3;
			Orcamento.Ruinas = 6;
			Orcamento.GaleriaReta = 0.28f;
			Orcamento.CemiteriosEsquecidos = 2;
			break;

		case EIslandBiome::Desert:
			// Sem mata não há bosque nem pomar selvagem, e a lavoura vive da
			// pouca água que houver. Acampamento é o que sobra: no deserto se
			// atravessa, não se mora.
			Orcamento.Agua = 0.012f;
			Orcamento.MataDensa = 0.0f;
			Orcamento.Bosques = 0;
			Orcamento.Clareiras = 2;
			Orcamento.Criadouros = 2;
			Orcamento.FazendasPorVila = 1;
			Orcamento.PomaresCuidados = 1;
			Orcamento.PomaresSelvagens = 0;
			// Deserto guarda ruína: a areia cobre e devolve, e nada apodrece.
			Orcamento.Acampamentos = 9;
			Orcamento.Ruinas = 7;
			Orcamento.GaleriaReta = 0.20f;
			Orcamento.CemiteriosEsquecidos = 2;
			break;

		case EIslandBiome::Glacier:
			// Água tem, mas dura. Nada cresce, e criar pet no gelo é ofício de
			// poucos.
			Orcamento.Agua = 0.035f;
			Orcamento.MataDensa = 0.0f;
			Orcamento.Bosques = 0;
			Orcamento.Clareiras = 3;
			Orcamento.Criadouros = 1;
			Orcamento.FazendasPorVila = 0;
			Orcamento.PomaresCuidados = 0;
			Orcamento.PomaresSelvagens = 0;
			Orcamento.Lojas = 2;
			Orcamento.Acampamentos = 7;
			Orcamento.Ruinas = 2;
			Orcamento.GaleriaReta = 0.06f;
			break;

		case EIslandBiome::Volcano:
			// Seco por calor. O chão é fértil e a vida é curta: muita lavoura
			// por vila, nenhum pomar — pomar leva anos.
			Orcamento.Agua = 0.02f;
			Orcamento.MataDensa = 0.35f;
			Orcamento.Bosques = 2;
			Orcamento.Clareiras = 4;
			Orcamento.Criadouros = 2;
			Orcamento.FazendasPorVila = 3;
			Orcamento.PomaresCuidados = 0;
			Orcamento.PomaresSelvagens = 1;
			break;

		case EIslandBiome::Forest:
		case EIslandBiome::Beach:
		default:
			break;
		}

		return Orcamento;
	}
}

float WorldBudget::WaterCoverage(EIslandBiome Biome) { return Do(Biome).Agua; }
float WorldBudget::ForestDensity(EIslandBiome Biome) { return Do(Biome).MataDensa; }

int32 WorldBudget::GroveCount(EIslandBiome Biome) { return Do(Biome).Bosques; }
int32 WorldBudget::HiddenClearingCount(EIslandBiome Biome) { return Do(Biome).Clareiras; }
int32 WorldBudget::BreederCount(EIslandBiome Biome) { return Do(Biome).Criadouros; }

int32 WorldBudget::FarmsPerSettlement(EIslandBiome Biome) { return Do(Biome).FazendasPorVila; }
int32 WorldBudget::TendedOrchardCount(EIslandBiome Biome) { return Do(Biome).PomaresCuidados; }
int32 WorldBudget::WildOrchardCount(EIslandBiome Biome) { return Do(Biome).PomaresSelvagens; }

int32 WorldBudget::RoadsideShopCount(EIslandBiome Biome) { return Do(Biome).Lojas; }
int32 WorldBudget::CampCount(EIslandBiome Biome) { return Do(Biome).Acampamentos; }
int32 WorldBudget::RuinCount(EIslandBiome Biome) { return Do(Biome).Ruinas; }
float WorldBudget::StraightGalleryShare(EIslandBiome Biome) { return Do(Biome).GaleriaReta; }
int32 WorldBudget::GraveyardsPerSettlement(EIslandBiome Biome) { return Do(Biome).CemiteriosPorAssentamento; }
int32 WorldBudget::ForgottenGraveyardCount(EIslandBiome Biome) { return Do(Biome).CemiteriosEsquecidos; }
