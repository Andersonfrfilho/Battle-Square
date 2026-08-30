// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WalkableMountain.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/ScenaryPalette.h"
#include "UObject/ConstructorHelpers.h"

namespace MontanhaDaIlha
{
	/** O cone da engine, o mesmo da serra: forma de montanha sem escultura. */
	const TCHAR* MalhaDoCorpo = TEXT("/Engine/BasicShapes/Cone.Cone");

	/** O cubo da engine vira patamar quando achatado. */
	const TCHAR* MalhaDoPatamar = TEXT("/Engine/BasicShapes/Cube.Cube");

	/**
	 * Quanto o patamar passa da distância até o vizinho.
	 *
	 * Acima de 1 por obrigação, não por gosto: dois patamares que apenas se
	 * encostam deixam uma fresta na diagonal, e uma fresta numa trilha em
	 * espiral é o lugar exato por onde se cai de volta ao chão.
	 */
	constexpr float SobreposicaoDoPatamar = 1.6f;

	/**
	 * O serpenteio, como fração do passo angular.
	 *
	 * Mexe no ÂNGULO e nunca no raio: raio sorteado tiraria o patamar de cima
	 * da rocha, e a trilha passaria a flutuar em alguns pontos. Um quarto de
	 * passo é o teto para os patamares não trocarem de ordem.
	 */
	constexpr float SerpenteioMaximo = 0.25f;
}

AWalkableMountain::AWalkableMountain()
{
	PrimaryActorTick.bCanEverTick = false;

	MountainRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MountainRoot"));
	SetRootComponent(MountainRoot);

	// Malha atribuída no construtor, não na montagem: componente sem asset
	// passa em todo teste de lógica e não existe na tela.
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(MontanhaDaIlha::MalhaDoCorpo);
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(MontanhaDaIlha::MalhaDoPatamar);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MountainBody"));
	Body->SetupAttachment(MountainRoot);

	Trail = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("MountainTrail"));
	Trail->SetupAttachment(MountainRoot);

	if (Cone.Succeeded())
	{
		Body->SetStaticMesh(Cone.Object);
	}

	if (Cubo.Succeeded())
	{
		Trail->SetStaticMesh(Cubo.Object);
	}

	// A montanha da ilha existe para ser TOCADA — é o que a separa da serra do
	// horizonte, que não tem colisão nenhuma.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Body->SetCollisionResponseToAllChannels(ECR_Block);

	Trail->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Trail->SetCollisionResponseToAllChannels(ECR_Block);
	Trail->SetCanEverAffectNavigation(false);
}

float AWalkableMountain::RadiusAtHeight(float ZUnits) const
{
	if (HeightUnits <= 0.0f || ZUnits >= HeightUnits)
	{
		return 0.0f;
	}

	return BaseRadiusUnits * (1.0f - FMath::Max(0.0f, ZUnits) / HeightUnits);
}

