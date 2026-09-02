// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TerrainMesh.h"

#include "Debug/BattleDebugScreen.h"
#include "Environment/ScenaryPalette.h"
#include "ProceduralMeshComponent.h"
#include "World/IslandBakedPlan.h"

namespace RelevoDaIlha
{
	/**
	 * A linha do painel do relevo.
	 *
	 * Chave fixa: o relevo é erguido uma vez, e uma linha nova a cada
	 * reconstrução empilharia cópias da mesma informação.
	 */
	constexpr int32 ChaveDoPainel = 700;
}

ATerrainMesh::ATerrainMesh()
{
	PrimaryActorTick.bCanEverTick = false;

	Surface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainSurface"));
	SetRootComponent(Surface);

	// Material atribuído NO CONSTRUTOR, não na montagem. Componente sem asset
	// passa em todo teste de lógica e não existe na tela — o padrão que já
	// custou três defeitos a este projeto (pets, inimigos, o próprio jogador).
	//
	// Malha procedural não tem `SetStaticMesh`: o que se atribui aqui é a
	// TINTA, e a geometria vem do assado. Sem esta linha o chão sobe com o
	// material cinza-xadrez padrão da engine, que é o mesmo tipo de invisível.
	if (UMaterialInterface* Tinta = ScenaryPalette::ColorableBaseMaterial())
	{
		Surface->SetMaterial(0, Tinta);
	}

	// O relevo existe para ser PISADO. Sem colisão ele é um desenho, e o
	// jogador atravessa a ilha inteira caindo.
	Surface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Surface->SetCollisionResponseToAllChannels(ECR_Block);
	Surface->bUseComplexAsSimpleCollision = true;
}

float ATerrainMesh::BuiltHeightAtCell(int32 Column, int32 Row) const
{
	const int32 Indice = Row * GridSide + Column;
	return BuiltHeightUnits.IsValidIndex(Indice) ? BuiltHeightUnits[Indice] : 0.0f;
}

float ATerrainMesh::BuiltHeightAt(const FVector2D& Onde) const
{
	if (GridSide < 2 || CellSizeUnits <= 0.0f)
	{
		return 0.0f;
	}

	const float Raio = CellSizeUnits * (GridSide - 1) * 0.5f;
	const float EmCasas_X = (Onde.X + Raio) / CellSizeUnits;
	const float EmCasas_Y = (Onde.Y + Raio) / CellSizeUnits;

	// Presa à grade: fora dela não há malha, e extrapolar inventaria chão no
	// mar aberto — que é como uma vila já nasceu fora da ilha neste projeto.
	const int32 Coluna = FMath::Clamp(FMath::FloorToInt(EmCasas_X), 0, GridSide - 2);
	const int32 Linha = FMath::Clamp(FMath::FloorToInt(EmCasas_Y), 0, GridSide - 2);

	const float FracaoX = FMath::Clamp(EmCasas_X - Coluna, 0.0f, 1.0f);
	const float FracaoY = FMath::Clamp(EmCasas_Y - Linha, 0.0f, 1.0f);

	const float Baixo = FMath::Lerp(
		BuiltHeightAtCell(Coluna, Linha), BuiltHeightAtCell(Coluna + 1, Linha), FracaoX);
	const float Cima = FMath::Lerp(
		BuiltHeightAtCell(Coluna, Linha + 1), BuiltHeightAtCell(Coluna + 1, Linha + 1),
		FracaoX);

	return FMath::Lerp(Baixo, Cima, FracaoY);
}

int32 ATerrainMesh::BuildFrom(const UIslandBakedPlan& Assado)
{
	GridSide = Assado.HeightGridSide;
	if (GridSide < 2 || !Surface)
	{
		return 0;
	}

	const float Raio = Assado.LandRadiusUnits;
	CellSizeUnits = (2.0f * Raio) / static_cast<float>(GridSide - 1);

	TArray<FVector> Vertices;
	TArray<int32> Triangulos;
	TArray<FVector2D> Uvs;

	Vertices.Reserve(GridSide * GridSide);
	Uvs.Reserve(GridSide * GridSide);
	BuiltHeightUnits.Reset(GridSide * GridSide);

	for (int32 Linha = 0; Linha < GridSide; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < GridSide; ++Coluna)
		{
			const float Altura = Assado.HeightAtCell(Coluna, Linha);
			BuiltHeightUnits.Add(Altura);

			Vertices.Add(FVector(
				-Raio + Coluna * CellSizeUnits,
				-Raio + Linha * CellSizeUnits,
				Altura));

			Uvs.Add(FVector2D(
				static_cast<float>(Coluna) / (GridSide - 1),
				static_cast<float>(Linha) / (GridSide - 1)));
		}
	}

	Triangulos.Reserve((GridSide - 1) * (GridSide - 1) * 6);
	for (int32 Linha = 0; Linha + 1 < GridSide; ++Linha)
	{
		for (int32 Coluna = 0; Coluna + 1 < GridSide; ++Coluna)
		{
			const int32 Aqui = Linha * GridSide + Coluna;
			const int32 Direita = Aqui + 1;
			const int32 Acima = Aqui + GridSide;
			const int32 Diagonal = Acima + 1;

			// Sentido anti-horário visto de cima: invertido, a ilha inteira
			// fica de costas para a câmera e some sem nada acusar.
			Triangulos.Add(Aqui);
			Triangulos.Add(Acima);
			Triangulos.Add(Direita);

			Triangulos.Add(Direita);
			Triangulos.Add(Acima);
			Triangulos.Add(Diagonal);
		}
	}

	Surface->CreateMeshSection(0, Vertices, Triangulos, TArray<FVector>(), Uvs,
		TArray<FColor>(), TArray<FProcMeshTangent>(), true);

	// Pinta pelo mesmo caminho do resto do cenário. A cor da faixa de terreno
	// vem depois; o que não pode é o chão subir sem tinta nenhuma.
	ScenaryPalette::PaintComponent(Surface, EScenaryRole::GroundCover);

	float MaisAlto = TNumericLimits<float>::Lowest();
	float MaisBaixo = TNumericLimits<float>::Max();
	for (const float Altura : BuiltHeightUnits)
	{
		MaisAlto = FMath::Max(MaisAlto, Altura);
		MaisBaixo = FMath::Min(MaisBaixo, Altura);
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("MUNDO: relevo %dx%d casas de %.0f u, alto %.0f, baixo %.0f"),
			GridSide, GridSide, CellSizeUnits, MaisAlto, MaisBaixo),
		30.0f, FColor::Green, RelevoDaIlha::ChaveDoPainel);

	return Vertices.Num();
}

void ATerrainMesh::BeginPlay()
{
	Super::BeginPlay();

	// Pela porta com guarda: assado de outra configuração faz o mundo subir
	// inteiro, parecendo certo, sendo de uma ilha que já não existe.
	if (const UIslandBakedPlan* Assado = IslandBakedPlan::LoadForWorld())
	{
		BuildFrom(*Assado);
	}
}
