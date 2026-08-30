// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ForestBackdrop.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/ScenaryPalette.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace MataDoCenario
{
	/** Onde o pacote de mata (Kenney Nature Kit, CC0) foi importado. */
	const TCHAR* PastaDaMata = TEXT("/Game/Environment/Nature/");

	/** Primitiva do disco de chão — conteúdo da engine, não vendorizado. */
	const TCHAR* CilindroDaEngine = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** Lado da primitiva da engine, em unidades de mundo. */
	constexpr float CilindroDaEngineUnidades = 100.0f;

	/** Raio do disco de chão, em casas — cobre o que a câmera alcança. */
	constexpr float RaioDoChaoEmCasas = 30.0f;

	/** Espessura do disco: fino, mas acima do plano do template. */
	constexpr float EspessuraDoChao = 4.0f;

	/** Quantas vezes tentar reposicionar uma planta antes de desistir dela. */
	constexpr int32 TentativasPorPlanta = 8;

	/**
	 * Faixa em que a espécie nasce, em casas do tabuleiro.
	 *
	 * O enquadramento é um diorama: o chão só é visível numa coroa estreita
	 * atrás da grade, e o que estiver mais longe sobe para fora do topo da
	 * tela. Por isso a rasteira fica perto (ela precisa ser vista no chão) e
	 * o dossel fica na faixa em que o TRONCO cruza o topo do quadro — é ele
	 * que fecha o fundo.
	 */
	struct FFaixa
	{
		float RaioMinimoEmCasas;
		float RaioMaximoEmCasas;
	};

	constexpr FFaixa Rasteira{ 2.3f, 6.0f };
	constexpr FFaixa Borda{ 2.6f, 7.5f };
	constexpr FFaixa Mata{ 3.4f, 9.0f };
	constexpr FFaixa Dossel{ 5.0f, 16.0f };

	/**
	 * Uma espécie da mata.
	 *
	 * `AlturaEmCasas` é a altura DESEJADA; a escala sai dela dividida pela
	 * caixa medida da malha. Escala fixa por espécie seria um número mágico
	 * contra uma malha que pode ser trocada — e trocar a malha calaria o
	 * defeito em vez de corrigi-lo.
	 */
	struct FEspecie
	{
		const TCHAR* Nome;
		FFaixa Faixa;
		float AlturaEmCasas;
		int32 Quantidade;
		/**
		 * O que esta espécie precisa DIZER na tela.
		 *
		 * Não sai da malha nem do material: `plant_bush` e `grass_large`
		 * chegam do pacote com o mesmo slot `grass`, e enquanto a cor vinha
		 * de lá o arbusto e o capim eram o mesmo verde. É esta coluna que os
		 * separa.
		 */
		EScenaryRole Papel;
	};

	const FEspecie Especies[] = {
		// Rasteira: o que dá textura ao chão logo atrás da grade.
		// Não existe malha "grass": o kit traz um MATERIAL com esse nome, e
		// ele ficou com o slot do asset na importação. As duas abaixo cobrem
		// o mesmo papel de capim rasteiro.
		{ TEXT("grass_large"),             Rasteira, 0.42f, 56, EScenaryRole::GroundCover },
		{ TEXT("grass_leafs"),             Rasteira, 0.26f, 54, EScenaryRole::GroundCover },
		{ TEXT("flower_redA"),             Rasteira, 0.24f, 14, EScenaryRole::Accent },
		{ TEXT("flower_yellowA"),          Rasteira, 0.20f, 14, EScenaryRole::Accent },
		{ TEXT("mushroom_red"),            Rasteira, 0.18f, 10, EScenaryRole::Accent },

		// Borda: o volume baixo que separa o tabuleiro da mata.
		{ TEXT("plant_bushSmall"),         Borda,    0.40f, 20, EScenaryRole::Undergrowth },
		{ TEXT("plant_bush"),              Borda,    0.55f, 18, EScenaryRole::Undergrowth },
		{ TEXT("plant_bushLarge"),         Borda,    0.62f, 16, EScenaryRole::Undergrowth },
		{ TEXT("stump_round"),             Borda,    0.38f,  8, EScenaryRole::DeadWood },
		{ TEXT("log"),                     Borda,    0.34f,  8, EScenaryRole::DeadWood },
		{ TEXT("rock_smallA"),             Borda,    0.32f, 12, EScenaryRole::Rock },
		{ TEXT("rock_smallD"),             Borda,    0.38f, 12, EScenaryRole::Rock },

		// Mata: primeiro plano de árvore, ainda inteira no quadro.
		{ TEXT("rock_largeA"),             Mata,     0.58f,  8, EScenaryRole::Rock },
		{ TEXT("rock_largeC"),             Mata,     0.70f,  6, EScenaryRole::Rock },
		{ TEXT("rock_tallD"),              Mata,     1.30f,  6, EScenaryRole::Rock },
		{ TEXT("rock_tallA"),              Mata,     1.80f,  4, EScenaryRole::Rock },
		{ TEXT("tree_pineSmallA"),         Mata,     1.70f, 12, EScenaryRole::ForestTree },
		{ TEXT("tree_pineSmallC"),         Mata,     1.90f, 12, EScenaryRole::ForestTree },
		{ TEXT("tree_oak"),                Mata,     2.30f, 10, EScenaryRole::ForestTree },
		{ TEXT("tree_blocks"),             Mata,     2.10f,  8, EScenaryRole::ForestTree },

		// Dossel: o paredão que fecha o fundo.
		{ TEXT("tree_pineRoundC"),         Dossel,   2.50f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineRoundA"),         Dossel,   2.70f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_default"),            Dossel,   2.90f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_thin"),               Dossel,   3.20f, 10, EScenaryRole::CanopyTree },
		{ TEXT("tree_tall"),               Dossel,   3.40f, 10, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineTallA_detailed"), Dossel,   3.50f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineTallB_detailed"), Dossel,   3.90f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineTallC_detailed"), Dossel,   3.70f, 12, EScenaryRole::CanopyTree },
	};

	constexpr int32 TotalDeEspecies = UE_ARRAY_COUNT(Especies);

	/** Fluxos de sorteio de uma planta — um índice por grandeza sorteada. */
	enum class ESorteio : int32
	{
		Angulo = 0,
		Raio = 1,
		Giro = 2,
		Tamanho = 3,
		Inclinacao = 4,
		PorPlanta = 5
	};

	int32 FluxoDaPlanta(int32 Planta, int32 Tentativa, ESorteio Qual)
	{
		const int32 PorTentativa = static_cast<int32>(ESorteio::PorPlanta);
		return (Planta * TentativasPorPlanta + Tentativa) * PorTentativa + static_cast<int32>(Qual);
	}
}

