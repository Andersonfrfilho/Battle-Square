// Copyright 2026 Anderson. All Rights Reserved.

#include "World/CrossingMesh.h"

#include "Debug/BattleDebugScreen.h"
#include "Environment/ScenaryPalette.h"
#include "ProceduralMeshComponent.h"
#include "World/FerryActor.h"
#include "World/RiverMesh.h"
#include "World/TrailLayout.h"

namespace Travessia
{
	constexpr int32 ChaveDoPainel = 730;

	/** Quantos tipos de travessia existem. Sai do enum, não de um número solto. */
	constexpr int32 Tipos = static_cast<int32>(TrailLayout::ECrossingKind::Balsa) + 1;

	constexpr uint8 Vau = static_cast<uint8>(TrailLayout::ECrossingKind::Vau);
	constexpr uint8 Ponte = static_cast<uint8>(TrailLayout::ECrossingKind::Ponte);
	constexpr uint8 Barranco = static_cast<uint8>(TrailLayout::ECrossingKind::Barranco);
	constexpr uint8 Balsa = static_cast<uint8>(TrailLayout::ECrossingKind::Balsa);

	/**
	 * Quanto o tabuleiro passa acima da lâmina.
	 *
	 * Ponte na altura da água não é ponte: ela some dentro do rio, e a obra
	 * que o traçado mandou construir vira uma faixa piscando.
	 */
	constexpr float VaoDaPonte = 120.0f;

	/** A balsa flutua: ela fica logo acima da lâmina, não acima do vão. */
	constexpr float BordaLivreDaBalsa = 30.0f;

	/** O tamanho da obra, como múltiplo da largura da trilha que ela continua. */
	constexpr float LarguraEmTrilhas = 1.2f;

	/** Uma laje retangular deitada, centrada e girada para o rumo dado. */
	void MontarLaje(const FVector2D& Centro, const FVector2D& Rumo, float MeioComprimento,
		float MeiaLargura, float Altura,
		TArray<FVector>& Vertices, TArray<int32>& Triangulos, TArray<FVector2D>& Uvs)
	{
		const FVector2D Lado(-Rumo.Y, Rumo.X);
		const int32 Primeiro = Vertices.Num();

		const FVector2D Cantos[4] = {
			Centro - Rumo * MeioComprimento - Lado * MeiaLargura,
			Centro + Rumo * MeioComprimento - Lado * MeiaLargura,
			Centro + Rumo * MeioComprimento + Lado * MeiaLargura,
			Centro - Rumo * MeioComprimento + Lado * MeiaLargura
		};

		for (int32 Canto = 0; Canto < 4; ++Canto)
		{
			Vertices.Add(FVector(Cantos[Canto].X, Cantos[Canto].Y, Altura));
			Uvs.Add(FVector2D(Canto == 1 || Canto == 2 ? 1.0f : 0.0f,
				Canto >= 2 ? 1.0f : 0.0f));
		}

		Triangulos.Add(Primeiro);
		Triangulos.Add(Primeiro + 3);
		Triangulos.Add(Primeiro + 1);

		Triangulos.Add(Primeiro + 1);
		Triangulos.Add(Primeiro + 3);
		Triangulos.Add(Primeiro + 2);
	}

	/**
	 * A RAMPA do barranco: a mesma laje, mas INCLINADA.
	 *
	 * É o que separa o barranco da ponte. A ponte precisa de duas margens no
	 * mesmo nível; onde uma delas é um degrau, o que se faz é cavar a descida
	 * — e uma laje horizontal ali seria uma ponte que ninguém pediu.
	 */
	void MontarRampa(const FVector2D& Centro, const FVector2D& Rumo, float MeioComprimento,
		float MeiaLargura, float AlturaDeCima, float AlturaDeBaixo,
		TArray<FVector>& Vertices, TArray<int32>& Triangulos, TArray<FVector2D>& Uvs)
	{
		const FVector2D Lado(-Rumo.Y, Rumo.X);
		const int32 Primeiro = Vertices.Num();

		const FVector2D Atras = Centro - Rumo * MeioComprimento;
		const FVector2D Frente = Centro + Rumo * MeioComprimento;

		Vertices.Add(FVector((Atras - Lado * MeiaLargura).X,
			(Atras - Lado * MeiaLargura).Y, AlturaDeCima));
		Vertices.Add(FVector((Frente - Lado * MeiaLargura).X,
			(Frente - Lado * MeiaLargura).Y, AlturaDeBaixo));
		Vertices.Add(FVector((Frente + Lado * MeiaLargura).X,
			(Frente + Lado * MeiaLargura).Y, AlturaDeBaixo));
		Vertices.Add(FVector((Atras + Lado * MeiaLargura).X,
			(Atras + Lado * MeiaLargura).Y, AlturaDeCima));

		Uvs.Add(FVector2D(0.0f, 0.0f));
		Uvs.Add(FVector2D(1.0f, 0.0f));
		Uvs.Add(FVector2D(1.0f, 1.0f));
		Uvs.Add(FVector2D(0.0f, 1.0f));

		Triangulos.Add(Primeiro);
		Triangulos.Add(Primeiro + 3);
		Triangulos.Add(Primeiro + 1);

		Triangulos.Add(Primeiro + 1);
		Triangulos.Add(Primeiro + 3);
		Triangulos.Add(Primeiro + 2);
	}
}

