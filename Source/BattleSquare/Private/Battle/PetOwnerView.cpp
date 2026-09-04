// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetOwnerView.h"
#include "Environment/ScenaryPalette.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace DonoDoPet
{
	/** Aresta da primitiva da engine. Toda escala abaixo é fração disto. */
	constexpr float PrimitivaDaEngine = 100.0f;

	constexpr float AlturaDasPernas = 46.0f;
	constexpr float AlturaDoTronco = 52.0f;
	constexpr float DiametroDaCabeca = 30.0f;
	constexpr float AlturaDoChapeu = 22.0f;

	/** O chapéu ENTRA na cabeça, senão sobra costura entre os dois. */
	constexpr float EncaixeDoChapeu = 6.0f;

	constexpr float LarguraDasPernas = 26.0f;
	constexpr float LarguraDoTronco = 36.0f;
	constexpr float DiametroDoChapeu = 34.0f;

	const FLinearColor CorDaCalca(0.18f, 0.16f, 0.20f);
	const FLinearColor CorDaPele(0.86f, 0.68f, 0.53f);

	/** O chapéu é a cor do lado escurecida: dois tons do mesmo time. */
	constexpr float EscurecimentoDoChapeu = 0.55f;

	// Nome próprio (L-042): "Pintar" já existe no anônimo de PetView.cpp, e o
	// unity build junta os dois arquivos numa unidade só.
	void PintarDono(UStaticMeshComponent* Componente, const FLinearColor& Cor)
	{
		if (!Componente)
		{
			return;
		}

		UMaterialInstanceDynamic* Dinamico = Cast<UMaterialInstanceDynamic>(Componente->GetMaterial(0));
		if (!Dinamico)
		{
			Dinamico = Componente->CreateDynamicMaterialInstance(0);
		}
		if (Dinamico)
		{
			Dinamico->SetVectorParameterValue(TEXT("Color"), Cor);
		}
	}
}

APetOwnerView::APetOwnerView()
{
	using namespace DonoDoPet;

	PrimaryActorTick.bCanEverTick = false;

	OwnerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OwnerRoot"));
	SetRootComponent(OwnerRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cylinder));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Sphere));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cone));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialBasico(
		ScenaryPalette::ColorableBaseMaterialPath());

	LegsMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Legs"));
	LegsMesh->SetupAttachment(OwnerRoot);
	LegsMesh->SetRelativeScale3D(FVector(
		LarguraDasPernas / PrimitivaDaEngine,
		LarguraDasPernas / PrimitivaDaEngine,
		AlturaDasPernas / PrimitivaDaEngine));
	LegsMesh->SetRelativeLocation(FVector(0.0f, 0.0f, AlturaDasPernas * 0.5f));

	TorsoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Torso"));
	TorsoMesh->SetupAttachment(OwnerRoot);
	TorsoMesh->SetRelativeScale3D(FVector(
		LarguraDoTronco / PrimitivaDaEngine,
		LarguraDoTronco / PrimitivaDaEngine,
		AlturaDoTronco / PrimitivaDaEngine));
	TorsoMesh->SetRelativeLocation(FVector(0.0f, 0.0f, AlturaDasPernas + AlturaDoTronco * 0.5f));

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	HeadMesh->SetupAttachment(OwnerRoot);
	HeadMesh->SetRelativeScale3D(FVector(DiametroDaCabeca / PrimitivaDaEngine));
	HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f,
		AlturaDasPernas + AlturaDoTronco + DiametroDaCabeca * 0.5f));

	HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
	HatMesh->SetupAttachment(OwnerRoot);
	HatMesh->SetRelativeScale3D(FVector(
		DiametroDoChapeu / PrimitivaDaEngine,
		DiametroDoChapeu / PrimitivaDaEngine,
		AlturaDoChapeu / PrimitivaDaEngine));
	HatMesh->SetRelativeLocation(FVector(0.0f, 0.0f,
		AlturaDasPernas + AlturaDoTronco + DiametroDaCabeca - EncaixeDoChapeu));

	// A malha é atribuída AQUI, no construtor: componente criado sem asset
	// passa em todo teste de lógica e não existe na tela.
	if (Cilindro.Succeeded())
	{
		LegsMesh->SetStaticMesh(Cilindro.Object);
		TorsoMesh->SetStaticMesh(Cilindro.Object);
	}
	if (Esfera.Succeeded())
	{
		HeadMesh->SetStaticMesh(Esfera.Object);
	}
	if (Cone.Succeeded())
	{
		HatMesh->SetStaticMesh(Cone.Object);
	}

	if (MaterialBasico.Succeeded())
	{
		for (UStaticMeshComponent* Parte : { LegsMesh.Get(), TorsoMesh.Get(), HeadMesh.Get(), HatMesh.Get() })
		{
			Parte->SetMaterial(0, MaterialBasico.Object);
			// Cenário não empurra ninguém: quem decide posição é o núcleo.
			Parte->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void APetOwnerView::SetSide(uint8 InSide)
{
	using namespace DonoDoPet;

	Side = InSide;

	const FLinearColor CorDoLado = Side == 0 ? LocalSideColor : OpponentSideColor;

	PintarDono(LegsMesh, CorDaCalca);
	PintarDono(TorsoMesh, CorDoLado);
	PintarDono(HeadMesh, CorDaPele);
	PintarDono(HatMesh, CorDoLado * EscurecimentoDoChapeu);
}

TArray<UStaticMeshComponent*> APetOwnerView::GetBodyParts() const
{
	return { LegsMesh, TorsoMesh, HeadMesh, HatMesh };
}

float APetOwnerView::GetStandingHeight()
{
	using namespace DonoDoPet;

	return AlturaDasPernas + AlturaDoTronco + DiametroDaCabeca + AlturaDoChapeu - EncaixeDoChapeu;
}
