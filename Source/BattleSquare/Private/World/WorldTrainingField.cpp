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
	if (Attribute == TEXT("musculature")) { return FLinearColor(0.85f, 0.20f, 0.15f); }
	if (Attribute == TEXT("personality")) { return FLinearColor(0.90f, 0.65f, 0.10f); }
	if (Attribute == TEXT("camouflage"))  { return FLinearColor(0.20f, 0.70f, 0.25f); }
	if (Attribute == TEXT("flight"))      { return FLinearColor(0.35f, 0.65f, 0.95f); }
	if (Attribute == TEXT("underground")) { return FLinearColor(0.55f, 0.35f, 0.20f); }

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
