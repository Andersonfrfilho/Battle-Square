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
		// TRONCOS: só eles têm lago. O galho de cabeceira morre na junção, e o
		// raio de lago dele é um valor fora de faixa — o mercado foi nascer
		// fora da ilha.
		const TArray<FreshWater::FRiverCourse> Rios = FreshWater::PlanTrunks();
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

		const FVector2D NoLago = FreshWater::PointAtProgress(Escolhido, Escolhido.LakeAtProgress);
		const float Afastar = FreshWater::HalfWidthAtProgress(Escolhido, Escolhido.LakeAtProgress)
			+ VillageLayout::ClearingHalfExtentUnitsFor(ESettlementKind::VilaDoMercado);

		// Para DENTRO, e não para fora.
		//
		// Empurrar para a margem de fora fazia sentido quando o lago ficava no
		// miolo: era a margem que dá para o mar. Com a bacia dendrítica o lago
		// desceu para perto da foz, e o mesmo empurrão pôs a vila a 935
		// unidades do mar aberto — fora do alcance do traçado de trilhas, que
		// para na praia.
		//
		// O sintoma foi mudo: nenhuma trilha chegava lá, e o traçado desenhava
		// uma linha reta em vez de dizer que falhou.
		const FVector2D ParaDentro = -NoLago.GetSafeNormal();
		const FVector2D NaMargem = NoLago + ParaDentro * Afastar;

		// E preso dentro da faixa que a trilha alcança: assentamento que
		// nenhuma trilha atinge é assentamento que ninguém acha.
		const float Limite = IslandGeography::LandRadiusUnits()
			- IslandGeography::BeachWidthUnits() * 1.6f;

		return (NaMargem.Size() > Limite)
			? NaMargem.GetSafeNormal() * Limite
			: NaMargem;
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
	// TRAÇADO UMA VEZ. O mapa é fixo — nada aqui depende de partida, de jogador
	// nem de relógio — e refazê-lo a cada pergunta era o que travava o traçado
	// das trilhas: o custo de um passo pergunta se o ponto está na rampa, a
	// rampa pergunta onde é a cidade, e a cidade estava sendo recalculada
	// (com os RIOS junto, por causa do Mercado do Lago) uma vez por aresta.
	//
	// Milhões de arestas, uma montagem de região em cada.
	static const TArray<FSettlementPlacement> Guardado = []()
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
	}();

	return Guardado;
}
