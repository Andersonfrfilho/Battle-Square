// Copyright 2026 Anderson. All Rights Reserved.

#include "World/VillagerActor.h"

#include "Battle/DeterministicSpread.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Environment/SceneLighting.h"
#include "Environment/ScenaryPalette.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "World/EncounterRoamingComponent.h"

namespace
{
	/** Gente é menor que prédio e maior que pet: corpo de ~1,6 m. */
	constexpr float AlturaDoCorpo = 130.0f;
	constexpr float LarguraDoCorpo = 52.0f;
	constexpr float DiametroDaCabeca = 44.0f;

	/**
	 * As cores de roupa. Tons apagados de propósito: o morador é a vila
	 * andando, não um marco — competir com o Centro vermelho e a Escola azul
	 * faria a praça gritar.
	 */
	const FLinearColor RoupasDeMorador[] = {
		FLinearColor(0.55f, 0.45f, 0.35f), FLinearColor(0.40f, 0.45f, 0.55f),
		FLinearColor(0.50f, 0.55f, 0.40f), FLinearColor(0.58f, 0.50f, 0.55f),
		FLinearColor(0.45f, 0.40f, 0.35f), FLinearColor(0.35f, 0.50f, 0.50f),
	};

	const FLinearColor PeleDeMorador(0.85f, 0.70f, 0.58f);

	void PintarMorador(UStaticMeshComponent* Componente, const FLinearColor& Cor)
	{
		UMaterialInterface* Base = ScenaryPalette::ColorableBaseMaterial();
		if (!Componente || !Base)
		{
			return;
		}

		if (UMaterialInstanceDynamic* Tinta =
			UMaterialInstanceDynamic::Create(Base, Componente))
		{
			Tinta->SetVectorParameterValue(TEXT("Color"), Cor);
			Componente->SetMaterial(0, Tinta);
		}
	}
}

AVillagerActor::AVillagerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	VillagerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VillagerRoot"));
	SetRootComponent(VillagerRoot);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(VillagerRoot);
	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(VillagerRoot);

	// AS MALHAS NO CONSTRUTOR — a regra três vezes paga. Componente sem asset
	// passa em toda bateria e não existe na tela.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(
		ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cylinder));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(
		ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Sphere));

	if (Cilindro.Succeeded())
	{
		Body->SetStaticMesh(Cilindro.Object);
	}
	if (Esfera.Succeeded())
	{
		Head->SetStaticMesh(Esfera.Object);
	}

	// O cubo da engine mede 100; o cilindro e a esfera também.
	Body->SetRelativeScale3D(FVector(
		LarguraDoCorpo / 100.0f, LarguraDoCorpo / 100.0f, AlturaDoCorpo / 100.0f));
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, AlturaDoCorpo * 0.5f));

	Head->SetRelativeScale3D(FVector(DiametroDaCabeca / 100.0f));
	Head->SetRelativeLocation(FVector(0.0f, 0.0f,
		AlturaDoCorpo + DiametroDaCabeca * 0.5f));

	// Morador não empurra nem bloqueia: ele é vizinho, não obstáculo — e uma
	// praça onde se fica preso em gente seria a calçada bloqueando de novo.
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AVillagerActor::Configure(ESettlementKind Kind, int32 DoorIndex,
	const FVector& PlazaCenter, float PlazaRadius)
{
	Resident = VillageResidents::ResidentFor(Kind, DoorIndex);
	HomeKind = Kind;
	HomeDoor = DoorIndex;
	bConfigured = true;

	// A roupa sai da MESMA semente do morador: a Dona Iraci veste sempre a
	// mesma cor, e reconhecê-la de longe é parte de ela ser vizinha.
	const uint32 Semente = BattleSpread::SeedFromText(FString::Printf(
		TEXT("roupa-%s-%d"), *Resident.Name, DoorIndex));
	PintarMorador(Body, RoupasDeMorador[
		BattleSpread::Below(Semente, 0, UE_ARRAY_COUNT(RoupasDeMorador))]);
	PintarMorador(Head, PeleDeMorador);

	// O passeio é o mesmo dos encontros, com a rua da vila por raio.
	if (!Roaming)
	{
		Roaming = NewObject<UEncounterRoamingComponent>(this);
		Roaming->RegisterComponent();
	}
	Roaming->RoamRadiusUnits = PlazaRadius;

	// Gente anda mais devagar que bicho passeia — e para mais.
	Roaming->RoamSpeedUnitsPerSecond = 120.0f;
	Roaming->PauseSecondsOnArrival = 3.0f;
	Roaming->ConfigureRoaming(PlazaCenter,
		static_cast<int32>(Semente & 0x7FFFFFFF));
}

void AVillagerActor::ApplyHour(float Hora)
{
	if (!bConfigured)
	{
		return;
	}

	// EM CASA, o corpo some da rua: ele está dentro, e dentro não se vê. É a
	// mesma janela que decide quem atende a porta — uma fonte só, senão a
	// Dona Iraci atenderia em casa e estaria na praça ao mesmo tempo.
	const bool bEmCasa = VillageResidents::IsHomeAtHour(Resident, Hora);
	SetActorHiddenInGame(bEmCasa);

	if (Roaming)
	{
		Roaming->SetComponentTickEnabled(!bEmCasa);
	}
}

void AVillagerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bConfigured)
	{
		return;
	}

	if (!CachedSky)
	{
		TActorIterator<ABattleSceneLighting> Ceu(GetWorld());
		CachedSky = Ceu ? *Ceu : nullptr;
	}

	// Sem céu rodando (mundo de teste), o morador fica na rua: trancar todo
	// mundo em casa num mundo sem tempo esconderia a mecânica onde ninguém a
	// vê — mesma postura da visita.
	if (CachedSky && CachedSky->IsDayCycleRunning())
	{
		ApplyHour(CachedSky->GetHour());
	}
}
