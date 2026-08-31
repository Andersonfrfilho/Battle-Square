// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldTrainingField.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/ForestBackdrop.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace CampoDeTreino
{
	/** Alta o bastante para se ver de longe, baixa o bastante para não ser parede. */
	constexpr float AlturaDoMarcoUnidades = 300.0f;

	/** A peça da borda é do tamanho de um arbusto: ela delimita, não fecha. */
	constexpr float AlturaDaPecaDaBordaUnidades = 95.0f;

	/**
	 * Quantas peças no anel.
	 *
	 * Dezoito num raio de 400 dá uma peça a cada ~140 unidades: o olho fecha o
	 * círculo e ainda passa entre elas. Um anel cheio seria cerca, e cerca em
	 * volta do treino diria "não entre" justamente onde é para entrar.
	 */
	constexpr int32 PecasNaBorda = 18;

	/** Desvio do raio, para o anel não sair um círculo de compasso. */
	constexpr float DesvioDaBordaUnidades = 34.0f;

	/**
	 * Pinta um componente com a cor do ATRIBUTO, não com a do papel.
	 *
	 * Reaproveita o material base da paleta em vez de nomear a primitiva da
	 * engine outra vez: no dia em que a paleta trocar de material, este campo
	 * troca junto (L-032).
	 */
	int32 PintarComACorDoAtributo(UMeshComponent* Componente, const FLinearColor& Cor)
	{
		UMaterialInterface* Base = ScenaryPalette::ColorableBaseMaterial();
		if (!Componente || !Base)
		{
			return 0;
		}

		int32 Pintados = 0;
		const int32 Slots = Componente->GetNumMaterials();
		for (int32 Slot = 0; Slot < Slots; ++Slot)
		{
			Componente->SetMaterial(Slot, Base);
			if (UMaterialInstanceDynamic* Tinta = Componente->CreateDynamicMaterialInstance(Slot))
			{
				Tinta->SetVectorParameterValue(TEXT("Color"), Cor);
				++Pintados;
			}
		}

		return Pintados;
	}
}