void AWalkableMountain::BuildMountain(uint32 Seed)
{
	if (!Body || !Trail)
	{
		return;
	}

	Trail->ClearInstances();
	TrailSteps.Reset();

	ScenaryPalette::PaintComponent(Body, EScenaryRole::ClimbableRock);
	ScenaryPalette::PaintComponent(Trail, EScenaryRole::MountainTrail);

	UStaticMesh* MalhaDoCorpo = Body->GetStaticMesh().Get();
	if (!MalhaDoCorpo)
	{
		return;
	}

	// Escala do corpo a partir da caixa MEDIDA: o dia em que o cone da engine
	// mudar de tamanho, a montanha continua com a altura que a trilha usou.
	const FBox CaixaDoCone = MalhaDoCorpo->GetBoundingBox();
	const FVector TamanhoDoCone = CaixaDoCone.GetSize();
	if (TamanhoDoCone.X <= 0.0 || TamanhoDoCone.Y <= 0.0 || TamanhoDoCone.Z <= 0.0)
	{
		return;
	}

	const float EscalaZ = HeightUnits / static_cast<float>(TamanhoDoCone.Z);
	const float EscalaXY = (2.0f * BaseRadiusUnits) / static_cast<float>(TamanhoDoCone.X);

	Body->SetRelativeScale3D(FVector(EscalaXY, EscalaXY, EscalaZ));
	Body->SetRelativeLocation(FVector(0.0, 0.0, -CaixaDoCone.Min.Z * EscalaZ));

	const float AlturaDaTrilha = HeightUnits * TrailTopFraction;
	const int32 Passos = FMath::Max(2,
		FMath::CeilToInt(AlturaDaTrilha / FMath::Max(1.0f, TrailRiseUnits)) + 1);

	const float PassoVertical = AlturaDaTrilha / static_cast<float>(Passos - 1);
	const float PassoAngular = (TrailTurns * 2.0f * PI) / static_cast<float>(Passos - 1);

	// Primeiro TODAS as posições, e só depois os patamares: o tamanho de um
	// patamar depende da distância até os vizinhos dele, e o vizinho de cima
	// ainda não existe enquanto se sobe.
	TArray<FVector> Posicoes;
	TArray<float> Angulos;
	Posicoes.Reserve(Passos);
	Angulos.Reserve(Passos);

	for (int32 Passo = 0; Passo < Passos; ++Passo)
	{
		const float Altura = Passo * PassoVertical;
		const float Serpenteio = BattleSpread::Between(
			-MontanhaDaIlha::SerpenteioMaximo, MontanhaDaIlha::SerpenteioMaximo,
			BattleSpread::Fraction(Seed, Passo)) * PassoAngular;
		const float Angulo = Passo * PassoAngular + Serpenteio;
		const float Raio = RadiusAtHeight(Altura);

		Angulos.Add(Angulo);
		Posicoes.Add(FVector(
			FMath::Cos(Angulo) * Raio,
			FMath::Sin(Angulo) * Raio,
			Altura));
	}

	for (int32 Passo = 0; Passo < Passos; ++Passo)
	{
		float Vizinhanca = 0.0f;
		if (Passo > 0)
		{
			Vizinhanca = FMath::Max(Vizinhanca,
				static_cast<float>(FVector::Dist2D(Posicoes[Passo], Posicoes[Passo - 1])));
		}
		if (Passo + 1 < Passos)
		{
			Vizinhanca = FMath::Max(Vizinhanca,
				static_cast<float>(FVector::Dist2D(Posicoes[Passo], Posicoes[Passo + 1])));
		}

		CarveStep(static_cast<float>(Posicoes[Passo].Z), Angulos[Passo],
			Vizinhanca * MontanhaDaIlha::SobreposicaoDoPatamar);
	}

	// O cume é um PATAMAR, não a ponta do cone: ponta não se pisa, e uma
	// trilha que termina num lugar onde não se para não terminou em lugar
	// nenhum.
	UStaticMesh* MalhaDoPatamar = Trail->GetStaticMesh().Get();
	const FVector TamanhoDoCubo = MalhaDoPatamar
		? MalhaDoPatamar->GetBoundingBox().GetSize() : FVector::ZeroVector;
	if (TamanhoDoCubo.X <= 0.0 || TamanhoDoCubo.Y <= 0.0 || TamanhoDoCubo.Z <= 0.0)
	{
		return;
	}

	const float LadoDoCume = 2.0f * FMath::Max(TrailWidthUnits, RadiusAtHeight(AlturaDaTrilha));

	FTransform Cume;
	Cume.SetScale3D(FVector(
		LadoDoCume / static_cast<float>(TamanhoDoCubo.X),
		LadoDoCume / static_cast<float>(TamanhoDoCubo.Y),
		TrailThicknessUnits / static_cast<float>(TamanhoDoCubo.Z)));
	Cume.SetLocation(FVector(0.0, 0.0, AlturaDaTrilha - 0.5f * TrailThicknessUnits));
	Trail->AddInstance(Cume);
	TrailSteps.Add(Cume);
}

void AWalkableMountain::CarveStep(float ZUnits, float AngleRadians, float TangentialLengthUnits)
{
	UStaticMesh* Malha = Trail ? Trail->GetStaticMesh().Get() : nullptr;
	if (!Malha)
	{
		return;
	}

	const FVector Tamanho = Malha->GetBoundingBox().GetSize();
	if (Tamanho.X <= 0.0 || Tamanho.Y <= 0.0 || Tamanho.Z <= 0.0)
	{
		return;
	}

	const float Raio = RadiusAtHeight(ZUnits);

	// Girado pelo ângulo, o X local aponta para FORA da montanha e o Y local
	// segue a encosta. Centrado no raio da rocha, metade do patamar fica
	// enterrada e metade vira beirada — é a beirada que se pisa.
	FTransform Patamar;
	Patamar.SetScale3D(FVector(
		TrailWidthUnits / static_cast<float>(Tamanho.X),
		FMath::Max(TrailWidthUnits, TangentialLengthUnits) / static_cast<float>(Tamanho.Y),
		TrailThicknessUnits / static_cast<float>(Tamanho.Z)));
	Patamar.SetRotation(FRotator(0.0f, FMath::RadiansToDegrees(AngleRadians), 0.0f).Quaternion());
	Patamar.SetLocation(FVector(
		FMath::Cos(AngleRadians) * Raio,
		FMath::Sin(AngleRadians) * Raio,
		ZUnits - 0.5f * TrailThicknessUnits));

	Trail->AddInstance(Patamar);
	TrailSteps.Add(Patamar);
}
