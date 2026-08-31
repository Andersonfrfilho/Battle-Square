// Copyright 2026 Anderson. All Rights Reserved.

#include "World/RegionLayout.h"
#include "Environment/IslandGeography.h"
#include "Environment/FreshWater.h"
#include "World/VillageLayout.h"

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

	/**
	 * A cidade foi para o rumo VAZIO.
	 *
	 * Estava a 320°, e com a academia a 55° e o mercado a 235° tudo morava no
	 * mesmo lado do mapa: metade da ilha não tinha assentamento nenhum. O
	 * quadrante noroeste ficava só com monte e vulcão.
	 */
	constexpr float RumoDaCidade = 150.0f;

	constexpr int32 QuantidadeDePostos = 3;
	constexpr float PrimeiroRumoDePosto = 15.0f;

	FVector2D PontoDaIlha(float Graus, float Distancia)
	{
		const float Radianos = FMath::DegreesToRadians(Graus);
		return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * Distancia;
	}

	/**
	 * O mercado mora NA MARGEM — na beira do lago de um rio.
	 *
	 * Toda vila real nasce na água, e esta região tinha sido assentada de
	 * costas para a única água doce que ela tem: os rios nascem na saia dos
	 * montes, e tudo o que se habitava estava mais para dentro.
	 *
	 * O mercado é o certo para ir: comércio anda por água, e mercado é porto
	 * em quase toda cidade que existiu. Ele fica na BEIRA do lago, afastado
	 * pela largura da água mais a folga da clareira — dentro do lago seria uma
	 * vila submersa.
	 *
	 * Isto o põe MAIS LONGE que a cidade grande, e a troca é deliberada: cedo
	 * se vende na cidade, que compra por menos; a viagem até o lago é o que
	 * paga melhor. A economia já dizia isso — a cidade tem tudo pelo pior
	 * preço — e agora a geografia diz junto.
	 */
	FVector2D CentroDoMercado()
	{
		const TArray<FreshWater::FRiverCourse> Rios = FreshWater::Plan();
		if (Rios.Num() == 0)
		{
			const float Radianos = FMath::DegreesToRadians(235.0f);
			return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos))
				* IslandGeography::LandRadiusUnits() * 0.29f;
		}

		// O rio cujo lago fica mais LONGE dos outros assentamentos seria o
		// ideal, mas escolher por distância a coisas que ainda não existem
		// seria conta circular. Escolho pelo índice, que é estável, e o teste
		// de sobreposição pega se um dia colidir.
		const FreshWater::FRiverCourse& Escolhido = Rios[Rios.Num() / 2];

		const FVector2D NoLago = FreshWater::PointAt(Escolhido, Escolhido.LakeRadiusUnits);
		const float Afastar = FreshWater::HalfWidthAt(Escolhido, Escolhido.LakeRadiusUnits)
			+ VillageLayout::ClearingHalfExtentUnitsFor(ESettlementKind::VilaDoMercado);

		// Para FORA do centro da ilha: a margem de fora é a que dá para o mar,
		// e é por onde a mercadoria sai.
		const FVector2D ParaFora = NoLago.GetSafeNormal();
		return NoLago + ParaFora * Afastar;
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
		case ESettlementKind::VilaDoMercado:   return CentroDoMercado().Size();
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
	{
		FSettlementPlacement Mercado;
		Mercado.Kind = ESettlementKind::VilaDoMercado;
		Mercado.CenterUnits = CentroDoMercado();
		Pecas.Add(Mercado);
	}

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
