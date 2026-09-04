// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WalkableMountain.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/MountainProfile.h"
#include "Environment/ScenaryPalette.h"
#include "UObject/ConstructorHelpers.h"

namespace MontanhaDaIlha
{
	/**
	 * O cilindro da engine, uma fatia de cada vez.
	 *
	 * Não é o cone: cone escalado continua cone, e foi o cone que o jogador
	 * chamou de esquisito. Curva se obtém empilhando raios diferentes.
	 */
	const TCHAR* MalhaDoCorpo = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cylinder);

	/** O cubo da engine vira patamar quando achatado. */
	const TCHAR* MalhaDoPatamar = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cube);

	/** A pedra esculpida do kit — o que dá curva onde o cilindro dá aresta. */
	const TCHAR* MalhaDoCostado = TEXT("/Game/Environment/Nature/rock_largeA.rock_largeA");

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

	/**
	 * Quanto cada fatia transborda a de baixo, na vertical.
	 *
	 * Fatias que apenas se encostam deixam a mesma fresta diagonal dos
	 * patamares, e ali a fresta é pior: é por dentro da montanha que se cai.
	 */
	constexpr float TransbordoDaFatia = 1.08f;

	/** Quantas pedras rodeiam cada fatia. */
	constexpr int32 PedrasPorFatia = 5;

	/** Tamanho da pedra como fração da altura da fatia. */
	constexpr float PedraMinima = 0.55f;
	constexpr float PedraMaxima = 1.35f;

	/** Quantas fatias do topo ficam sem costado, para o cume continuar limpo. */
	constexpr int32 FatiasLimpasNoCume = 2;

	/** Um fluxo de sorteio por pedra, para uma não roubar o número da outra. */
	int32 FluxoDaPedra(int32 Fatia, int32 Pedra, int32 Passo)
	{
		constexpr int32 SorteiosPorPedra = 4;
		return 1000 + (Fatia * PedrasPorFatia + Pedra) * SorteiosPorPedra + Passo;
	}
}

AWalkableMountain::AWalkableMountain()
{
	PrimaryActorTick.bCanEverTick = false;

	MountainRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MountainRoot"));
	SetRootComponent(MountainRoot);

	// Malha atribuída no construtor, não na montagem: componente sem asset
	// passa em todo teste de lógica e não existe na tela.
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(MontanhaDaIlha::MalhaDoCorpo);
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(MontanhaDaIlha::MalhaDoPatamar);
	ConstructorHelpers::FObjectFinder<UStaticMesh> Pedra(MontanhaDaIlha::MalhaDoCostado);

	Body = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("MountainBody"));
	Body->SetupAttachment(MountainRoot);

	Trail = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("MountainTrail"));
	Trail->SetupAttachment(MountainRoot);

	Boulders = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("MountainBoulders"));
	Boulders->SetupAttachment(MountainRoot);

	if (Cilindro.Succeeded())
	{
		Body->SetStaticMesh(Cilindro.Object);
	}

	if (Cubo.Succeeded())
	{
		Trail->SetStaticMesh(Cubo.Object);
	}

	if (Pedra.Succeeded())
	{
		Boulders->SetStaticMesh(Pedra.Object);
	}

	// A montanha da ilha existe para ser TOCADA — é o que a separa da serra do
	// horizonte, que não tem colisão nenhuma.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Body->SetCollisionResponseToAllChannels(ECR_Block);

	Trail->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Trail->SetCollisionResponseToAllChannels(ECR_Block);
	Trail->SetCanEverAffectNavigation(false);

	// O costado é ENFEITE. Se ele colidisse, a trilha teria pedra no meio do
	// caminho em posição sorteada — e trilha com obstáculo sorteado é trilha
	// que às vezes não passa.
	Boulders->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Boulders->SetCanEverAffectNavigation(false);
}

UStaticMeshComponent* AWalkableMountain::GetBody() const
{
	// Fora de linha porque o cabeçalho só declara o HISM adiantadamente, e sem
	// a definição o compilador não sabe que ele deriva de UStaticMeshComponent.
	return Body.Get();
}

float AWalkableMountain::RadiusAtHeight(float ZUnits) const
{
	if (HeightUnits <= 0.0f || ZUnits >= HeightUnits)
	{
		return 0.0f;
	}

	return BaseRadiusUnits
		* MountainProfile::RadiusFractionAt(FMath::Max(0.0f, ZUnits) / HeightUnits);
}

