// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/CaveSystem.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include "Environment/ScenaryPalette.h"

namespace CavernaDaIlha
{
	const TCHAR* MalhaDaPedra = TEXT("/Engine/BasicShapes/Cube.Cube");
}

ACaveSystem::ACaveSystem()
{
	PrimaryActorTick.bCanEverTick = false;

	CaveRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CaveRoot"));
	SetRootComponent(CaveRoot);

	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(CavernaDaIlha::MalhaDaPedra);

	Floor = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CaveFloor"));
	Floor->SetupAttachment(CaveRoot);

	Walls = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CaveWalls"));
	Walls->SetupAttachment(CaveRoot);

	Shell = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CaveShell"));
	Shell->SetupAttachment(CaveRoot);

	UHierarchicalInstancedStaticMeshComponent* Pedras[3] = { Floor, Walls, Shell };
	for (UHierarchicalInstancedStaticMeshComponent* Pedra : Pedras)
	{
		if (Cubo.Succeeded())
		{
			Pedra->SetStaticMesh(Cubo.Object);
		}

		// Sem colisão a caverna é pintura: atravessa-se a parede e o labirinto
		// deixa de ser um labirinto.
		Pedra->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Pedra->SetCollisionResponseToAllChannels(ECR_Block);
		Pedra->SetCanEverAffectNavigation(false);
	}
}

FVector ACaveSystem::CellCenterLocal(int32 Column, int32 Row) const
{
	// Centrado no ator: a caverna cresce para os dois lados a partir da origem,
	// e plantá-la é escolher um ponto, não corrigir um canto.
	const float X = (static_cast<float>(Column) + 0.5f - 0.5f * static_cast<float>(Grid.Columns))
		* CellSizeUnits;
	const float Y = (static_cast<float>(Row) + 0.5f - 0.5f * static_cast<float>(Grid.Rows))
		* CellSizeUnits;
	return FVector(X, Y, 0.0f);
}

FVector ACaveSystem::EntranceLocal() const
{
	if (!Grid.IsValid())
	{
		return FVector::ZeroVector;
	}

	// Um passo para FORA da boca: quem for posto aqui está na frente da caverna,
	// não dentro da parede.
	FVector Boca = CellCenterLocal(Grid.EntranceColumn, 0);
	Boca.Y -= CellSizeUnits;
	return Boca;
}

FVector ACaveSystem::DeepestLocal() const
{
	if (!Grid.IsValid())
	{
		return FVector::ZeroVector;
	}

	int32 FundoColuna = 0;
	int32 FundoLinha = 0;
	CaveLabyrinth::DeepestFrom(Grid, Grid.EntranceColumn, 0, FundoColuna, FundoLinha);
	return CellCenterLocal(FundoColuna, FundoLinha);
}

float ACaveSystem::FootprintUnits() const
{
	if (!Grid.IsValid())
	{
		return 0.0f;
	}

	const float Maior = static_cast<float>(FMath::Max(Grid.Columns, Grid.Rows));
	return Maior * CellSizeUnits + 2.0f * ShellThicknessUnits;
}

void ACaveSystem::AddSlab(UHierarchicalInstancedStaticMeshComponent* Alvo,
	const FVector& Centro, const FVector& Tamanho)
{
	if (Alvo == nullptr || Alvo->GetStaticMesh() == nullptr)
	{
		return;
	}

	// A escala sai do tamanho MEDIDO da malha. Transcrever "o cubo tem 100" é
	// como o mesmo número vira mentira quando alguém troca a malha.
	const FVector TamanhoDoCubo = Alvo->GetStaticMesh()->GetBoundingBox().GetSize();
	if (TamanhoDoCubo.X <= 0.0f || TamanhoDoCubo.Y <= 0.0f || TamanhoDoCubo.Z <= 0.0f)
	{
		return;
	}

	FTransform Bloco;
	Bloco.SetScale3D(FVector(
		Tamanho.X / TamanhoDoCubo.X,
		Tamanho.Y / TamanhoDoCubo.Y,
		Tamanho.Z / TamanhoDoCubo.Z));
	Bloco.SetLocation(Centro);
	Alvo->AddInstance(Bloco);
}

