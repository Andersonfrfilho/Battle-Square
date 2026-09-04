// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldBoundaryWater.h"
#include "Environment/ScenaryPalette.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** O cilindro da engine tem 100 unidades de lado. */
	constexpr float CilindroDaEngineUnidades = 100.0f;

	/** Espessura do disco de água. Fino: é superfície, não volume. */
	constexpr float EspessuraDaAguaUnidades = 20.0f;

	/** Altura da parede da margem. Alta o bastante para não se pular fora. */
	constexpr float AlturaDaParedeUnidades = 1200.0f;

	constexpr float EspessuraDaParedeUnidades = 60.0f;
}

AWorldBoundaryWater::AWorldBoundaryWater()
{
	PrimaryActorTick.bCanEverTick = false;

	WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
	SetRootComponent(WaterMesh);

	// A água não colide: quem barra é o anel. Água que empurra viraria uma
	// parede molhada, e a ideia é justamente o contrário — a barreira some e a
	// razão dela aparece.
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// MALHA ATRIBUÍDA AQUI. Quinta vez que este projeto escreve esta linha:
	// ator com componente visual e sem asset passa em todo teste de lógica e
	// não existe na tela.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(
		ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cylinder));
	if (Cilindro.Succeeded())
	{
		WaterMesh->SetStaticMesh(Cilindro.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basico(
		ScenaryPalette::ColorableBaseMaterialPath());
	if (Basico.Succeeded())
	{
		WaterMesh->SetMaterial(0, Basico.Object);
	}
}

void AWorldBoundaryWater::BuildBoundary()
{
	const float RaioDaAgua = FMath::Max(WaterRadiusUnits, ShoreRadiusUnits + 100.0f);

	WaterMesh->SetRelativeScale3D(FVector(
		RaioDaAgua * 2.0f / CilindroDaEngineUnidades,
		RaioDaAgua * 2.0f / CilindroDaEngineUnidades,
		EspessuraDaAguaUnidades / CilindroDaEngineUnidades));
	WaterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -WaterDepthUnits));

	if (UMaterialInstanceDynamic* Material = WaterMesh->CreateDynamicMaterialInstance(0))
	{
		// Azul fundo e dessaturado: é TERRENO, e terreno não disputa com
		// criatura (ver a regra da paleta). E escuro o bastante para a margem
		// de terra clara se destacar contra ele.
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.16f, 0.26f));
	}

	// O anel: um polígono de caixas, porque a engine não traz cilindro oco.
	const int32 Lados = FMath::Max(8, ShoreSegments);
	const float AnguloPorLado = 2.0f * PI / static_cast<float>(Lados);

	// Cada caixa cobre a CORDA do arco, não o arco: a corda é mais curta, e
	// dimensionar pelo arco deixaria vãos nos cantos por onde se escapa.
	const float ComprimentoDaCorda = 2.0f * ShoreRadiusUnits * FMath::Sin(AnguloPorLado * 0.5f);

	for (int32 Lado = 0; Lado < Lados; ++Lado)
	{
		const float Angulo = AnguloPorLado * static_cast<float>(Lado);

		UBoxComponent* Parede = NewObject<UBoxComponent>(this);
		Parede->SetupAttachment(WaterMesh);
		Parede->RegisterComponent();

		// 10% a mais de comprimento: as caixas se SOBREPÕEM nos cantos. Vão de
		// um centímetro numa parede é vão por onde o jogador escapa, e ele
		// sempre encontra.
		Parede->SetBoxExtent(FVector(
			EspessuraDaParedeUnidades * 0.5f,
			ComprimentoDaCorda * 0.55f,
			AlturaDaParedeUnidades * 0.5f));

		Parede->SetRelativeLocation(FVector(
			FMath::Cos(Angulo) * ShoreRadiusUnits,
			FMath::Sin(Angulo) * ShoreRadiusUnits,
			WaterDepthUnits + AlturaDaParedeUnidades * 0.5f));
		Parede->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angulo), 0.0f));

		Parede->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Parede->SetCollisionResponseToAllChannels(ECR_Block);

		// INVISÍVEL: quem explica o limite é a água. Uma parede que se vê
		// transforma "o mundo acaba aqui" em "aqui tem uma cerca".
		Parede->SetHiddenInGame(true);

		ShoreWalls.Add(Parede);
	}
}
