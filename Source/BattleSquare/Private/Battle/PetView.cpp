// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetView.h"
#include "Battle/BattleTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

APetView::APetView()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	SetRootComponent(BodyMesh);
	// Sem colisão: o pet é apresentação, e o núcleo é quem decide onde ele
	// está. Um corpo que empurra outro seria uma segunda fonte de verdade.
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Conteúdo da engine, não vendorizado — mesmo princípio de AD-019.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(SphereMesh.Object);

		// A esfera da engine tem 100uu de diâmetro e origem no CENTRO. A casa
		// da grade fica no nível do chão, então sem levantar pelo raio o pet
		// nasce metade enterrado no tabuleiro.
		constexpr float BodyScale = 0.7f;
		constexpr float SphereRadiusUnits = 50.0f;
		BodyMesh->SetRelativeScale3D(FVector(BodyScale));
		BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SphereRadiusUnits * BodyScale));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		BodyMesh->SetMaterial(0, BasicMaterial.Object);
	}
}

void APetView::RefreshBodyAppearance()
{
	if (!BodyMesh)
	{
		return;
	}

	if (!BodyMaterial)
	{
		BodyMaterial = BodyMesh->CreateDynamicMaterialInstance(0);
	}
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"),
			Side == 0 ? LocalSideColor : OpponentSideColor);
	}

	// Derrotado some do tabuleiro — o núcleo já o tirou da partida.
	BodyMesh->SetVisibility(!bDefeated);
}

void APetView::SetInitialState(const FPetState& InitialState, const FPetPresentationInfo& Presentation)
{
	PetId = InitialState.PetId;
	Column = InitialState.Column;
	Row = InitialState.Row;
	MaxHealth = InitialState.MaxHealth;
	Side = InitialState.Side;
	HealthRatio = MaxHealth > 0 ? 1.0f : 0.0f;
	bDefeated = false;

	RefreshBodyAppearance();
}

void APetView::ApplyEvent(const FBattleEvent& Event)
{
	switch (Event.Type)
	{
		case EBattleEventType::DanoAplicado:
		{
			// Nunca recalcula dano — Value já é o número final que o
			// núcleo aplicou (BTL-22). Só converte para uma razão visual.
			if (MaxHealth > 0)
			{
				const float DamageRatio = static_cast<float>(Event.Value) / static_cast<float>(MaxHealth);
				HealthRatio = FMath::Clamp(HealthRatio - DamageRatio, 0.0f, 1.0f);
			}
			break;
		}
		case EBattleEventType::PetMorreu:
		{
			bDefeated = true;
			break;
		}
		case EBattleEventType::Moveu:
		{
			uint8 NewColumn = 0;
			uint8 NewRow = 0;
			UnpackCell(Event.ToCell, NewColumn, NewRow);
			Column = NewColumn;
			Row = NewRow;
			break;
		}
		default:
			break;
	}
}
