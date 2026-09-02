// Copyright 2026 Anderson. All Rights Reserved.

#include "World/AqueductMesh.h"

#include "Debug/BattleDebugScreen.h"
#include "Environment/ScenaryPalette.h"
#include "ProceduralMeshComponent.h"
#include "World/AqueductLayout.h"

namespace Aqueduto
{
	constexpr int32 ChaveDoPainel = 735;

	/**
	 * Quanto a calha passa acima do chão MAIS ALTO do percurso.
	 *
	 * Acima do mais alto, e não de cada ponto: a calha é uma reta que desce, e
	 * medir ponto a ponto a faria acompanhar o morro — que é exatamente o que
	 * um aqueduto não faz. Ele vence o terreno; não o segue.
	 */
	constexpr float FolgaDaCalha = 200.0f;
}

float AAqueductMesh::ClearanceUnits()
{
	return Aqueduto::FolgaDaCalha;
}

AAqueductMesh::AAqueductMesh()
{
	PrimaryActorTick.bCanEverTick = false;

	Channel = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("AqueductChannel"));
	SetRootComponent(Channel);

	// Tinta no CONSTRUTOR: sem ela a obra sobe com o xadrez cinza da engine.
	if (UMaterialInterface* Tinta = ScenaryPalette::ColorableBaseMaterial())
	{
		Channel->SetMaterial(0, Tinta);
	}

	// O aqueduto é obra de pedra: sustenta quem anda em cima.
	Channel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Channel->SetCollisionResponseToAllChannels(ECR_Block);
	Channel->bUseComplexAsSimpleCollision = true;
}

bool AAqueductMesh::IsTunnelAt(int32 Qual, int32 Ponto) const
{
	if (!FirstPointOf.IsValidIndex(Qual))
	{
		return false;
	}

	const int32 Indice = FirstPointOf[Qual] + Ponto;
	return BuiltIsTunnel.IsValidIndex(Indice) ? BuiltIsTunnel[Indice] : false;
}

float AAqueductMesh::BuiltHeightAt(int32 Qual, int32 Ponto) const
{
	if (!FirstPointOf.IsValidIndex(Qual))
	{
		return 0.0f;
	}

	const int32 Indice = FirstPointOf[Qual] + Ponto;
	return BuiltHeights.IsValidIndex(Indice) ? BuiltHeights[Indice] : 0.0f;
}

int32 AAqueductMesh::BuildFrom(const UIslandBakedPlan& Assado)
{
	if (!Channel)
	{
		return 0;
	}

	Channel->ClearAllMeshSections();
	BuiltHeights.Reset();
	BuiltIsTunnel.Reset();
	FirstPointOf.Reset();
	BuiltAqueductCount = 0;
	TunnelPointCount = 0;

	const float Meia = AqueductLayout::HalfWidthUnits();

	int32 Secao = 0;
	for (const FBakedAqueduct& Obra : Assado.Aqueducts)
	{
		FirstPointOf.Add(BuiltHeights.Num());

		const int32 Pontos = Obra.PointsUnits.Num();
		if (Pontos < 2)
		{
			continue;
		}

		// A CALHA DESCE DIREITO, e onde o morro sobe acima dela há TÚNEL.
		//
		// Ela já foi um ENVELOPE que contornava tudo por cima, justamente para
		// nunca entrar na rocha. Isso satisfazia metade do que a obra é — "a
		// estrutura dá a volta" — e deixava a outra metade sem existir.
		//
		// A referência é de fora, como as outras deste projeto: o aqueduto
		// romano corria em sua MAIOR PARTE sob a terra, e o arco era a
		// exceção. Uma obra que só contorna é uma obra que não sabe furar.
		//
		// A SAÍDA fica acima do chão sempre: ela entrega água à vila, e uma
		// boca enterrada não entrega nada.
		const float NoFim =
			Assado.HeightAt(Obra.PointsUnits.Last()) + Aqueduto::FolgaDaCalha;

		// A entrada é a saída MAIS A QUEDA DECLARADA. A queda é do traçado, e
		// respeitá-la é o que impede a obra de inventar altura para escapar do
		// morro — que era exatamente o que o envelope fazia.
		const float NoComeco = NoFim + FMath::Max(Obra.DropUnits, 1.0f);
		TArray<FVector> Vertices;
		TArray<int32> Triangulos;
		TArray<FVector2D> Uvs;

		for (int32 Ponto = 0; Ponto < Pontos; ++Ponto)
		{
			const FVector2D Aqui = Obra.PointsUnits[Ponto];
			const FVector2D Antes = Obra.PointsUnits[FMath::Max(Ponto - 1, 0)];
			const FVector2D Depois = Obra.PointsUnits[FMath::Min(Ponto + 1, Pontos - 1)];

			FVector2D Rumo = Depois - Antes;
			if (Rumo.IsNearlyZero())
			{
				Rumo = FVector2D(1.0f, 0.0f);
			}
			Rumo.Normalize();

			const float AoLongo = static_cast<float>(Ponto) / (Pontos - 1);
			const float Altura = FMath::Lerp(NoComeco, NoFim, AoLongo);
			BuiltHeights.Add(Altura);

			// TÚNEL é onde o chão passa por cima da calha.
			const bool bEmTunel = Altura < Assado.HeightAt(Aqui);
			BuiltIsTunnel.Add(bEmTunel);
			if (bEmTunel)
			{
				++TunnelPointCount;
			}

			const FVector2D Lado(-Rumo.Y, Rumo.X);
			const FVector2D Esquerda = Aqui - Lado * Meia;
			const FVector2D Direita = Aqui + Lado * Meia;

			Vertices.Add(FVector(Esquerda.X, Esquerda.Y, Altura));
			Vertices.Add(FVector(Direita.X, Direita.Y, Altura));
			Uvs.Add(FVector2D(0.0f, AoLongo));
			Uvs.Add(FVector2D(1.0f, AoLongo));
		}

		for (int32 Ponto = 0; Ponto + 1 < Pontos; ++Ponto)
		{
			const int32 EsquerdaAqui = Ponto * 2;
			Triangulos.Add(EsquerdaAqui);
			Triangulos.Add(EsquerdaAqui + 2);
			Triangulos.Add(EsquerdaAqui + 1);

			Triangulos.Add(EsquerdaAqui + 1);
			Triangulos.Add(EsquerdaAqui + 2);
			Triangulos.Add(EsquerdaAqui + 3);
		}

		Channel->CreateMeshSection(Secao, Vertices, Triangulos, TArray<FVector>(), Uvs,
			TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		++Secao;
		++BuiltAqueductCount;
	}

	if (Secao > 0)
	{
		ScenaryPalette::PaintComponent(Channel, EScenaryRole::Rock);
		if (UMaterialInterface* Tinta = Channel->GetMaterial(0))
		{
			for (int32 Qual = 1; Qual < Secao; ++Qual)
			{
				Channel->SetMaterial(Qual, Tinta);
			}
		}
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("MUNDO: %d aquedutos, %d pontos em tunel"),
			BuiltAqueductCount, TunnelPointCount),
		30.0f, FColor(180, 180, 200), Aqueduto::ChaveDoPainel);

	return BuiltAqueductCount;
}

void AAqueductMesh::BeginPlay()
{
	Super::BeginPlay();

	if (const UIslandBakedPlan* Assado = IslandBakedPlan::LoadForWorld())
	{
		BuildFrom(*Assado);
	}
}
