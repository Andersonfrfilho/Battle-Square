// Copyright 2026 Anderson. All Rights Reserved.

#include "World/MountView.h"
#include "Environment/ScenaryPalette.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace MontariaSilhueta
{
	constexpr float ComprimentoDoCorpo = 150.0f;
	constexpr float LarguraDoCorpo = 70.0f;
	constexpr float AlturaDoCorpo = 90.0f;

	/** A cor padrão do montado — já no construtor, nunca cinza sem asset. */
	const FLinearColor CorPadrao = FLinearColor(0.55f, 0.42f, 0.30f);
}

AMountView::AMountView()
{
	PrimaryActorTick.bCanEverTick = false;

	MountRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MountRoot"));
	SetRootComponent(MountRoot);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(MountRoot);

	// A MALHA NO CONSTRUTOR — a regra três vezes paga. Componente sem asset
	// passa em toda bateria e não existe na tela.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(
		ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cylinder));
	if (Cilindro.Succeeded())
	{
		Body->SetStaticMesh(Cilindro.Object);
	}

	// Deitado: a montaria é mais comprida que alta, para ler como bicho de
	// carga e não como o morador em pé.
	Body->SetRelativeScale3D(FVector(
		MontariaSilhueta::ComprimentoDoCorpo / 100.0f,
		MontariaSilhueta::LarguraDoCorpo / 100.0f,
		MontariaSilhueta::AlturaDoCorpo / 100.0f));
	Body->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// A COR TAMBÉM NO CONSTRUTOR: sem cor atribuída, o material padrão da engine
	// entra e o montado nasce cinza — indistinguível do que não foi pintado.
	SetMountColor(MontariaSilhueta::CorPadrao);
}

void AMountView::SetMountColor(const FLinearColor& Color)
{
	if (!Body)
	{
		return;
	}
	UMaterialInterface* Base = Body->GetMaterial(0);
	UMaterialInstanceDynamic* Tinta = Base
		? UMaterialInstanceDynamic::Create(Base, this)
		: nullptr;
	if (Tinta)
	{
		Tinta->SetVectorParameterValue(TEXT("Color"), Color);
		Body->SetMaterial(0, Tinta);
	}
}
