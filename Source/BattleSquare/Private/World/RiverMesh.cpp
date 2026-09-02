// Copyright 2026 Anderson. All Rights Reserved.

#include "World/RiverMesh.h"

#include "Debug/BattleDebugScreen.h"
#include "Environment/FreshWater.h"
#include "Environment/ScenaryPalette.h"
#include "ProceduralMeshComponent.h"

namespace AguaCorrente
{
	constexpr int32 ChaveDoPainel = 710;

	/**
	 * Quanto a lâmina sobe acima do chão.
	 *
	 * Pequeno de propósito: o suficiente para ganhar a disputa de
	 * profundidade, pouco o bastante para o rio não ficar boiando sobre o
	 * leito. Uma unidade a mais e a água lê como um tapete.
	 */
	constexpr float ElevacaoDaLamina = 20.0f;

	/**
	 * Quantos lados tem o disco de água parada.
	 *
	 * Doze: abaixo disso o lago lê como polígono, acima é geometria gasta num
	 * remanso que ninguém mede de perto.
	 */
	constexpr int32 LadosDoDisco = 12;

	/**
	 * Monta um disco de água parada, deitado sobre o chão.
	 *
	 * Lago e poço de queda são a mesma forma em tamanhos diferentes — um
	 * remanso. Duas funções para isso divergiriam na primeira edição.
	 */
	void MontarDisco(const UIslandBakedPlan& Assado, const FVector2D& Centro, float Meia,
		TArray<FVector>& Vertices, TArray<int32>& Triangulos, TArray<FVector2D>& Uvs)
	{
		const int32 NoCentro = Vertices.Num();
		Vertices.Add(FVector(Centro.X, Centro.Y,
			Assado.HeightAt(Centro) + ElevacaoDaLamina));
		Uvs.Add(FVector2D(0.5f, 0.5f));

		for (int32 Lado = 0; Lado < LadosDoDisco; ++Lado)
		{
			const float Angulo = (2.0f * PI * Lado) / LadosDoDisco;
			const FVector2D NaBorda = Centro
				+ FVector2D(FMath::Cos(Angulo), FMath::Sin(Angulo)) * Meia;

			// A borda acompanha o CHÃO dela, não o do centro: um disco plano
			// num remanso encostado na encosta enterraria metade dele.
			Vertices.Add(FVector(NaBorda.X, NaBorda.Y,
				Assado.HeightAt(NaBorda) + ElevacaoDaLamina));
			Uvs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angulo),
				0.5f + 0.5f * FMath::Sin(Angulo)));
		}

		for (int32 Lado = 0; Lado < LadosDoDisco; ++Lado)
		{
			Triangulos.Add(NoCentro);
			Triangulos.Add(NoCentro + 1 + Lado);
			Triangulos.Add(NoCentro + 1 + (Lado + 1) % LadosDoDisco);
		}
	}
}

float ARiverMesh::SurfaceLiftUnits()
{
	return AguaCorrente::ElevacaoDaLamina;
}

ARiverMesh::ARiverMesh()
{
	PrimaryActorTick.bCanEverTick = false;

	Water = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RiverWater"));
	SetRootComponent(Water);

	// Tinta no CONSTRUTOR. Malha procedural não tem `SetStaticMesh`, e sem
	// material a água sobe com o xadrez cinza da engine — o mesmo invisível
	// que já custou três defeitos a este projeto.
	if (UMaterialInterface* Tinta = ScenaryPalette::ColorableBaseMaterial())
	{
		Water->SetMaterial(0, Tinta);
	}

	// A água não BARRA. Quem entra no rio atravessa — o que muda é o
	// movimento, e isso é regra, não colisão. Rio que empurra seria parede.
	Water->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Water->SetCollisionResponseToAllChannels(ECR_Overlap);
	Water->SetCanEverAffectNavigation(false);
}

float ARiverMesh::BuiltHalfWidthAt(int32 Curso, int32 Amostra) const
{
	const int32 Indice = Curso * SamplesPerCourse + Amostra;
	return BuiltHalfWidths.IsValidIndex(Indice) ? BuiltHalfWidths[Indice] : 0.0f;
}

