// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SettlementEconomy.h"
#include "World/VillageLayout.h"

namespace
{
	constexpr int32 PrecoBase = 100;

	/**
	 * O ágio da cidade grande.
	 *
	 * Ela cobra pela CONVENIÊNCIA de resolver tudo num lugar só. Quem tem
	 * pressa paga; quem tem tempo viaja e economiza — e é isso que faz as duas
	 * escolhas continuarem válidas no fim do jogo.
	 */
	constexpr int32 AgioDaCidade = 160;

	/** A cidade também compra pior: o mercado é de quem se especializou nele. */
	constexpr int32 CidadePagaPeloPet = 70;

	/**
	 * A cura na cidade é BARATA, não cara.
	 *
	 * É a exceção ao ágio, e ela existe para não fechar o laço de morte:
	 * "não tenho dinheiro para curar para poder lutar para ganhar dinheiro".
	 * O piso de verdade é a cura de graça em casa, que custa uma caminhada —
	 * preço pago em tempo, nunca em moeda.
	 */
	constexpr int32 CuraNaCidade = 25;

	/**
	 * As arenas das vilas pagam pouco e esgotam cedo: são o dinheiro do
	 * COMEÇO, quando a cidade ainda não se alcança. O prêmio grande mora na
	 * cidade, e é ele que abre a fronteira.
	 */
	constexpr int32 PremioDaVila = 30;

	bool TemPredio(ESettlementKind Tipo, EVillageBuilding Predio)
	{
		return VillageLayout::HasBuilding(VillageLayout::PlanFor(Tipo), Predio);
	}
}

bool SettlementEconomy::Offers(ESettlementKind Kind, ESettlementService Service)
{
	// A pergunta é respondida pelo TRAÇADO, não por uma segunda lista. Duas
	// listas concordam até a primeira edição (L-032), e a divergência
	// apareceria como um mercado que vende sem ter prédio.
	switch (Service)
	{
		case ESettlementService::Cura:
			return TemPredio(Kind, EVillageBuilding::CentroDeRecuperacao);

		case ESettlementService::Academia:
			return TemPredio(Kind, EVillageBuilding::Academia);

		case ESettlementService::Venda:
			return TemPredio(Kind, EVillageBuilding::Mercado);

		case ESettlementService::PremioDeRanking:
			return TemPredio(Kind, EVillageBuilding::Arena);
	}

	return false;
}

int32 SettlementEconomy::PricePercent(ESettlementKind Kind, ESettlementService Service)
{
	if (!Offers(Kind, Service))
	{
		return 0;
	}

	if (Service == ESettlementService::Cura)
	{
		// De graça em CASA, e para sempre. É o chão da economia: sem ele, o
		// jogador quebrado fica preso.
		return Kind == ESettlementKind::CidadeGrande ? CuraNaCidade : 0;
	}

	if (Service == ESettlementService::Academia)
	{
		return Kind == ESettlementKind::CidadeGrande ? AgioDaCidade : PrecoBase;
	}

	return 0;
}

int32 SettlementEconomy::PetBaseValue()
{
	return PrecoBase;
}

int32 SettlementEconomy::SalePayout(ESettlementKind Kind)
{
	return PetBaseValue() * PayoutPercent(Kind, ESettlementService::Venda) / 100;
}

int32 SettlementEconomy::RankingPrize(ESettlementKind Kind)
{
	return PrecoBase * PayoutPercent(Kind, ESettlementService::PremioDeRanking) / 100;
}

int32 SettlementEconomy::PayoutPercent(ESettlementKind Kind, ESettlementService Service)
{
	if (!Offers(Kind, Service))
	{
		return 0;
	}

	if (Service == ESettlementService::Venda)
	{
		return Kind == ESettlementKind::CidadeGrande ? CidadePagaPeloPet : PrecoBase;
	}

	if (Service == ESettlementService::PremioDeRanking)
	{
		return Kind == ESettlementKind::CidadeGrande ? PrecoBase : PremioDaVila;
	}

	return 0;
}