float ACrossingMesh::DeckClearanceUnits()
{
	return Travessia::VaoDaPonte;
}

ACrossingMesh::ACrossingMesh()
{
	PrimaryActorTick.bCanEverTick = false;

	Structure = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CrossingStructure"));
	SetRootComponent(Structure);

	// Tinta no CONSTRUTOR. Sem ela a obra sobe com o xadrez cinza da engine.
	if (UMaterialInterface* Tinta = ScenaryPalette::ColorableBaseMaterial())
	{
		Structure->SetMaterial(0, Tinta);
	}

	// A obra SUSTENTA: é ela que faz a travessia possível onde a água é funda.
	// Sem colisão, quem pisa na ponte cai no rio, e a ponte vira desenho.
	Structure->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Structure->SetCollisionResponseToAllChannels(ECR_Block);
	Structure->bUseComplexAsSimpleCollision = true;
}

int32 ACrossingMesh::GetSeenCount(uint8 Tipo) const
{
	return SeenByKind.IsValidIndex(Tipo) ? SeenByKind[Tipo] : 0;
}

int32 ACrossingMesh::GetBuiltCount(uint8 Tipo) const
{
	return BuiltByKind.IsValidIndex(Tipo) ? BuiltByKind[Tipo] : 0;
}

int32 ACrossingMesh::GetSeenTotal() const
{
	int32 Total = 0;
	for (const int32 Quantas : SeenByKind)
	{
		Total += Quantas;
	}
	return Total;
}