void ACaveSystem::BuildCave(int32 Columns, int32 Rows, uint32 Seed)
{
	Floor->ClearInstances();
	Walls->ClearInstances();
	Shell->ClearInstances();

	ScenaryPalette::PaintComponent(Floor, EScenaryRole::CaveFloor);
	ScenaryPalette::PaintComponent(Walls, EScenaryRole::CaveRock);
	ScenaryPalette::PaintComponent(Shell, EScenaryRole::CaveRock);

	Grid = CaveLabyrinth::Carve(Columns, Rows, Seed);
	if (!Grid.IsValid())
	{
		return;
	}

	const float Meia = 0.5f * CellSizeUnits;

	for (int32 Linha = 0; Linha < Grid.Rows; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Grid.Columns; ++Coluna)
		{
			const FVector Centro = CellCenterLocal(Coluna, Linha);

			AddSlab(Floor,
				Centro - FVector(0.0f, 0.0f, 0.5f * FloorThicknessUnits),
				FVector(CellSizeUnits, CellSizeUnits, FloorThicknessUnits));

			// Cada segmento de parede é desenhado UMA vez. Norte e leste sempre;
			// sul e oeste só na borda, onde não há vizinho para desenhá-los.
			// Desenhar dos dois lados dobraria a pedra no mesmo lugar e faria a
			// contagem mentir sobre o que existe.
			struct FSegmento
			{
				uint8 Parede;
				FVector Direcao;
				bool bAoLongoDeX;
				bool bNaBorda;
			};

			const bool bBordaNorte = (Linha == Grid.Rows - 1);
			const bool bBordaLeste = (Coluna == Grid.Columns - 1);

			TArray<FSegmento, TInlineAllocator<4>> Segmentos;
			Segmentos.Add({ CaveLabyrinth::WallNorth, FVector(0.0f, 1.0f, 0.0f), true, bBordaNorte });
			Segmentos.Add({ CaveLabyrinth::WallEast, FVector(1.0f, 0.0f, 0.0f), false, bBordaLeste });

			if (Linha == 0)
			{
				Segmentos.Add({ CaveLabyrinth::WallSouth, FVector(0.0f, -1.0f, 0.0f), true, true });
			}
			if (Coluna == 0)
			{
				Segmentos.Add({ CaveLabyrinth::WallWest, FVector(-1.0f, 0.0f, 0.0f), false, true });
			}

			for (const FSegmento& Segmento : Segmentos)
			{
				if (!Grid.HasWall(Coluna, Linha, Segmento.Parede))
				{
					continue;
				}

				const float Altura = Segmento.bNaBorda ? ShellHeightUnits : WallHeightUnits;
				const float Grossura = Segmento.bNaBorda ? ShellThicknessUnits : WallThicknessUnits;

				// O comprimento passa da casa pela PRÓPRIA grossura, e não pela
				// grossura da parede de dentro: é isso que fecha o canto da
				// muralha em vez de deixar um entalhe onde duas se encontram.
				const float Comprimento = CellSizeUnits + Grossura;
				const FVector Tamanho = Segmento.bAoLongoDeX
					? FVector(Comprimento, Grossura, Altura)
					: FVector(Grossura, Comprimento, Altura);

				AddSlab(Segmento.bNaBorda ? Shell : Walls,
					Centro + Segmento.Direcao * Meia + FVector(0.0f, 0.0f, 0.5f * Altura),
					Tamanho);
			}
		}
	}

	// A verga da boca: fecha por cima o vão que a entrada abriu na muralha, para
	// se passar POR BAIXO da pedra em vez de por um entalhe nela.
	const FVector Boca = CellCenterLocal(Grid.EntranceColumn, 0)
		- FVector(0.0f, Meia, 0.0f);
	const float AlturaDaVerga = ShellHeightUnits - WallHeightUnits;
	AddSlab(Shell,
		Boca + FVector(0.0f, 0.0f, WallHeightUnits + 0.5f * AlturaDaVerga),
		FVector(CellSizeUnits + ShellThicknessUnits, ShellThicknessUnits, AlturaDaVerga));
}
