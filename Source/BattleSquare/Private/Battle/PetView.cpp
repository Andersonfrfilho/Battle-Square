// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetView.h"
#include "Battle/BattleTypes.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

APetView::APetView()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	SetRootComponent(BodyRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(BodyRoot);
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

	HealthBarBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthBarBackground"));
	HealthBarBackground->SetupAttachment(BodyRoot);
	HealthBarBackground->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthBarFill = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthBarFill"));
	HealthBarFill->SetupAttachment(BodyRoot);
	HealthBarFill->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		HealthBarBackground->SetStaticMesh(CubeMesh.Object);
		HealthBarFill->SetStaticMesh(CubeMesh.Object);

		// A câmera olha ao longo de +X, então a barra é FINA em X, larga em Y
		// e baixa em Z. Escala em cima do cubo de 100uu da engine.
		HealthBarBackground->SetRelativeScale3D(FVector(BarDepthScale, BarWidthScale, BarHeightScale));
		HealthBarBackground->SetRelativeLocation(FVector(0.0f, 0.0f, BarHeightUnits));

		// O preenchimento fica INTEIRAMENTE à frente do fundo, e mais baixo.
		//
		// Deslocar só 1uu não bastava: com 4uu de espessura nos dois cubos, as
		// faces continuavam no mesmo plano e a GPU não tinha como escolher qual
		// desenhar — é o piscar. Com a separação maior que a espessura, nenhuma
		// face coincide. Sendo mais baixo, o fundo ainda vira moldura, e a
		// borda de cima (onde o piscar aparecia) deixa de ser compartilhada.
		HealthBarFill->SetRelativeLocation(FVector(-BarFrontOffsetUnits, 0.0f, BarHeightUnits));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		BodyMesh->SetMaterial(0, BasicMaterial.Object);
		HealthBarBackground->SetMaterial(0, BasicMaterial.Object);
		HealthBarFill->SetMaterial(0, BasicMaterial.Object);
	}
}

void APetView::RefreshHealthBar()
{
	if (!HealthBarFill || !HealthBarBackground)
	{
		return;
	}

	if (!HealthBarFillMaterial)
	{
		HealthBarFillMaterial = HealthBarFill->CreateDynamicMaterialInstance(0);
	}
	if (!HealthBarBackgroundMaterial)
	{
		HealthBarBackgroundMaterial = HealthBarBackground->CreateDynamicMaterialInstance(0);
	}

	const float Ratio = FMath::Clamp(HealthRatio, 0.0f, 1.0f);
	HealthBarFill->SetRelativeScale3D(FVector(BarDepthScale, BarWidthScale * Ratio, BarFillHeightScale));

	// O cubo tem origem no CENTRO: encolher a escala sozinha faria a barra
	// sumir pelos DOIS lados. Deslocar metade do que se perdeu ancora ela à
	// esquerda, que é como uma barra de vida se lê.
	const float FullWidthUnits = BarWidthScale * CubeSizeUnits;
	HealthBarFill->SetRelativeLocation(
		FVector(-BarFrontOffsetUnits, -(FullWidthUnits * (1.0f - Ratio)) * 0.5f, BarHeightUnits));

	if (HealthBarFillMaterial)
	{
		// Verde -> vermelho conforme cai: a cor avisa antes do tamanho, e a
		// diferença entre 30% e 15% de largura é difícil de ver de longe.
		HealthBarFillMaterial->SetVectorParameterValue(TEXT("Color"),
			FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Green, Ratio));
	}
	if (HealthBarBackgroundMaterial)
	{
		HealthBarBackgroundMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.05f, 0.05f));
	}

	HealthBarBackground->SetVisibility(!bDefeated);
	HealthBarFill->SetVisibility(!bDefeated && Ratio > 0.0f);
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

	RefreshHealthBar();
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