void AWalkableMountain::BuildMountain(uint32 Seed)
{
	if (!Body || !Trail || !Boulders)
	{
		return;
	}

	Body->ClearInstances();
	Boulders->ClearInstances();
	Trail->ClearInstances();
	TrailSteps.Reset();

	ScenaryPalette::PaintComponent(Body, EScenaryRole::ClimbableRock);
	ScenaryPalette::PaintComponent(Boulders, EScenaryRole::MountainRock);
	ScenaryPalette::PaintComponent(Trail, EScenaryRole::MountainTrail);

	UStaticMesh* MalhaDoCorpo = Body->GetStaticMesh().Get();
	if (!MalhaDoCorpo)
	{
		return;
	}

	// Escala a partir da caixa MEDIDA: o dia em que o cilindro da engine mudar
	// de tamanho, a montanha continua com a altura que a trilha usou.
	const FVector TamanhoDoCilindro = MalhaDoCorpo->GetBoundingBox().GetSize();
	if (TamanhoDoCilindro.X <= 0.0 || TamanhoDoCilindro.Y <= 0.0 || TamanhoDoCilindro.Z <= 0.0)
	{
		return;
	}

	const float AlturaDaFatia = HeightUnits * MountainProfile::SliceHeightFraction();

	for (int32 Fatia = 0; Fatia < MountainProfile::SliceCount; ++Fatia)
	{
		const float Raio = BaseRadiusUnits * MountainProfile::SliceRadiusFraction(Fatia);
		const float Base = Fatia * AlturaDaFatia;
		const float Altura = AlturaDaFatia * MontanhaDaIlha::TransbordoDaFatia;

		FTransform Cilindro;
		Cilindro.SetScale3D(FVector(
			(2.0f * Raio) / static_cast<float>(TamanhoDoCilindro.X),
			(2.0f * Raio) / static_cast<float>(TamanhoDoCilindro.Y),
			Altura / static_cast<float>(TamanhoDoCilindro.Z)));
		Cilindro.SetLocation(FVector(0.0, 0.0, Base + 0.5f * Altura));

		Body->AddInstance(Cilindro);
	}

	CladSlopeWithBoulders(Seed, AlturaDaFatia);

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

	// O cume é um PATAMAR, e agora ele cobre a calota inteira: a fatia de cima
	// tem largura, e uma trilha que termina numa borda estreita de um platô
	// termina a um passo de cair.
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

void AWalkableMountain::CladSlopeWithBoulders(uint32 Seed, float SliceHeightUnits)
{
	UStaticMesh* Malha = Boulders ? Boulders->GetStaticMesh().Get() : nullptr;
	if (!Malha)
	{
		return;
	}

	const FVector Tamanho = Malha->GetBoundingBox().GetSize();
	const float MaiorLado = static_cast<float>(FMath::Max3(Tamanho.X, Tamanho.Y, Tamanho.Z));
	if (MaiorLado <= 0.0f)
	{
		return;
	}

	const int32 UltimaFatia =
		MountainProfile::SliceCount - MontanhaDaIlha::FatiasLimpasNoCume;

	for (int32 Fatia = 0; Fatia < UltimaFatia; ++Fatia)
	{
		const float Raio = BaseRadiusUnits * MountainProfile::SliceRadiusFraction(Fatia);
		const float Topo = (Fatia + 1) * SliceHeightUnits;

		for (int32 Pedra = 0; Pedra < MontanhaDaIlha::PedrasPorFatia; ++Pedra)
		{
			const float Angulo = 2.0f * PI * BattleSpread::Fraction(
				Seed, MontanhaDaIlha::FluxoDaPedra(Fatia, Pedra, 0));
			const float Escala = SliceHeightUnits * BattleSpread::Between(
				MontanhaDaIlha::PedraMinima, MontanhaDaIlha::PedraMaxima,
				BattleSpread::Fraction(Seed, MontanhaDaIlha::FluxoDaPedra(Fatia, Pedra, 1)))
				/ MaiorLado;
			const float Giro = 360.0f * BattleSpread::Fraction(
				Seed, MontanhaDaIlha::FluxoDaPedra(Fatia, Pedra, 2));
			// A pedra sobe ou desce um pouco em relação ao degrau: alinhadas
			// todas na mesma altura, cinco pedras por fatia desenhariam de
			// volta exatamente a aresta que elas existem para esconder.
			const float Desnivel = SliceHeightUnits * BattleSpread::Between(-0.35f, 0.15f,
				BattleSpread::Fraction(Seed, MontanhaDaIlha::FluxoDaPedra(Fatia, Pedra, 3)));

			FTransform Bloco;
			Bloco.SetScale3D(FVector(Escala));
			Bloco.SetRotation(FRotator(0.0f, Giro, 0.0f).Quaternion());
			Bloco.SetLocation(FVector(
				FMath::Cos(Angulo) * Raio,
				FMath::Sin(Angulo) * Raio,
				Topo + Desnivel));

			Boulders->AddInstance(Bloco);
		}
	}
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
