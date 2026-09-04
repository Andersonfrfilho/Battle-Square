// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/CaveSystem.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/ScenaryPalette.h"

namespace CavernaDaIlha
{
	const TCHAR* MalhaDaPedra = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cube);

	/** O cone da engine: estalactite é literalmente esta forma, de ponta-cabeça. */
	const TCHAR* MalhaDaPonta = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cone);

	/** Os fluxos de sorteio, um por assunto e bem separados. */
	constexpr int32 PrimeiroFluxoDaPoca = 3000;
	constexpr int32 PrimeiroFluxoDaEstalagmite = 9000;
	constexpr int32 PrimeiroFluxoDaEstalactite = 15000;

	/** Quanto do vão de um corredor uma poça ocupa. */
	constexpr float LarguraDaPoca = 0.82f;

	/** A menor ponta de pedra, em fração da maior. */
	constexpr float MenorPontaDaMaior = 0.45f;

	/** A grossura da base de uma ponta, em fração da altura dela. */
	constexpr float GrossuraDaPonta = 0.34f;

	/** A pedra que forra a caverna, conforme o sabor. */
	EScenaryRole PedraDoSabor(ECaveFlavor Sabor)
	{
		return Sabor == ECaveFlavor::Lava ? EScenaryRole::VolcanicRock : EScenaryRole::CaveRock;
	}

	/** O chão, conforme o sabor. */
	EScenaryRole ChaoDoSabor(ECaveFlavor Sabor)
	{
		return Sabor == ECaveFlavor::Lava ? EScenaryRole::VolcanicRock : EScenaryRole::CaveFloor;
	}

	/** O que se acumula no chão. A caverna seca não acumula nada. */
	EScenaryRole PocaDoSabor(ECaveFlavor Sabor)
	{
		switch (Sabor)
		{
		case ECaveFlavor::Lava:
			return EScenaryRole::LavaGlow;
		case ECaveFlavor::Water:
			return EScenaryRole::CaveWater;
		default:
			return EScenaryRole::CaveFloor;
		}
	}
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

	Spikes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CaveSpikes"));
	Spikes->SetupAttachment(CaveRoot);

	Pools = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CavePools"));
	Pools->SetupAttachment(CaveRoot);

	// A brasa nasce apagada. Acender é decisão de `BuildCave`, que é quem sabe o
	// sabor; um componente que já nasce aceso ilumina caverna seca.
	LavaGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("CaveLavaGlow"));
	LavaGlow->SetupAttachment(CaveRoot);
	LavaGlow->SetLightColor(FLinearColor(1.0f, 0.38f, 0.10f));
	LavaGlow->SetCastShadows(false);
	LavaGlow->SetIntensity(0.0f);

	ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(CavernaDaIlha::MalhaDaPonta);
	if (Cone.Succeeded())
	{
		Spikes->SetStaticMesh(Cone.Object);
	}

	if (Cubo.Succeeded())
	{
		Pools->SetStaticMesh(Cubo.Object);
	}

	// Poça e ponta de pedra não param ninguém: a poça é lâmina no chão, e a ponta
	// que bloqueia corredor transforma o labirinto medido num labirinto com
	// obstáculo que nenhum teste de caminho conhece.
	UHierarchicalInstancedStaticMeshComponent* Enfeites[2] = { Spikes, Pools };
	for (UHierarchicalInstancedStaticMeshComponent* Enfeite : Enfeites)
	{
		Enfeite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Enfeite->SetCanEverAffectNavigation(false);
	}

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