int32 ACrossingMesh::BuildFrom(const UIslandBakedPlan& Assado)
{
	if (!Structure)
	{
		return 0;
	}

	Structure->ClearAllMeshSections();
	SeenByKind.Reset();
	SeenByKind.SetNumZeroed(Travessia::Tipos);
	BuiltByKind.Reset();
	BuiltByKind.SetNumZeroed(Travessia::Tipos);
	Built.Reset();

	// O plano da montagem anterior sai: remontar sem isto planejaria duas
	// frotas no mesmo rio.
	FerryPlacements.Reset();

	const float MeiaLargura = TrailLayout::HalfWidthUnits() * Travessia::LarguraEmTrilhas;

	TArray<FVector> Vertices;
	TArray<int32> Triangulos;
	TArray<FVector2D> Uvs;

	for (const FBakedCrossing& Onde : Assado.Crossings)
	{
		if (SeenByKind.IsValidIndex(Onde.Kind))
		{
			++SeenByKind[Onde.Kind];
		}

		// O VAU não constrói nada, e isso é decisão. Ele é a AUSÊNCIA de
		// obstáculo: quem diz que ali se passa a pé é `WaterFooting`, e uma
		// geometria aqui seria obra onde o traçado disse que não há.
		if (Onde.Kind == Travessia::Vau)
		{
			continue;
		}

		// O rumo da obra é o da trilha que ela continua — perpendicular ao
		// rio. Sem rumo, toda ponte sairia alinhada ao eixo X, e metade delas
		// atravessaria o rio no sentido do comprimento.
		const FVector2D Rumo = [&Assado, &Onde]()
		{
			float MaisPerto = TNumericLimits<float>::Max();
			FVector2D Achado(1.0f, 0.0f);

			for (const FBakedTrail& Rota : Assado.Trails)
			{
				for (int32 Ponto = 1; Ponto < Rota.PointsUnits.Num(); ++Ponto)
				{
					const float Distancia =
						FVector2D::Distance(Rota.PointsUnits[Ponto], Onde.CenterUnits);
					if (Distancia < MaisPerto)
					{
						MaisPerto = Distancia;
						FVector2D Direcao =
							Rota.PointsUnits[Ponto] - Rota.PointsUnits[Ponto - 1];
						if (!Direcao.IsNearlyZero())
						{
							Direcao.Normalize();
							Achado = Direcao;
						}
					}
				}
			}
			return Achado;
		}();

		const float NoChao = Assado.HeightAt(Onde.CenterUnits);
		const float NaLamina = NoChao + ARiverMesh::SurfaceLiftUnits();

		// O comprimento da obra vem da FUNDURA que ela vence — obra de tamanho
		// fixo seria uma ponte curta demais no rio largo e longa demais no
		// estreito, e a fundura é o que o traçado mediu ali.
		const float MeioComprimento =
			FMath::Max(Onde.DepthUnits, TrailLayout::HalfWidthUnits());

		const int32 VerticesAntes = Vertices.Num();

		if (Onde.Kind == Travessia::Ponte)
		{
			Travessia::MontarLaje(Onde.CenterUnits, Rumo, MeioComprimento, MeiaLargura,
				NaLamina + Travessia::VaoDaPonte, Vertices, Triangulos, Uvs);
		}
		else if (Onde.Kind == Travessia::Barranco)
		{
			// INCLINADA, e é o que a separa da ponte: onde uma margem é um
			// degrau, o que se faz é cavar a descida.
			Travessia::MontarRampa(Onde.CenterUnits, Rumo, MeioComprimento, MeiaLargura,
				NoChao + Travessia::VaoDaPonte, NaLamina, Vertices, Triangulos, Uvs);
		}
		else if (Onde.Kind == Travessia::Balsa)
		{
			// A BALSA NÃO É LAJE: ela é ATOR, porque é INTERAÇÃO.
			//
			// A ponte resolve o rio ficando parada; a balsa resolve indo e
			// voltando, e esperar por ela é o que torna a travessia uma coisa
			// que se faz em vez de uma que se atravessa. Desenhada como laje,
			// ela era um deck no meio do rio — altura certa, contagem certa, e
			// a distinção que o traçado fez entre os dois tipos apagada.
			// Ela é PLANEJADA aqui e instanciada pelo `GameMode`. Um ator que
			// monta malha nascendo outros atores por dentro mistura duas
			// responsabilidades — e derrubou o processo com uma asserção de
			// thread ao erguer as 25 de uma vez.
			FFerryPlacement Onde2;
			Onde2.CenterUnits = Onde.CenterUnits;
			Onde2.AxisUnits = Rumo;
			Onde2.SpanUnits = MeioComprimento * 2.0f;
			Onde2.WaterZ = NaLamina;
			FerryPlacements.Add(Onde2);

			// Ela entra na conta pelo PLANO, não pelos vértices: a obra dela não
			// vive nesta malha.
			FBuiltCrossing Feita;
			Feita.Kind = Onde.Kind;
			Feita.WaterZ = NaLamina;
			Feita.TopZ = NaLamina + AFerryActor::FreeboardUnits();
			Feita.BottomZ = Feita.TopZ;
			Built.Add(Feita);

			++BuiltByKind[Onde.Kind];
			continue;
		}
		else
		{
			continue;
		}

		++BuiltByKind[Onde.Kind];

		// Guarda a altura DESTA obra, medida nos vértices que ELA acabou de
		// acrescentar. Sem isto, conferir a altura de uma travessia exigiria
		// procurar vértices por perto — e o perto pega a obra do vizinho.
		FBuiltCrossing Feita;
		Feita.Kind = Onde.Kind;
		Feita.WaterZ = NaLamina;
		Feita.TopZ = TNumericLimits<float>::Lowest();
		Feita.BottomZ = TNumericLimits<float>::Max();
		for (int32 Qual = VerticesAntes; Qual < Vertices.Num(); ++Qual)
		{
			Feita.TopZ = FMath::Max(Feita.TopZ, static_cast<float>(Vertices[Qual].Z));
			Feita.BottomZ = FMath::Min(Feita.BottomZ, static_cast<float>(Vertices[Qual].Z));
		}
		Built.Add(Feita);
	}

	int32 Construidas = 0;
	for (const int32 Quantas : BuiltByKind)
	{
		Construidas += Quantas;
	}

	if (Vertices.Num() > 0)
	{
		Structure->CreateMeshSection(0, Vertices, Triangulos, TArray<FVector>(), Uvs,
			TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		ScenaryPalette::PaintComponent(Structure, EScenaryRole::DeadWood);
	}

	FBattleDebugScreen::Show(
		FString::Printf(
			TEXT("MUNDO: %d travessias — %d vaus (sem obra), %d pontes, %d barrancos, %d balsas"),
			GetSeenTotal(), GetSeenCount(Travessia::Vau), GetBuiltCount(Travessia::Ponte),
			GetBuiltCount(Travessia::Barranco), GetBuiltCount(Travessia::Balsa)),
		30.0f, FColor(200, 170, 120), Travessia::ChaveDoPainel);

	return Construidas;
}

void ACrossingMesh::BeginPlay()
{
	Super::BeginPlay();

	if (const UIslandBakedPlan* Assado = IslandBakedPlan::LoadForWorld())
	{
		BuildFrom(*Assado);
	}
}
