// Copyright 2026 Anderson. All Rights Reserved.

#include "World/Village.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/ScenaryPalette.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace Vila
{
	/** O cubo da engine mede 100 unidades de lado. */
	constexpr float CuboDaEngine = 100.0f;

	/** O telhado é um pouco MAIOR que a parede: beiral se lê de longe. */
	constexpr float BeiralUnidades = 60.0f;
	constexpr float AlturaDoTelhado = 90.0f;

	/**
	 * A cor de cada prédio, e ela é INFORMAÇÃO.
	 *
	 * O Centro de Recuperação precisa se achar sem placa — é onde se corre
	 * quando o pet está mal. As casas são surdas de propósito: elas são
	 * silhueta, e competir com o que tem função faria a vila inteira gritar.
	 */
	FLinearColor CorDoPredio(EVillageBuilding Predio)
	{
		switch (Predio)
		{
			case EVillageBuilding::CentroDeRecuperacao: return FLinearColor(0.90f, 0.35f, 0.32f);
			case EVillageBuilding::Escola:              return FLinearColor(0.45f, 0.60f, 0.85f);
			case EVillageBuilding::Arena:               return FLinearColor(0.85f, 0.68f, 0.30f);
			case EVillageBuilding::Marco:               return FLinearColor(0.70f, 0.72f, 0.75f);
			case EVillageBuilding::Praca:               return FLinearColor(0.52f, 0.47f, 0.38f);
			case EVillageBuilding::Casa:                return FLinearColor(0.58f, 0.50f, 0.42f);
			// Cores próprias, e é o que faz reconhecer o assentamento de longe
			// sem placa: verde de academia, roxo de mercado, pedra escura de
			// portão.
			case EVillageBuilding::Academia:            return FLinearColor(0.35f, 0.75f, 0.45f);
			case EVillageBuilding::Mercado:             return FLinearColor(0.62f, 0.40f, 0.78f);
			case EVillageBuilding::Portao:              return FLinearColor(0.30f, 0.32f, 0.36f);
		}
		return FLinearColor(0.5f, 0.5f, 0.5f);
	}

	/** Telhado sempre mais escuro que a parede: é o que separa os dois de longe. */
	FLinearColor CorDoTelhado(EVillageBuilding Predio)
	{
		return CorDoPredio(Predio) * 0.45f;
	}

	void Pintar(UStaticMeshComponent* Componente, const FLinearColor& Cor)
	{
		UMaterialInterface* Base = ScenaryPalette::ColorableBaseMaterial();
		if (!Componente || !Base)
		{
			return;
		}

		UMaterialInstanceDynamic* Tinta =
			UMaterialInstanceDynamic::Create(Base, Componente);
		if (!Tinta)
		{
			return;
		}

		Tinta->SetVectorParameterValue(TEXT("Color"), Cor);
		Componente->SetMaterial(0, Tinta);
	}
}

AVillage::AVillage()
{
	PrimaryActorTick.bCanEverTick = false;

	VillageRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VillageRoot"));
	SetRootComponent(VillageRoot);

	// A MALHA vem no construtor, e não em BeginPlay: ator que nasce sem asset
	// atribuído passa em toda bateria e não existe na tela — três vezes neste
	// projeto, e hoje há sonda para isso.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cubo.Succeeded())
	{
		BoxMesh = Cubo.Object;
	}
}

void AVillage::BeginPlay()
{
	Super::BeginPlay();
	BuildVillage();
}

void AVillage::BuildVillage()
{
	for (TObjectPtr<UStaticMeshComponent>& Componente : BuiltMeshes)
	{
		if (Componente) { Componente->DestroyComponent(); }
	}
	for (TObjectPtr<UStaticMeshComponent>& Componente : BuiltRoofs)
	{
		if (Componente) { Componente->DestroyComponent(); }
	}
	BuiltMeshes.Reset();
	BuiltRoofs.Reset();

	if (!BoxMesh)
	{
		return;
	}

	int32 Indice = 0;
	for (const FVillagePlacement& Peca : VillageLayout::PlanFor(Kind))
	{
		UStaticMeshComponent* Parede = NewObject<UStaticMeshComponent>(this,
			*FString::Printf(TEXT("Predio_%d"), Indice));
		Parede->SetupAttachment(VillageRoot);
		Parede->RegisterComponent();
		Parede->SetStaticMesh(BoxMesh);

		Parede->SetRelativeScale3D(FVector(
			(Peca.HalfExtentUnits.X * 2.0f) / Vila::CuboDaEngine,
			(Peca.HalfExtentUnits.Y * 2.0f) / Vila::CuboDaEngine,
			Peca.HeightUnits / Vila::CuboDaEngine));
		Parede->SetRelativeLocation(FVector(
			Peca.OffsetUnits.X, Peca.OffsetUnits.Y, Peca.HeightUnits * 0.5f));

		// BLOQUEIA. Cenário sólido sem colisão é o defeito que o usuário
		// relatou atravessando montanha, e uma vila que se atravessa não é
		// um lugar.
		Parede->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Parede->SetCollisionResponseToAllChannels(ECR_Block);

		Vila::Pintar(Parede, Vila::CorDoPredio(Peca.Building));
		BuiltMeshes.Add(Parede);

		// A PRAÇA não tem telhado: ela é chão, e um telhado sobre ela seria
		// um galpão no meio da vila.
		if (Peca.Building != EVillageBuilding::Praca)
		{
			UStaticMeshComponent* Telhado = NewObject<UStaticMeshComponent>(this,
				*FString::Printf(TEXT("Telhado_%d"), Indice));
			Telhado->SetupAttachment(VillageRoot);
			Telhado->RegisterComponent();
			Telhado->SetStaticMesh(BoxMesh);

			Telhado->SetRelativeScale3D(FVector(
				(Peca.HalfExtentUnits.X * 2.0f + Vila::BeiralUnidades) / Vila::CuboDaEngine,
				(Peca.HalfExtentUnits.Y * 2.0f + Vila::BeiralUnidades) / Vila::CuboDaEngine,
				Vila::AlturaDoTelhado / Vila::CuboDaEngine));
			Telhado->SetRelativeLocation(FVector(Peca.OffsetUnits.X, Peca.OffsetUnits.Y,
				Peca.HeightUnits + Vila::AlturaDoTelhado * 0.5f));

			Telhado->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Telhado->SetCollisionResponseToAllChannels(ECR_Block);

			Vila::Pintar(Telhado, Vila::CorDoTelhado(Peca.Building));
			BuiltRoofs.Add(Telhado);
		}

		++Indice;
	}
}
