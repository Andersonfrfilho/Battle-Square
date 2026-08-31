// Copyright 2026 Anderson. All Rights Reserved.

#include "World/RegionLayout.h"
#include "Environment/IslandGeography.h"

namespace
{
	/**
	 * As distâncias, em fração do raio da terra.
	 *
	 * Com o raio de 1,4 km elas dão cerca de 400 m, 700 m e 950 m — o que a
	 * spec pediu. Guardadas como fração, e não como metros, elas continuam
	 * dando o mesmo TEMPO de caminhada quando a ilha mudar de tamanho.
	 */
	constexpr float FracaoDasVilas = 0.29f;
	constexpr float FracaoDaCidade = 0.50f;
	/**
	 * Os postos vão para a COSTA, e não a 0.68 do raio como eu tinha posto.
	 *
	 * A ilha é redonda: não existe fronteira de terra. A fronteira é a praia,
	 * e o que se atravessa é água — a ilha é a unidade de servidor, e sair
	 * dela é embarcar. Um posto no meio do mato guardava uma linha que não
	 * existe.
	 *
	 * Ele fica logo ATRÁS da faixa de praia: em cima da areia, a maré e a
	 * rampa da orla o deixariam meio enterrado.
	 */
	constexpr float FracaoDaOrlaDoPosto = 0.5f;

	/**
	 * Os rumos. Nenhum deles é 180°, e isso não é gosto: 180° é onde o vulcão
	 * mora, e o chão em volta da cratera é rocha queimada.
	 */
	constexpr float RumoDaAcademia = 55.0f;
	constexpr float RumoDoMercado = 235.0f;
	constexpr float RumoDaCidade = 320.0f;

	constexpr int32 QuantidadeDePostos = 3;
	constexpr float PrimeiroRumoDePosto = 15.0f;

	FVector2D PontoDaIlha(float Graus, float Distancia)
	{
		const float Radianos = FMath::DegreesToRadians(Graus);
		return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * Distancia;
	}

	FSettlementPlacement Assentamento(ESettlementKind Tipo, float Graus)
	{
		FSettlementPlacement Peca;
		Peca.Kind = Tipo;
		Peca.CenterUnits = PontoDaIlha(Graus, RegionLayout::DistanceFromCenterUnits(Tipo));
		return Peca;
	}
}

float RegionLayout::DistanceFromCenterUnits(ESettlementKind Kind)
{
	const float Raio = IslandGeography::LandRadiusUnits();

	switch (Kind)
	{
		case ESettlementKind::VilaInicial:     return 0.0f;
		case ESettlementKind::VilaDaAcademia:  return Raio * FracaoDasVilas;
		case ESettlementKind::VilaDoMercado:   return Raio * FracaoDasVilas;
		case ESettlementKind::CidadeGrande:    return Raio * FracaoDaCidade;
		case ESettlementKind::PostoDeFronteira:
			return Raio - IslandGeography::BeachWidthUnits() * (1.0f + FracaoDaOrlaDoPosto);
	}

	return 0.0f;
}

int32 RegionLayout::BorderPostCount()
{
	return QuantidadeDePostos;
}

TArray<FSettlementPlacement> RegionLayout::Plan()
{
	TArray<FSettlementPlacement> Pecas;

	// A vila inicial no CENTRO, debaixo de onde o jogador nasce. Sair de casa é
	// o primeiro passo do jogo; nascer longe da vila faria o primeiro passo ser
	// uma caminhada até o começo.
	Pecas.Add(Assentamento(ESettlementKind::VilaInicial, 0.0f));

	// Academia e mercado em rumos OPOSTOS: a quatro minutos de casa, e uma da
	// outra. Postas lado a lado, a segunda viagem seria a mesma viagem.
	Pecas.Add(Assentamento(ESettlementKind::VilaDaAcademia, RumoDaAcademia));
	Pecas.Add(Assentamento(ESettlementKind::VilaDoMercado, RumoDoMercado));

	Pecas.Add(Assentamento(ESettlementKind::CidadeGrande, RumoDaCidade));

	// Os postos ESPALHADOS pela borda. Um só posto faria a fronteira ser um
	// lugar; três fazem dela uma borda, que é o que ela é.
	for (int32 Indice = 0; Indice < QuantidadeDePostos; ++Indice)
	{
		const float Graus = PrimeiroRumoDePosto
			+ (static_cast<float>(Indice) * 360.0f / static_cast<float>(QuantidadeDePostos));
		Pecas.Add(Assentamento(ESettlementKind::PostoDeFronteira, Graus));
	}

	return Pecas;
}