AForestBackdrop::AForestBackdrop()
{
	using namespace MataDoCenario;

	PrimaryActorTick.bCanEverTick = false;

	ForestRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ForestRoot"));
	SetRootComponent(ForestRoot);

	GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForestGround"));
	GroundMesh->SetupAttachment(ForestRoot);
	// Cenário não empurra ninguém: quem decide onde o pet está é o núcleo.
	GroundMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CilindroDoChao(CilindroDaEngine);
	if (CilindroDoChao.Succeeded())
	{
		GroundMesh->SetStaticMesh(CilindroDoChao.Object);
	}

	SpeciesClusters.Reset();
	for (int32 Indice = 0; Indice < TotalDeEspecies; ++Indice)
	{
		const FEspecie& Especie = Especies[Indice];

		UHierarchicalInstancedStaticMeshComponent* Grupo =
			CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
				FName(*FString::Printf(TEXT("Species_%s"), Especie.Nome)));
		Grupo->SetupAttachment(ForestRoot);
		Grupo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Grupo->SetCastShadow(true);

		// A malha é atribuída AQUI, no construtor. Componente criado sem
		// asset passa em todo teste de lógica e não existe na tela — foi
		// assim três vezes neste projeto (pets, inimigos do mundo, o
		// próprio jogador).
		const FString Caminho = FString::Printf(TEXT("%s%s.%s"), PastaDaMata, Especie.Nome, Especie.Nome);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Malha(*Caminho);
		if (Malha.Succeeded())
		{
			Grupo->SetStaticMesh(Malha.Object);
		}

		SpeciesClusters.Add(Grupo);
	}
}

