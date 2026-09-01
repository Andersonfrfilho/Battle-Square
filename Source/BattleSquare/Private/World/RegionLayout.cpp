// Copyright 2026 Anderson. All Rights Reserved.

#include "World/RegionLayout.h"
#include "World/PlanReentryGuard.h"
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
	 * E ele fica NA AREIA, não atrás dela.
	 *
	 * Eu o tinha posto atrás da praia com medo da rampa da orla — e cais atrás
	 * da praia é galpão: quem embarca precisa alcançar a água. A rampa não é
	 * problema no meio da faixa, onde o chão já está na metade da altura da
	 * terra.
	 */
	constexpr float FracaoDaOrlaDoPosto = -0.45f;

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

	/** Quantos rumos tentar atrás de água, e de quanto em quanto. */
	constexpr int32 VoltasAtrasDeAgua = 36;
	constexpr float GrausPorTentativa = 9.0f;
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
		// TODOS os cursos, não só os troncos: o lago deixou de morar na foz
		// quando passou a ser escolhido pelo trecho manso do leito.
		const TArray<FreshWater::FRiverCourse> Rios = FreshWater::Plan();

		const float Limite = IslandGeography::LandRadiusUnits()
			- IslandGeography::BeachWidthUnits() * 1.6f;

		// ESCOLHE um lago que já esteja no alcance, em vez de escolher qualquer
		// um e empurrar depois.
		//
		// O clamp que eu tinha posto consertava a trilha e estragava a vila:
		// ele arrastava o mercado para o raio limite, a 134 metros do lago. O
		// nome "Mercado do Lago" virou mentira, e a medição foi o que mostrou —
		// o mapa não deixava ver 134 metros.
		//
		// Prefere o lago mais LARGO entre os que servem: mercado é porto, e
		// porto quer água que aguente barco.
		const FreshWater::FRiverCourse* Escolhido = nullptr;
		float MaisLargo = 0.0f;

		for (const FreshWater::FRiverCourse& Rio : Rios)
		{
			if (Rio.LakeAtProgress < 0.0f)
			{
				continue;
			}

			const FVector2D NoLago = FreshWater::PointAtProgress(Rio, Rio.LakeAtProgress);
			const float Largura = FreshWater::HalfWidthAtProgress(Rio, Rio.LakeAtProgress);

			if (NoLago.Size() + Largura > Limite || Largura <= MaisLargo)
			{
				continue;
			}

			MaisLargo = Largura;
			Escolhido = &Rio;
		}

		if (Escolhido == nullptr)
		{
			// Sem lago que sirva, a vila vai para o rumo antigo — e o nome
			// deixa de prometer o que não existe.
			const float Radianos = FMath::DegreesToRadians(235.0f);
			return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos))
				* IslandGeography::LandRadiusUnits() * 0.29f;
		}

		const FVector2D NoLago = FreshWater::PointAtProgress(*Escolhido, Escolhido->LakeAtProgress);

		// ENCOSTADA na água: o afastamento é a lâmina mais meio LOTE, e não
		// mais a clareira inteira. A clareira é a folga para a mata não
		// encostar na parede; usá-la aqui punha a vila longe da margem, que é
		// justamente o que ela veio para tocar.
		const FVector2D ParaDentro = -NoLago.GetSafeNormal();
		const float Afastar = MaisLargo
			+ VillageLayout::PlotHalfExtentUnitsFor(ESettlementKind::VilaDoMercado);

		return NoLago + ParaDentro * Afastar;
	}

	/**
	 * A que distância da água doce um ponto está.
	 *
	 * Toda cidade capta água de algum lugar — é a primeira coisa que se procura
	 * ao fundar uma. Vila longe de água é vila que não existe, e a nossa estava
	 * sendo posta por ÂNGULO, sem ninguém perguntar se havia de beber.
	 */
	float AteAAguaDoce(const FVector2D& Onde)
	{
		float Menor = TNumericLimits<float>::Max();

		for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
		{
			float Aonde = 0.0f;
			Menor = FMath::Min(Menor, FreshWater::NearestOn(Curso, Onde, Aonde)
				- FreshWater::HalfWidthAtProgress(Curso, Aonde));
		}

		for (const FreshWater::FSpring& Fonte : FreshWater::PlanSprings())
		{
			Menor = FMath::Min(Menor, static_cast<float>(
				FVector2D::Distance(Onde, Fonte.CenterUnits)) - Fonte.PoolHalfWidthUnits);
		}

		return Menor;
	}

	/**
	 * Gira o assentamento até achar água, sem sair do anel dele.
	 *
	 * A distância do centro é decisão de RITMO — quatro minutos de caminhada de
	 * casa — e ela não se negocia. O rumo é livre, e é ele que se ajusta: a
	 * vila anda pelo anel até encontrar de beber.
	 *
	 * Fica com o MELHOR rumo tentado mesmo que nenhum sirva. Devolver o rumo
	 * pedido quando nada serve seria fingir que a busca não aconteceu.
	 */
	FSettlementPlacement AssentamentoComAgua(ESettlementKind Tipo, float Graus)
	{
		const float Distancia = RegionLayout::DistanceFromCenterUnits(Tipo);
		const float Longe = VillageLayout::ClearingHalfExtentUnitsFor(Tipo) * 2.0f;

		FSettlementPlacement Melhor;
		Melhor.Kind = Tipo;
		Melhor.CenterUnits = PontoDaIlha(Graus, Distancia);

		float MaisPerto = AteAAguaDoce(Melhor.CenterUnits);

		for (int32 Passo = 1; Passo <= VoltasAtrasDeAgua && MaisPerto > Longe; ++Passo)
		{
			// Alterna para os dois lados, abrindo: assim a vila fica no rumo
			// pedido quando dá, e só se afasta o necessário quando não dá.
			const float Desvio = GrausPorTentativa * ((Passo + 1) / 2)
				* ((Passo % 2 == 0) ? 1.0f : -1.0f);

			const FVector2D Onde = PontoDaIlha(Graus + Desvio, Distancia);
			const float Ate = AteAAguaDoce(Onde);

			if (Ate < MaisPerto)
			{
				MaisPerto = Ate;
				Melhor.CenterUnits = Onde;
			}
		}

		return Melhor;
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
		const FPlanReentryGuard Guarda(TEXT("RegionLayout::Plan"));

	TArray<FSettlementPlacement> Pecas;

	// A vila inicial no CENTRO, debaixo de onde o jogador nasce. Sair de casa é
	// o primeiro passo do jogo; nascer longe da vila faria o primeiro passo ser
	// uma caminhada até o começo.
	Pecas.Add(Assentamento(ESettlementKind::VilaInicial, 0.0f));

	// Academia e mercado em rumos OPOSTOS: a quatro minutos de casa, e uma da
	// outra. Postas lado a lado, a segunda viagem seria a mesma viagem.
	Pecas.Add(AssentamentoComAgua(ESettlementKind::VilaDaAcademia, RumoDaAcademia));
	{
		FSettlementPlacement Mercado;
		Mercado.Kind = ESettlementKind::VilaDoMercado;
		Mercado.CenterUnits = CentroDoMercado();
		Pecas.Add(Mercado);
	}

	Pecas.Add(AssentamentoComAgua(ESettlementKind::CidadeGrande, RumoDaCidade));

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
