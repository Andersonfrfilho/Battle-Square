// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TrailMesh.h"

#include "Debug/BattleDebugScreen.h"
#include "Environment/ScenaryPalette.h"
#include "ProceduralMeshComponent.h"
#include "World/TrailLayout.h"

namespace CaminhoDaIlha
{
	constexpr int32 ChaveDoPainel = 720;

	/**
	 * Quanto a trilha sobe acima do chão.
	 *
	 * Maior que a da água de propósito: a trilha é terra batida SOBRE o
	 * terreno, e onde ela cruza um vau ela tem de passar por cima da lâmina —
	 * senão o caminho desaparece dentro do rio justo no ponto em que o
	 * traçado promete travessia.
	 */
	constexpr float ElevacaoDaTrilha = 30.0f;
}

float ATrailMesh::SurfaceLiftUnits()
{
	return CaminhoDaIlha::ElevacaoDaTrilha;
}

ATrailMesh::ATrailMesh()
{
	PrimaryActorTick.bCanEverTick = false;

	Trail = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TrailSurface"));
	SetRootComponent(Trail);

	// Tinta no CONSTRUTOR: sem ela a trilha sobe com o xadrez cinza da engine,
	// que é o mesmo invisível que já custou três defeitos a este projeto.
	if (UMaterialInterface* Tinta = ScenaryPalette::ColorableBaseMaterial())
	{
		Trail->SetMaterial(0, Tinta);
	}

	// A trilha não barra e não empurra: ela é chão, e o chão de baixo já
	// colide. Colisão aqui poria um degrau de 30 unidades em toda a rede.
	Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trail->SetCanEverAffectNavigation(false);
}

float ATrailMesh::BuiltHeightAt(int32 Trilha, int32 Ponto) const
{
	if (!FirstPointOfTrail.IsValidIndex(Trilha))
	{
		return 0.0f;
	}

	const int32 Indice = FirstPointOfTrail[Trilha] + Ponto;
	return BuiltHeights.IsValidIndex(Indice) ? BuiltHeights[Indice] : 0.0f;
}

int32 ATrailMesh::BuildFrom(const UIslandBakedPlan& Assado)
{
	if (!Trail)
	{
		return 0;
	}

	Trail->ClearAllMeshSections();
	BuiltHeights.Reset();
	FirstPointOfTrail.Reset();
	BuiltTrailCount = 0;

	const float Meia = TrailLayout::HalfWidthUnits();

	int32 Secao = 0;
	for (const FBakedTrail& Rota : Assado.Trails)
	{
		FirstPointOfTrail.Add(BuiltHeights.Num());

		const int32 Pontos = Rota.PointsUnits.Num();
		if (Pontos < 2)
		{
			continue;
		}

		TArray<FVector> Vertices;
		TArray<int32> Triangulos;
		TArray<FVector2D> Uvs;

		for (int32 Ponto = 0; Ponto < Pontos; ++Ponto)
		{
			const FVector2D Aqui = Rota.PointsUnits[Ponto];
			const FVector2D Antes = Rota.PointsUnits[FMath::Max(Ponto - 1, 0)];
			const FVector2D Depois = Rota.PointsUnits[FMath::Min(Ponto + 1, Pontos - 1)];

			FVector2D Rumo = Depois - Antes;
			if (Rumo.IsNearlyZero())
			{
				Rumo = FVector2D(1.0f, 0.0f);
			}
			Rumo.Normalize();

			const FVector2D Lado(-Rumo.Y, Rumo.X);
			const FVector2D Esquerda = Aqui - Lado * Meia;
			const FVector2D Direita = Aqui + Lado * Meia;

			// Assenta na altura que a SUPERFÍCIE tem, não na que o plano diz.
			// A altura do plano deixaria a trilha flutuando ou enterrada
			// exatamente onde a grade arredonda, e o declive continuaria certo.
			const float NoChao = Assado.HeightAt(Aqui) + CaminhoDaIlha::ElevacaoDaTrilha;
			BuiltHeights.Add(NoChao);

			Vertices.Add(FVector(Esquerda.X, Esquerda.Y,
				Assado.HeightAt(Esquerda) + CaminhoDaIlha::ElevacaoDaTrilha));
			Vertices.Add(FVector(Direita.X, Direita.Y,
				Assado.HeightAt(Direita) + CaminhoDaIlha::ElevacaoDaTrilha));

			const float AoLongo = static_cast<float>(Ponto) / (Pontos - 1);
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

		Trail->CreateMeshSection(Secao, Vertices, Triangulos, TArray<FVector>(), Uvs,
			TArray<FColor>(), TArray<FProcMeshTangent>(), false);
		++Secao;
		++BuiltTrailCount;
	}

	ScenaryPalette::PaintComponent(Trail, EScenaryRole::MountainTrail);
	if (UMaterialInterface* Tinta = Trail->GetMaterial(0))
	{
		for (int32 Qual = 1; Qual < Secao; ++Qual)
		{
			Trail->SetMaterial(Qual, Tinta);
		}
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("MUNDO: %d trilhas assentadas no relevo"), BuiltTrailCount),
		30.0f, FColor(200, 170, 120), CaminhoDaIlha::ChaveDoPainel);

	return BuiltTrailCount;
}

void ATrailMesh::BeginPlay()
{
	Super::BeginPlay();

	if (const UIslandBakedPlan* Assado = IslandBakedPlan::LoadForWorld())
	{
		BuildFrom(*Assado);
	}
}