void AForestBackdrop::BuildForest(float CellSize, uint32 Seed, const FVector2D& CameraGroundOffset)
{
	using namespace MataDoCenario;

	if (CellSize <= 0.0f)
	{
		return;
	}

	const float RaioDoChao = RaioDoChaoEmCasas * CellSize;
	GroundMesh->SetRelativeScale3D(FVector(
		RaioDoChao * 2.0f / CilindroDaEngineUnidades,
		RaioDoChao * 2.0f / CilindroDaEngineUnidades,
		EspessuraDoChao / CilindroDaEngineUnidades));
	GroundMesh->SetRelativeLocation(FVector::ZeroVector);

	ApplyGroundMaterial();

	const float FolgaDoTabuleiro = BoardClearanceInCells * CellSize;
	const float FolgaDaCamera = CameraClearanceInCells * CellSize;

	for (int32 Indice = 0; Indice < TotalDeEspecies && Indice < SpeciesClusters.Num(); ++Indice)
	{
		const FEspecie& Especie = Especies[Indice];
		UHierarchicalInstancedStaticMeshComponent* Grupo = SpeciesClusters[Indice];

		Grupo->ClearInstances();

		UStaticMesh* Malha = Grupo->GetStaticMesh();
		if (!Malha)
		{
			continue;
		}

		// A escala sai da caixa MEDIDA: pedir "3 casas de altura" e dividir
		// pela altura real da malha é o que mantém o conjunto coerente
		// quando as espécies vêm em tamanhos diferentes.
		const float AlturaDaMalha = Malha->GetBoundingBox().GetSize().Z;
		if (AlturaDaMalha <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const float EscalaBase = (Especie.AlturaEmCasas * CellSize) / AlturaDaMalha;

		// A cor vem da NOSSA paleta, não do pacote. Os dez materiais do kit
		// são variações de uma faixa estreita: trocar um pelo outro moveria o
		// problema em vez de resolvê-lo. Aqui capim, arbusto, árvore, dossel
		// e pedra recebem degraus de BRILHO distintos, que é o que se lê de
		// longe.
		ScenaryPalette::PaintComponent(Grupo, Especie.Papel);

		const uint32 SementeDaEspecie = BattleSpread::Scatter(Seed ^ BattleSpread::SeedFromText(Especie.Nome));

		for (int32 Planta = 0; Planta < Especie.Quantidade; ++Planta)
		{
			for (int32 Tentativa = 0; Tentativa < TentativasPorPlanta; ++Tentativa)
			{
				const float Angulo = BattleSpread::Fraction(
					SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Angulo)) * 2.0f * PI;

				// Raio pela raiz: sem ela a densidade se acumula no centro da
				// coroa e a mata fica com um anel visível.
				const float Sorteio = BattleSpread::Fraction(
					SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Raio));
				const float RaioMinimo = Especie.Faixa.RaioMinimoEmCasas * CellSize;
				const float RaioMaximo = Especie.Faixa.RaioMaximoEmCasas * CellSize;
				const float Raio = FMath::Sqrt(BattleSpread::Between(
					RaioMinimo * RaioMinimo, RaioMaximo * RaioMaximo, Sorteio));

				const FVector2D Posicao(Raio * FMath::Cos(Angulo), Raio * FMath::Sin(Angulo));

				if (Posicao.Size() < FolgaDoTabuleiro)
				{
					continue;
				}
				if (FVector2D::Distance(Posicao, CameraGroundOffset) < FolgaDaCamera)
				{
					continue;
				}

				const float Giro = BattleSpread::Fraction(
					SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Giro)) * 360.0f;
				const float Tamanho = EscalaBase * BattleSpread::Between(0.78f, 1.28f,
					BattleSpread::Fraction(SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Tamanho)));
				const float Inclinacao = BattleSpread::Between(-4.0f, 4.0f,
					BattleSpread::Fraction(SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Inclinacao)));

				const FTransform Onde(
					FRotator(Inclinacao, Giro, 0.0f),
					FVector(Posicao.X, Posicao.Y, EspessuraDoChao * 0.5f),
					FVector(Tamanho));
				Grupo->AddInstance(Onde);
				break;
			}
		}
	}
}

int32 AForestBackdrop::GetPlantedCount() const
{
	int32 Total = 0;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : SpeciesClusters)
	{
		if (Grupo)
		{
			Total += Grupo->GetInstanceCount();
		}
	}
	return Total;
}

TArray<FVector> AForestBackdrop::GetPlantedLocations() const
{
	TArray<FVector> Posicoes;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : SpeciesClusters)
	{
		if (!Grupo)
		{
			continue;
		}
		const int32 Quantas = Grupo->GetInstanceCount();
		for (int32 Instancia = 0; Instancia < Quantas; ++Instancia)
		{
			FTransform Onde;
			if (Grupo->GetInstanceTransform(Instancia, Onde, /*bWorldSpace=*/false))
			{
				Posicoes.Add(Onde.GetLocation());
			}
		}
	}
	return Posicoes;
}

float AForestBackdrop::GroundTopLocalZ()
{
	return MataDoCenario::EspessuraDoChao * 0.5f;
}

void AForestBackdrop::SetGroundMaterialOverride(UMaterialInterface* Material)
{
	AdoptedGroundMaterial = Material;

	// Aplicar JÁ, e não só na próxima construção: a arena empresta o chão
	// depois de sondar o mundo, que é depois de a mata estar plantada.
	ApplyGroundMaterial();
}

void AForestBackdrop::ApplyGroundMaterial()
{
	if (!GroundMesh)
	{
		return;
	}

	// O que o mundo emprestou vem primeiro: a batalha que nasce de um
	// encontro deve acontecer no chão daquele lugar, não numa paleta à parte.
	if (UMaterialInterface* Emprestado = AdoptedGroundMaterial.Get())
	{
		GroundMesh->SetMaterial(0, Emprestado);
		return;
	}

	// Sem empréstimo, o chão sai da NOSSA paleta.
	//
	// Ele já foi vestido com `leafsGreen` — o mesmo material das folhas das
	// árvores. Chão e copa eram literalmente uma cor só, e o quadro inteiro
	// lia como uma mancha verde-água sem relevo. O chão agora é o degrau
	// mais escuro da escala, justamente para o resto se destacar contra ele.
	if (UMaterialInterface* Base = ScenaryPalette::ColorableBaseMaterial())
	{
		if (UMaterialInstanceDynamic* Tinta = GroundMesh->CreateDynamicMaterialInstance(0, Base))
		{
			Tinta->SetVectorParameterValue(TEXT("Color"), ScenaryPalette::GroundColor());
		}
	}
}
