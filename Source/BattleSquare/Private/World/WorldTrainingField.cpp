// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldTrainingField.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** O cilindro da engine tem 100 unidades de lado. */
	constexpr float CilindroDaEngineUnidades = 100.0f;

	/** Baixo: é uma clareira marcada no chão, não uma parede. */
	constexpr float AlturaDoDiscoUnidades = 12.0f;
}

AWorldTrainingField::AWorldTrainingField()
{
	PrimaryActorTick.bCanEverTick = false;

	FieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FieldMesh"));
	SetRootComponent(FieldMesh);

	// Sem colisão: é chão pintado, não obstáculo. Um campo que empurra o
	// jogador transformaria o destino em armadilha.
	FieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// MALHA ATRIBUÍDA AQUI. Quarta vez que este projeto escreve esta linha, e
	// pelo mesmo motivo das três anteriores: ator com componente visual e sem
	// asset passa em todo teste de lógica e não existe na tela.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DiscoMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DiscoMesh.Succeeded())
	{
		FieldMesh->SetStaticMesh(DiscoMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DiscoMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (DiscoMaterial.Succeeded())
	{
		FieldMesh->SetMaterial(0, DiscoMaterial.Object);
	}
}

void AWorldTrainingField::BeginPlay()
{
	Super::BeginPlay();

	// A escala segue o RAIO declarado: um disco que não bate com a área que
	// treina ensinaria a coisa errada sobre onde ficar.
	FieldMesh->SetRelativeScale3D(FVector(
		FieldRadiusUnits * 2.0f / CilindroDaEngineUnidades,
		FieldRadiusUnits * 2.0f / CilindroDaEngineUnidades,
		AlturaDoDiscoUnidades / CilindroDaEngineUnidades));

	if (UMaterialInstanceDynamic* Material = FieldMesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), ColorForAttribute(TrainedAttribute));
	}
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