int32 ARiverMesh::BuildFrom(const UIslandBakedPlan& Assado)
{
	if (!Water)
	{
		return 0;
	}

	Water->ClearAllMeshSections();
	BuiltHalfWidths.Reset();
	BuiltCourseCount = 0;
	BuiltLakeCount = 0;
	BuiltPlungePoolCount = 0;
	SamplesPerCourse = IslandBakedPlan::RiverSampleCount();

	int32 Secao = 0;
	for (const FBakedRiver& Curso : Assado.Rivers)
	{
		const int32 Pontos = Curso.PointsUnits.Num();
		if (Pontos < 2)
		{
			continue;
		}

		TArray<FVector> Vertices;
		TArray<int32> Triangulos;
		TArray<FVector2D> Uvs;
		Vertices.Reserve(Pontos * 2);
		Uvs.Reserve(Pontos * 2);

		for (int32 Ponto = 0; Ponto < Pontos; ++Ponto)
		{
			const FVector2D Aqui = Curso.PointsUnits[Ponto];

			// A direção do curso NESTE ponto, olhando para o vizinho. Nas
			// pontas olha só para o lado que existe: uma diferença com o
			// próprio ponto daria vetor nulo, e a calha fecharia num bico.
			const FVector2D Antes = Curso.PointsUnits[FMath::Max(Ponto - 1, 0)];
			const FVector2D Depois = Curso.PointsUnits[FMath::Min(Ponto + 1, Pontos - 1)];

			FVector2D Rumo = Depois - Antes;
			if (Rumo.IsNearlyZero())
			{
				Rumo = FVector2D(1.0f, 0.0f);
			}
			Rumo.Normalize();

			const FVector2D Lado(-Rumo.Y, Rumo.X);
			const float Meia = Curso.HalfWidthUnits.IsValidIndex(Ponto)
				? Curso.HalfWidthUnits[Ponto]
				: 0.0f;
			BuiltHalfWidths.Add(Meia);

			const FVector2D Esquerda = Aqui - Lado * Meia;
			const FVector2D Direita = Aqui + Lado * Meia;

			// A lâmina segue o CHÃO ponto a ponto. Uma altura só para o curso
			// inteiro poria a foz enterrada ou a cabeceira no ar.
			Vertices.Add(FVector(Esquerda.X, Esquerda.Y,
				Assado.HeightAt(Esquerda) + AguaCorrente::ElevacaoDaLamina));
			Vertices.Add(FVector(Direita.X, Direita.Y,
				Assado.HeightAt(Direita) + AguaCorrente::ElevacaoDaLamina));

			const float AoLongo = static_cast<float>(Ponto) / (Pontos - 1);
			Uvs.Add(FVector2D(0.0f, AoLongo));
			Uvs.Add(FVector2D(1.0f, AoLongo));
		}

		for (int32 Ponto = 0; Ponto + 1 < Pontos; ++Ponto)
		{
			const int32 EsquerdaAqui = Ponto * 2;
			const int32 DireitaAqui = EsquerdaAqui + 1;
			const int32 EsquerdaAdiante = EsquerdaAqui + 2;
			const int32 DireitaAdiante = EsquerdaAqui + 3;

			Triangulos.Add(EsquerdaAqui);
			Triangulos.Add(EsquerdaAdiante);
			Triangulos.Add(DireitaAqui);

			Triangulos.Add(DireitaAqui);
			Triangulos.Add(EsquerdaAdiante);
			Triangulos.Add(DireitaAdiante);
		}

		// O LAGO e o POÇO DA QUEDA entram na mesma seção do curso a que
		// pertencem. Seção própria por remanso multiplicaria as seções por
		// três para desenhar água da mesma cor no mesmo lugar.
		//
		// O booleano é perguntado ANTES da posição: nem todo curso alaga nem
		// despenca, e ler a posição sem checar já pôs uma trilha mirando o mar
		// aberto neste projeto.
		if (Curso.bHasLake)
		{
			AguaCorrente::MontarDisco(Assado, Curso.LakeCenterUnits,
				FreshWater::LakeHalfWidthUnits(), Vertices, Triangulos, Uvs);
			++BuiltLakeCount;
		}

		if (Curso.bHasFall && Curso.PlungePoolHalfWidthUnits > 0.0f)
		{
			AguaCorrente::MontarDisco(Assado, Curso.FallCenterUnits,
				Curso.PlungePoolHalfWidthUnits, Vertices, Triangulos, Uvs);
			++BuiltPlungePoolCount;
		}

		Water->CreateMeshSection(Secao, Vertices, Triangulos, TArray<FVector>(), Uvs,
			TArray<FColor>(), TArray<FProcMeshTangent>(), false);
		++Secao;
		++BuiltCourseCount;
	}

	// Pinta DEPOIS de as seções existirem: pintar antes acha zero slot e
	// devolve zero, que é a pintura silenciosa que não pinta nada.
	ScenaryPalette::PaintComponent(Water, EScenaryRole::FreshWater);
	if (UMaterialInterface* Tinta = Water->GetMaterial(0))
	{
		for (int32 Qual = 1; Qual < Secao; ++Qual)
		{
			Water->SetMaterial(Qual, Tinta);
		}
	}

	FBattleDebugScreen::Show(
		FString::Printf(
			TEXT("MUNDO: %d cursos d'agua, %d lagos, %d pocos de queda"),
			BuiltCourseCount, BuiltLakeCount, BuiltPlungePoolCount),
		30.0f, FColor(120, 180, 255), AguaCorrente::ChaveDoPainel);

	return BuiltCourseCount;
}

void ARiverMesh::BeginPlay()
{
	Super::BeginPlay();

	if (const UIslandBakedPlan* Assado = IslandBakedPlan::LoadForWorld())
	{
		BuildFrom(*Assado);
	}
}