void ACaveSystem::BuildCave(const FCaveRecipe& Recipe)
{
	Flavor = Recipe.Flavor;

	Floor->ClearInstances();
	Walls->ClearInstances();
	Shell->ClearInstances();
	Spikes->ClearInstances();
	Pools->ClearInstances();
	LavaGlow->SetIntensity(0.0f);

	ScenaryPalette::PaintComponent(Floor, CavernaDaIlha::ChaoDoSabor(Flavor));
	ScenaryPalette::PaintComponent(Walls, CavernaDaIlha::PedraDoSabor(Flavor));
	ScenaryPalette::PaintComponent(Shell, CavernaDaIlha::PedraDoSabor(Flavor));
	ScenaryPalette::PaintComponent(Spikes, CavernaDaIlha::PedraDoSabor(Flavor));
	ScenaryPalette::PaintComponent(Pools, CavernaDaIlha::PocaDoSabor(Flavor));

	Grid = CaveLabyrinth::Carve(Recipe.Columns, Recipe.Rows, Recipe.Seed);
	if (!Grid.IsValid())
	{
		return;
	}

	const uint32 Semente = Recipe.Seed;
	const bool bTemPoca = Flavor != ECaveFlavor::Dry;
	const float MaiorPonta = WallHeightUnits * FMath::Clamp(StalactiteReachOfWall, 0.0f, 1.0f);

	const float Meia = 0.5f * CellSizeUnits;

	for (int32 Linha = 0; Linha < Grid.Rows; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Grid.Columns; ++Coluna)
		{
			const FVector Centro = CellCenterLocal(Coluna, Linha);

			AddSlab(Floor,
				Centro - FVector(0.0f, 0.0f, 0.5f * FloorThicknessUnits),
				FVector(CellSizeUnits, CellSizeUnits, FloorThicknessUnits));

			const int32 Casa = Grid.Index(Coluna, Linha);
			const bool bNaBoca = (Linha == 0 && Coluna == Grid.EntranceColumn);

			// A boca fica limpa: poça e ponta de pedra logo na entrada leem como
			// "não passe", e a caverna que se explora não devia dizer isso.
			if (bTemPoca && !bNaBoca
				&& BattleSpread::Fraction(Semente, CavernaDaIlha::PrimeiroFluxoDaPoca + Casa) < PoolChance)
			{
				const float Lado = CorridorWidthUnits() * CavernaDaIlha::LarguraDaPoca;
				AddSlab(Pools,
					Centro + FVector(0.0f, 0.0f, 0.5f * PoolThicknessUnits),
					FVector(Lado, Lado, PoolThicknessUnits));
			}

			if (!bNaBoca
				&& BattleSpread::Fraction(Semente, CavernaDaIlha::PrimeiroFluxoDaEstalagmite + Casa * 2)
					< FloorStalagmiteChance)
			{
				const float Altura = BattleSpread::Between(
					MaiorPonta * CavernaDaIlha::MenorPontaDaMaior, MaiorPonta,
					BattleSpread::Fraction(Semente, CavernaDaIlha::PrimeiroFluxoDaEstalagmite + Casa * 2 + 1));
				AddSpike(Centro, Altura, /*bPendurada=*/false);
			}

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

	// A verga de CADA boca: fecha por cima o vão que ela abriu na muralha, para
	// se passar POR BAIXO da pedra em vez de por um entalhe nela.
	//
	// Uma por boca, e não só na entrada do sul. As bocas extras abriam a parede
	// INTERNA e a muralha continuava inteira por cima delas — a caverna
	// continuava com uma saída só no mundo, e as outras eram buraco em parede
	// que ninguém alcança. O teste de contagem de lajes foi quem mostrou.
	const float AlturaDaVerga = ShellHeightUnits - WallHeightUnits;

	const FVector Boca = CellCenterLocal(Grid.EntranceColumn, 0)
		- FVector(0.0f, Meia, 0.0f);
	AddSlab(Shell,
		Boca + FVector(0.0f, 0.0f, WallHeightUnits + 0.5f * AlturaDaVerga),
		FVector(CellSizeUnits + ShellThicknessUnits, ShellThicknessUnits, AlturaDaVerga));

	for (const CaveLabyrinth::FCaveGrid::FMouth& Outra : Grid.ExtraMouths)
	{
		FVector Onde = FVector::ZeroVector;
		FVector Tamanho = FVector::ZeroVector;

		switch (Outra.Edge)
		{
		case 1:
			Onde = CellCenterLocal(Outra.Along, Grid.Rows - 1) + FVector(0.0f, Meia, 0.0f);
			Tamanho = FVector(CellSizeUnits + ShellThicknessUnits,
				ShellThicknessUnits, AlturaDaVerga);
			break;

		case 2:
			Onde = CellCenterLocal(0, Outra.Along) - FVector(Meia, 0.0f, 0.0f);
			Tamanho = FVector(ShellThicknessUnits,
				CellSizeUnits + ShellThicknessUnits, AlturaDaVerga);
			break;

		default:
			Onde = CellCenterLocal(Grid.Columns - 1, Outra.Along) + FVector(Meia, 0.0f, 0.0f);
			Tamanho = FVector(ShellThicknessUnits,
				CellSizeUnits + ShellThicknessUnits, AlturaDaVerga);
			break;
		}

		AddSlab(Shell, Onde + FVector(0.0f, 0.0f, WallHeightUnits + 0.5f * AlturaDaVerga),
			Tamanho);
	}

	HangMouthStalactites(Boca, Semente);

	// A tampa. Ela não é um portão fechado: é rocha, do chão até a verga, e é o
	// que faz a silhueta dizer de longe que ali não se entra.
	if (!IsExplorable())
	{
		AddSlab(Shell,
			Boca + FVector(0.0f, 0.0f, 0.5f * WallHeightUnits),
			FVector(CellSizeUnits + ShellThicknessUnits, ShellThicknessUnits, WallHeightUnits));
	}

	if (Flavor == ECaveFlavor::Lava)
	{
		// A luz mora na altura do peito das paredes, e não no chão: no chão ela
		// acende a laje e deixa a pedra em volta preta, que é a mesma caverna de
		// antes com uma mancha laranja.
		LavaGlow->SetRelativeLocation(FVector(0.0, 0.0, 0.5 * WallHeightUnits));
		LavaGlow->SetAttenuationRadius(FootprintUnits());
		LavaGlow->SetIntensity(LavaGlowIntensity);
	}
}

void ACaveSystem::HangMouthStalactites(const FVector& Boca, uint32 Seed)
{
	if (MouthStalactiteCount <= 0)
	{
		return;
	}

	const float MaiorPonta = WallHeightUnits * FMath::Clamp(StalactiteReachOfWall, 0.0f, 1.0f);
	const float Vao = CellSizeUnits;
	const float Passo = Vao / static_cast<float>(MouthStalactiteCount + 1);

	for (int32 Qual = 0; Qual < MouthStalactiteCount; ++Qual)
	{
		const int32 Fluxo = CavernaDaIlha::PrimeiroFluxoDaEstalactite + Qual * 2;
		const float Altura = BattleSpread::Between(
			MaiorPonta * CavernaDaIlha::MenorPontaDaMaior, MaiorPonta,
			BattleSpread::Fraction(Seed, Fluxo));

		// Espalhadas ao longo do vão, com um empurrão para o lado para não saírem
		// alinhadas como dentes de pente.
		const float Deslocamento = -0.5f * Vao + Passo * static_cast<float>(Qual + 1)
			+ BattleSpread::Between(-0.2f * Passo, 0.2f * Passo,
				BattleSpread::Fraction(Seed, Fluxo + 1));

		AddSpike(Boca + FVector(Deslocamento, 0.0f, WallHeightUnits), Altura, /*bPendurada=*/true);
	}
}

void ACaveSystem::AddSpike(const FVector& Apoio, float Altura, bool bPendurada)
{
	if (Spikes == nullptr || Spikes->GetStaticMesh() == nullptr || Altura <= 0.0f)
	{
		return;
	}

	const FBox Caixa = Spikes->GetStaticMesh()->GetBoundingBox();
	const FVector TamanhoDoCone = Caixa.GetSize();
	if (TamanhoDoCone.X <= 0.0f || TamanhoDoCone.Y <= 0.0f || TamanhoDoCone.Z <= 0.0f)
	{
		return;
	}

	const float Largura = Altura * CavernaDaIlha::GrossuraDaPonta;
	const FVector Escala(
		Largura / TamanhoDoCone.X,
		Largura / TamanhoDoCone.Y,
		Altura / TamanhoDoCone.Z);

	FTransform Ponta;
	Ponta.SetScale3D(Escala);
	Ponta.SetRotation(FQuat(FRotator(0.0f, 0.0f, bPendurada ? 180.0f : 0.0f)));

	// O apoio é a BASE de quem sobe e o TETO de quem pende. Mirar o centro da
	// instância direto enterraria metade da ponta na pedra — e o cone da engine
	// nem tem o centro da caixa na origem, então a conta é medida, não suposta.
	const FVector Alvo = Apoio
		+ FVector(0.0f, 0.0f, bPendurada ? -0.5f * Altura : 0.5f * Altura);
	const FVector Desvio = Ponta.GetRotation().RotateVector(Caixa.GetCenter() * Escala);
	Ponta.SetLocation(Alvo - Desvio);

	Spikes->AddInstance(Ponta);
}

UPointLightComponent* ACaveSystem::GetLavaGlow() const
{
	return LavaGlow;
}