AWorldTrainingField::AWorldTrainingField()
{
	PrimaryActorTick.bCanEverTick = false;

	FieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FieldMesh"));
	SetRootComponent(FieldMesh);

	// Sem colisão: é marco de lugar, não obstáculo. Um campo que empurra o
	// jogador transformaria o destino em armadilha.
	FieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BorderProps = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("BorderProps"));
	BorderProps->SetupAttachment(FieldMesh);
	BorderProps->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// MALHAS ATRIBUÍDAS AQUI, as duas. Quarta vez que este projeto escreve
	// esta linha, e pelo mesmo motivo das três anteriores: ator com componente
	// visual e sem asset passa em todo teste de lógica e não existe na tela.
	//
	// O padrão do CDO é o do atributo padrão. Assim o ator colocado à mão no
	// editor já nasce parecendo alguma coisa, antes de qualquer `BeginPlay`.
	const FTrainingFieldMarker Padrao = MarkerForAttribute(TrainedAttribute);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MalhaDoMarco(
		*AForestBackdrop::SpeciesAssetPath(Padrao.MarkerSpecies));
	if (MalhaDoMarco.Succeeded())
	{
		FieldMesh->SetStaticMesh(MalhaDoMarco.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MalhaDaBorda(
		*AForestBackdrop::SpeciesAssetPath(Padrao.BorderSpecies));
	if (MalhaDaBorda.Succeeded())
	{
		BorderProps->SetStaticMesh(MalhaDaBorda.Object);
	}
}

void AWorldTrainingField::BeginPlay()
{
	Super::BeginPlay();

	RebuildMarker();
}

void AWorldTrainingField::RebuildMarker()
{
	const FTrainingFieldMarker Marco = MarkerForAttribute(TrainedAttribute);

	ApplyCenterMarker(Marco);
	ApplyBorderRing(Marco);
}

void AWorldTrainingField::ApplyCenterMarker(const FTrainingFieldMarker& Marker)
{
	if (!FieldMesh)
	{
		return;
	}

	if (UStaticMesh* Malha = LoadObject<UStaticMesh>(
			nullptr, *AForestBackdrop::SpeciesAssetPath(Marker.MarkerSpecies)))
	{
		FieldMesh->SetStaticMesh(Malha);
	}

	UStaticMesh* Atual = FieldMesh->GetStaticMesh();
	const float AlturaDaMalha = Atual ? Atual->GetBoundingBox().GetSize().Z : 0.0f;
	if (AlturaDaMalha <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// A escala sai da caixa MEDIDA, como na mata: pedir uma altura em unidades
	// e dividir pela altura real é o que mantém marco de pedra e marco de
	// árvore do mesmo tamanho na tela.
	const float Escala = CampoDeTreino::AlturaDoMarcoUnidades / AlturaDaMalha;
	FieldMesh->SetRelativeScale3D(FVector(Escala));

	// O marco do CENTRO usa a cor do papel dele, não a do atributo: pedra
	// pintada de azul-aço não é pedra, e o campo tem que parecer parte da mata
	// em que está. Quem carrega a cor do atributo é o anel.
	ScenaryPalette::PaintComponent(FieldMesh, Marker.MarkerRole);
}

void AWorldTrainingField::ApplyBorderRing(const FTrainingFieldMarker& Marker)
{
	if (!BorderProps)
	{
		return;
	}

	// LIMPA ANTES. Esta função pode correr duas vezes — `BeginPlay` e depois
	// quem atribuiu o atributo — e sem isto o segundo anel nasceria por cima
	// do primeiro, com o dobro das peças e metade delas da espécie errada.
	BorderProps->ClearInstances();

	if (UStaticMesh* Malha = LoadObject<UStaticMesh>(
			nullptr, *AForestBackdrop::SpeciesAssetPath(Marker.BorderSpecies)))
	{
		BorderProps->SetStaticMesh(Malha);
	}

	UStaticMesh* Atual = BorderProps->GetStaticMesh();
	const float AlturaDaMalha = Atual ? Atual->GetBoundingBox().GetSize().Z : 0.0f;
	if (AlturaDaMalha <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Escala = CampoDeTreino::AlturaDaPecaDaBordaUnidades / AlturaDaMalha;

	// Semente do NOME do atributo: o mesmo campo é sempre o mesmo campo, em
	// qualquer partida. Aparência, não regra — por isso vive fora do BattleSim.
	const uint32 Semente = BattleSpread::SeedFromText(TrainedAttribute);

	for (int32 Peca = 0; Peca < CampoDeTreino::PecasNaBorda; ++Peca)
	{
		const float Volta = static_cast<float>(Peca) / static_cast<float>(CampoDeTreino::PecasNaBorda);
		const float Desvio = BattleSpread::Between(
			-CampoDeTreino::DesvioDaBordaUnidades,
			CampoDeTreino::DesvioDaBordaUnidades,
			BattleSpread::Fraction(Semente, Peca));

		const float Angulo = 2.0f * PI * Volta;
		const float Raio = FieldRadiusUnits + Desvio;

		FTransform Pouso;
		Pouso.SetLocation(FVector(FMath::Cos(Angulo) * Raio, FMath::Sin(Angulo) * Raio, 0.0f));
		Pouso.SetRotation(FQuat(FRotator(
			0.0f, BattleSpread::Between(0.0f, 360.0f, BattleSpread::Fraction(Semente, Peca + 97)), 0.0f)));
		Pouso.SetScale3D(FVector(Escala * BattleSpread::Between(
			0.82f, 1.24f, BattleSpread::Fraction(Semente, Peca + 211))));

		BorderProps->AddInstance(Pouso);
	}

	// O ANEL é quem carrega a cor do atributo. Era o disco inteiro; agora são
	// dezoito peças pequenas, e a informação é a mesma sem a elipse chapada.
	CampoDeTreino::PintarComACorDoAtributo(BorderProps, ColorForAttribute(TrainedAttribute));
}

FTrainingFieldMarker AWorldTrainingField::MarkerForAttribute(const FString& Attribute)
{
	// TODAS as peças saem do elenco da FLORESTA (`BiomeFlora`), porque é nela
	// que os campos nascem. Escolher uma agulha de pedra de vulcão para o campo
	// de voo poria no meio da mata a única peça que a mata não planta — e a
	// clareira voltaria a parecer colada de fora, que é o defeito de origem.
	if (Attribute == TEXT("musculature"))
	{
		return { TEXT("rock_largeA"), TEXT("rock_smallD"), EScenaryRole::Rock };
	}
	if (Attribute == TEXT("personality"))
	{
		return { TEXT("stump_round"), TEXT("flower_yellowA"), EScenaryRole::DeadWood };
	}
	if (Attribute == TEXT("camouflage"))
	{
		return { TEXT("plant_bushLarge"), TEXT("plant_bush"), EScenaryRole::Undergrowth };
	}
	if (Attribute == TEXT("flight"))
	{
		// Árvore alta e não agulha de pedra: de onde se salta aqui é de um galho.
		return { TEXT("tree_tall"), TEXT("rock_smallA"), EScenaryRole::ForestTree };
	}
	if (Attribute == TEXT("underground"))
	{
		return { TEXT("rock_largeC"), TEXT("mushroom_red"), EScenaryRole::Rock };
	}

	// Peça existe até para o desconhecido — marco invisível seria campo que não
	// está na tela. Quem denuncia o erro é a COR, que vem cinza.
	return { TEXT("rock_largeA"), TEXT("rock_smallD"), EScenaryRole::Rock };
}

FLinearColor AWorldTrainingField::ColorForAttribute(const FString& Attribute)
{
	// Uma cor por atributo, e sempre a MESMA: o jogador aprende o lugar pela
	// cor antes de chegar perto o bastante para ler qualquer texto.
	//
	// CRIATURA É VIVA, TERRENO É TERRA. Estas cinco são deliberadamente
	// apagadas, e a regra vale para todo campo futuro: um pet e o chão em que
	// ele pisa podem partilhar a matiz — o que os separa é que um está vivo.
	//
	// A primeira versão não obedecia a isso, e o resultado era concreto: o
	// verde da camuflagem (#33B240) e o do pet de Planta (#33C740) ficavam a
	// cinco pontos de distância, e um pet de Planta parado no disco de
	// camuflagem virava duas coisas da mesma cor querendo dizer coisas
	// diferentes. O mesmo com o azul do voo contra o do pet de Água.
	if (Attribute == TEXT("musculature")) { return FLinearColor(0.549f, 0.290f, 0.259f); }
	if (Attribute == TEXT("personality")) { return FLinearColor(0.612f, 0.518f, 0.259f); }
	if (Attribute == TEXT("camouflage"))  { return FLinearColor(0.310f, 0.478f, 0.322f); }
	if (Attribute == TEXT("flight"))      { return FLinearColor(0.369f, 0.494f, 0.588f); }
	if (Attribute == TEXT("underground")) { return FLinearColor(0.420f, 0.325f, 0.251f); }

	// Cinza para o desconhecido, em vez de uma cor bonita qualquer: campo mal
	// configurado precisa PARECER mal configurado.
	return FLinearColor(0.5f, 0.5f, 0.5f);
}

bool AWorldTrainingField::IsInside(const FVector& WorldLocation) const
{
	// Distância no PLANO: a altura não conta. Um jogador pulando dentro do
	// campo continua no campo, e exigir que ele encoste no chão faria o treino
	// piscar a cada salto.
	const FVector Delta = WorldLocation - GetActorLocation();
	return FVector2D(Delta.X, Delta.Y).Size() <= FieldRadiusUnits;
}
