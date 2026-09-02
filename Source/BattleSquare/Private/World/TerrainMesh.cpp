// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TerrainMesh.h"

#include "Debug/BattleDebugScreen.h"
#include "Environment/ScenaryPalette.h"
#include "ProceduralMeshComponent.h"
#include "World/IslandBakedPlan.h"

namespace RelevoDaIlha
{
	/**
	 * A cor de cada faixa de terreno — a tabela, num lugar só.
	 *
	 * O papel vem da paleta do cenário, e não de uma cor literal: praia e areia
	 * de deserto são a mesma areia, e duas cores literais para a mesma coisa
	 * divergem na primeira edição.
	 */
	EScenaryRole PapelDaFaixa(ETerrainBand Faixa)
	{
		switch (Faixa)
		{
		case ETerrainBand::Praia:         return EScenaryRole::BeachSand;
		case ETerrainBand::Mata:          return EScenaryRole::GroundCover;
		case ETerrainBand::Barranco:      return EScenaryRole::ClimbableRock;
		case ETerrainBand::RochaQueimada: return EScenaryRole::VolcanicRock;
		case ETerrainBand::Cume:          return EScenaryRole::MountainRock;
		default: break;
		}
		return EScenaryRole::GroundCover;
	}

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

	// UMA SEÇÃO POR FAIXA DE TERRENO, e não uma malha só.
	//
	// A cor precisa contar o terreno, e o material base do cenário não lê cor
	// de vértice — pintar por vértice exigiria um material autorado, e
	// capacidade que espera edição de asset é capacidade que não existe hoje.
	// Seção por faixa pinta pelo mesmo caminho que o resto do mundo já usa.
	//
	// O triângulo pertence à faixa do seu PRIMEIRO vértice. Repartir o
	// triângulo na fronteira daria a borda exata e multiplicaria a geometria;
	// numa grade desta finura a serrilha da fronteira mede uma casa.
	TArray<TArray<int32>> TrianguloDaFaixa;
	TrianguloDaFaixa.SetNum(static_cast<int32>(ETerrainBand::Count));

	for (int32 Qual = 0; Qual + 2 < Triangulos.Num(); Qual += 3)
	{
		const FVector& Primeiro = Vertices[Triangulos[Qual]];
		const int32 Faixa = static_cast<int32>(
			Assado.BandAt(FVector2D(Primeiro.X, Primeiro.Y)));

		TrianguloDaFaixa[Faixa].Add(Triangulos[Qual]);
		TrianguloDaFaixa[Faixa].Add(Triangulos[Qual + 1]);
		TrianguloDaFaixa[Faixa].Add(Triangulos[Qual + 2]);
	}

	Surface->ClearAllMeshSections();
	SectionOfBand.Reset();
	SectionOfBand.SetNum(static_cast<int32>(ETerrainBand::Count));

	int32 ProximaSecao = 0;
	for (int32 Faixa = 0; Faixa < TrianguloDaFaixa.Num(); ++Faixa)
	{
		SectionOfBand[Faixa] = INDEX_NONE;
		if (TrianguloDaFaixa[Faixa].Num() == 0)
		{
			continue;
		}

		Surface->CreateMeshSection(ProximaSecao, Vertices, TrianguloDaFaixa[Faixa],
			TArray<FVector>(), Uvs, TArray<FColor>(), TArray<FProcMeshTangent>(), true);

		// Pinta a seção com o papel da faixa. Seção sem tinta sobe com o xadrez
		// cinza da engine — o mesmo invisível de sempre, agora em pedaços.
		ScenaryPalette::PaintComponent(Surface,
			RelevoDaIlha::PapelDaFaixa(static_cast<ETerrainBand>(Faixa)));
		if (UMaterialInterface* Tinta = Surface->GetMaterial(0))
		{
			Surface->SetMaterial(ProximaSecao, Tinta);
		}

		SectionOfBand[Faixa] = ProximaSecao;
		++ProximaSecao;
	}

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

	// Quantas faixas de fato subiram. Faixa que o traçado tem e a malha não
	// mostrou é cor que o jogador nunca vê, e nada mais acusaria.
	FString Faixas;
	for (int32 Faixa = 0; Faixa < SectionOfBand.Num(); ++Faixa)
	{
		if (SectionOfBand[Faixa] != INDEX_NONE)
		{
			Faixas += FString::Printf(TEXT("%s%s"), Faixas.IsEmpty() ? TEXT("") : TEXT(", "),
				UIslandBakedPlan::BandDebugName(static_cast<ETerrainBand>(Faixa)));
		}
	}
	FBattleDebugScreen::Show(
		FString::Printf(TEXT("MUNDO: terreno pintado — %s"), *Faixas),
		30.0f, FColor::Green, RelevoDaIlha::ChaveDoPainel + 1);

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

int32 ATerrainMesh::GetSectionOfBand(ETerrainBand Faixa) const
{
	const int32 Qual = static_cast<int32>(Faixa);
	return SectionOfBand.IsValidIndex(Qual) ? SectionOfBand[Qual] : INDEX_NONE;
}
