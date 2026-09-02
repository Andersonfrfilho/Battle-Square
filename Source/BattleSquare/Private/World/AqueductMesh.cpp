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
	FirstPointOf.Reset();
	BuiltAqueductCount = 0;

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

		// A CALHA É UM ENVELOPE, montado DE TRÁS PARA A FRENTE.
		//
		// Ela tem de cumprir duas coisas ao mesmo tempo: nunca subir (água não
		// sobe o morro sozinha) e nunca entrar no morro. Uma reta do começo ao
		// fim cumpre a primeira e falha a segunda — MEDIDO: no aqueduto 0, a
		// reta passava a 4.103,9 onde o chão está a 4.168,5, porque ela desce
		// mais depressa que o terreno naquele trecho.
		//
		// Andando do FIM para o começo e tomando o máximo entre "o chão daqui
		// mais a folga" e "a altura do ponto seguinte", as duas condições saem
		// juntas por construção. A altura de partida deixa de ser escolhida:
		// ela é o que o terreno obriga.
		TArray<float> AlturaDoPonto;
		AlturaDoPonto.SetNumZeroed(Pontos);

		AlturaDoPonto[Pontos - 1] =
			Assado.HeightAt(Obra.PointsUnits.Last()) + Aqueduto::FolgaDaCalha;

		for (int32 Ponto = Pontos - 2; Ponto >= 0; --Ponto)
		{
			AlturaDoPonto[Ponto] = FMath::Max(
				Assado.HeightAt(Obra.PointsUnits[Ponto]) + Aqueduto::FolgaDaCalha,
				AlturaDoPonto[Ponto + 1]);
		}

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
			const float Altura = AlturaDoPonto[Ponto];
			BuiltHeights.Add(Altura);

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
		FString::Printf(TEXT("MUNDO: %d aquedutos"), BuiltAqueductCount),
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
