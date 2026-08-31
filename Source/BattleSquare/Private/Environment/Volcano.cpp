// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/Volcano.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/ScenaryPalette.h"
#include "UObject/ConstructorHelpers.h"

namespace VulcaoDaIlha
{
	/**
	 * O cilindro da engine, e não o cone.
	 *
	 * O cone daria a encosta de graça e cobraria caro: a ponta dele não se
	 * corta, e sem corte não há cratera. Empilhar cilindros de raio decrescente
	 * dá a mesma silhueta E deixa o topo aberto — e o degrau entre um disco e o
	 * outro vira, de brinde, a camada de derrame que a encosta vulcânica tem.
	 */
	const TCHAR* MalhaDoDisco = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** O cubo da engine: fecha o anel da cratera e desce como derrame. */
	const TCHAR* MalhaDoBloco = TEXT("/Engine/BasicShapes/Cube.Cube");

	/** Quanto cada pedaço de derrame passa do de baixo, para não haver falha. */
	constexpr float SobreposicaoDoDerrame = 1.4f;

	/** O quanto um derrame pode sair da vertical, em graus, por pedaço. */
	constexpr float DesvioDoDerrame = 7.0f;

	/** O quanto a boca de um derrame se afasta do seu ângulo de partida. */
	constexpr float AberturaEntreDerrames = 16.0f;

	/** A esfera da engine: a fumaça é curva, e cubo de fumaça é caixa voando. */
	const TCHAR* MalhaDoBafo = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	/** O lado da esfera da engine, em unidades, com escala 1. */
	constexpr float LadoDaEsfera = 100.0f;

	/** A largura da primeira baforada, em fração do raio da cratera. */
	constexpr float LarguraDoPrimeiroBafo = 0.55f;

	/** Quanto a última baforada é mais larga que a primeira. */
	constexpr float AlargamentoDaColuna = 3.4f;

	/** O quanto o vento entorta a coluna, em fração do raio da cratera. */
	constexpr float DerivaDaColuna = 1.6f;

	/** O primeiro fluxo de sorteio da fumaça, longe do dos derrames. */
	constexpr int32 PrimeiroFluxoDoBafo = 40000;
}

AVolcano::AVolcano()
{
	PrimaryActorTick.bCanEverTick = false;

	VolcanoRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VolcanoRoot"));
	SetRootComponent(VolcanoRoot);

	// Malha atribuída no construtor, não na montagem: componente sem asset
	// passa em todo teste de lógica e não existe na tela.
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(VulcaoDaIlha::MalhaDoDisco);
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(VulcaoDaIlha::MalhaDoBloco);

	Slopes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("VolcanoSlopes"));
	Slopes->SetupAttachment(VolcanoRoot);

	Rim = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("VolcanoRim"));
	Rim->SetupAttachment(VolcanoRoot);

	Lava = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VolcanoLava"));
	Lava->SetupAttachment(VolcanoRoot);

	Flows = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("VolcanoFlows"));
	Flows->SetupAttachment(VolcanoRoot);

	Plume = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("VolcanoPlume"));
	Plume->SetupAttachment(VolcanoRoot);

	ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(VulcaoDaIlha::MalhaDoBafo);
	if (Esfera.Succeeded())
	{
		Plume->SetStaticMesh(Esfera.Object);
	}

	// A brasa da cratera. Sem ela o vulcão é uma silhueta preta a noite
	// inteira: pintar de laranja escolhe uma cor, e cor nao ilumina nada.
	CraterGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("VolcanoGlow"));
	CraterGlow->SetupAttachment(VolcanoRoot);
	CraterGlow->SetLightColor(FLinearColor(1.0f, 0.42f, 0.12f));
	CraterGlow->SetCastShadows(false);

	if (Cilindro.Succeeded())
	{
		Slopes->SetStaticMesh(Cilindro.Object);
		Lava->SetStaticMesh(Cilindro.Object);
	}

	if (Cubo.Succeeded())
	{
		Rim->SetStaticMesh(Cubo.Object);
		Flows->SetStaticMesh(Cubo.Object);
	}

	Slopes->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Slopes->SetCollisionResponseToAllChannels(ECR_Block);
	Rim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Rim->SetCollisionResponseToAllChannels(ECR_Block);

	// A lava e os derrames não param ninguém: encostar neles é o começo de uma
	// regra de dano que ainda não existe, e colisão sem regra só empurra o
	// jogador para fora da encosta sem explicar por quê.
	Lava->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flows->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Plume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plume->SetCastShadow(false);
	Plume->SetCanEverAffectNavigation(false);

	Slopes->SetCanEverAffectNavigation(false);
	Rim->SetCanEverAffectNavigation(false);
	Flows->SetCanEverAffectNavigation(false);
}

float AVolcano::GetCraterRadiusUnits() const
{
	return BaseRadiusUnits * FMath::Clamp(CraterRadiusFraction, 0.05f, 0.6f);
}

float AVolcano::GetLavaSurfaceUnits() const
{
	return FMath::Max(0.0f, HeightUnits - CraterDepthUnits);
}

float AVolcano::RadiusAtHeight(float ZUnits) const
{
	if (HeightUnits <= 0.0f)
	{
		return BaseRadiusUnits;
	}

	const float Fatia = FMath::Clamp(ZUnits, 0.0f, HeightUnits) / HeightUnits;
	return FMath::Lerp(BaseRadiusUnits, GetCraterRadiusUnits(), Fatia);
}

void AVolcano::BuildVolcano(uint32 Seed)
{
	Slopes->ClearInstances();
	Rim->ClearInstances();
	Flows->ClearInstances();
	RimBlocks.Reset();
	FlowSteps.Reset();

	Plume->ClearInstances();

	RaiseSlopes();
	CloseRim();
	PourFlows(Seed);
	RaisePlume(Seed);

	// A luz mora na boca da cratera, nao no centro do ator: no centro ela
	// acende a rocha por dentro e o cume continua escuro.
	CraterGlow->SetRelativeLocation(FVector(0.0, 0.0, GetLavaSurfaceUnits()));
	CraterGlow->SetAttenuationRadius(
		BaseRadiusUnits * FMath::Max(1.0f, GlowReachInBaseRadii));
	CraterGlow->SetIntensity(GlowIntensity);

	ScenaryPalette::PaintComponent(Slopes, EScenaryRole::VolcanicRock);
	ScenaryPalette::PaintComponent(Rim, EScenaryRole::VolcanicRock);
	ScenaryPalette::PaintComponent(Lava, EScenaryRole::LavaGlow);
	ScenaryPalette::PaintComponent(Flows, EScenaryRole::LavaGlow);
	ScenaryPalette::PaintComponent(Plume, EScenaryRole::AshPlume);
}

void AVolcano::RaiseSlopes()
{
	UStaticMesh* Disco = Slopes->GetStaticMesh();
	if (!Disco || SliceCount <= 0)
	{
		return;
	}

	const FVector Tamanho = Disco->GetBoundingBox().GetSize();
	if (Tamanho.X <= 0.0 || Tamanho.Z <= 0.0)
	{
		return;
	}

	// A pilha para na superfície da lava, não no cume: é justamente o que ela
	// NÃO ocupa que vira o poço da cratera.
	const float Fundo = GetLavaSurfaceUnits();
	const float Espessura = Fundo / static_cast<float>(SliceCount);

	for (int32 Camada = 0; Camada < SliceCount; ++Camada)
	{
		// O raio vem da BASE da camada, nunca do meio: usar o meio afinaria
		// cada disco meia camada cedo demais e abriria um degrau para dentro
		// entre um disco e o de baixo.
		const float ZDaBase = Espessura * static_cast<float>(Camada);
		const float Raio = RadiusAtHeight(ZDaBase);
		const float EscalaXY = (2.0f * Raio) / static_cast<float>(Tamanho.X);

		FTransform Camada3D;
		Camada3D.SetScale3D(FVector(
			EscalaXY, EscalaXY, Espessura / static_cast<float>(Tamanho.Z)));
		Camada3D.SetLocation(FVector(0.0, 0.0, ZDaBase + 0.5f * Espessura));
		Slopes->AddInstance(Camada3D);
	}

	UStaticMesh* MalhaDaLava = Lava->GetStaticMesh();
	if (!MalhaDaLava)
	{
		return;
	}

	const FVector TamanhoDaLava = MalhaDaLava->GetBoundingBox().GetSize();
	if (TamanhoDaLava.X <= 0.0 || TamanhoDaLava.Z <= 0.0)
	{
		return;
	}

	const float RaioDaLava = GetCraterRadiusUnits() * FMath::Clamp(LavaFillFraction, 0.1f, 0.99f);
	const float EscalaDaLava = (2.0f * RaioDaLava) / static_cast<float>(TamanhoDaLava.X);

	Lava->SetRelativeScale3D(FVector(
		EscalaDaLava, EscalaDaLava,
		LavaThicknessUnits / static_cast<float>(TamanhoDaLava.Z)));

	// A face de cima da lava fica NO nível da superfície; o disco afunda daí
	// para baixo. Assentá-lo sobre o nível faria a lava boiar acima do fundo.
	Lava->SetRelativeLocation(FVector(0.0, 0.0, Fundo - 0.5f * LavaThicknessUnits));
}

void AVolcano::CloseRim()
{
	UStaticMesh* Bloco = Rim->GetStaticMesh();
	if (!Bloco || RimBlockCount <= 0)
	{
		return;
	}

	const FVector Tamanho = Bloco->GetBoundingBox().GetSize();
	if (Tamanho.X <= 0.0 || Tamanho.Y <= 0.0 || Tamanho.Z <= 0.0)
	{
		return;
	}

	const float Fundo = GetLavaSurfaceUnits();
	const float Altura = HeightUnits - Fundo;
	if (Altura <= 0.0f)
	{
		return;
	}

	// A espessura do anel é MEDIDA, não escolhida: é o que sobra da encosta
	// entre a boca da cratera e o cume. Um número escrito à mão aqui seria uma
	// segunda fonte de verdade sobre a mesma forma (L-032).
	const float Espessura = RadiusAtHeight(Fundo) - RadiusAtHeight(HeightUnits);
	const float RaioDoAnel = GetCraterRadiusUnits() + 0.5f * Espessura;
	const float PassoAngular = 2.0f * UE_PI / static_cast<float>(RimBlockCount);
	const float Comprimento = RaioDoAnel * PassoAngular * FMath::Max(1.0f, RimOverlap);

	for (int32 Pedra = 0; Pedra < RimBlockCount; ++Pedra)
	{
		const float Angulo = PassoAngular * static_cast<float>(Pedra);

		FTransform Pedaco;
		Pedaco.SetScale3D(FVector(
			Espessura / static_cast<float>(Tamanho.X),
			Comprimento / static_cast<float>(Tamanho.Y),
			Altura / static_cast<float>(Tamanho.Z)));
		// Girar pelo próprio ângulo põe o X local apontando para fora, e é o
		// que faz a espessura do bloco ser radial em vez de sempre a mesma
		// direção do mundo.
		Pedaco.SetRotation(
			FRotator(0.0f, FMath::RadiansToDegrees(Angulo), 0.0f).Quaternion());
		Pedaco.SetLocation(FVector(
			FMath::Cos(Angulo) * RaioDoAnel,
			FMath::Sin(Angulo) * RaioDoAnel,
			Fundo + 0.5f * Altura));

		Rim->AddInstance(Pedaco);
		RimBlocks.Add(Pedaco);
	}
}

void AVolcano::PourFlows(uint32 Seed)
{
	UStaticMesh* Bloco = Flows->GetStaticMesh();
	if (!Bloco || FlowCount <= 0 || FlowStepCount <= 0)
	{
		return;
	}

	const FVector Tamanho = Bloco->GetBoundingBox().GetSize();
	if (Tamanho.X <= 0.0 || Tamanho.Y <= 0.0 || Tamanho.Z <= 0.0)
	{
		return;
	}

	const float Fundo = GetLavaSurfaceUnits();
	const float Degrau = Fundo / static_cast<float>(FlowStepCount);
	const float PassoEntreDerrames = 360.0f / static_cast<float>(FlowCount);

	for (int32 Derrame = 0; Derrame < FlowCount; ++Derrame)
	{
		// O ângulo de partida é sorteado dentro da fatia do derrame, nunca no
		// círculo inteiro: dois derrames sorteados livremente caem no mesmo
		// lado da encosta com frequência, e metade do vulcão fica sem nada.
		const float Boca = PassoEntreDerrames * static_cast<float>(Derrame)
			+ BattleSpread::Between(
				-VulcaoDaIlha::AberturaEntreDerrames, VulcaoDaIlha::AberturaEntreDerrames,
				BattleSpread::Fraction(Seed, Derrame));

		float Angulo = Boca;

		for (int32 Passo = 0; Passo < FlowStepCount; ++Passo)
		{
			// A lava desce: o passo zero é o mais alto, encostado na boca.
			const float ZDoPasso = Fundo - Degrau * (static_cast<float>(Passo) + 0.5f);

			Angulo += BattleSpread::Between(
				-VulcaoDaIlha::DesvioDoDerrame, VulcaoDaIlha::DesvioDoDerrame,
				BattleSpread::Fraction(Seed, 1000 + Derrame * FlowStepCount + Passo));

			const float EmRadianos = FMath::DegreesToRadians(Angulo);
			const float Raio = RadiusAtHeight(ZDoPasso);

			FTransform Pedaco;
			Pedaco.SetScale3D(FVector(
				FlowThicknessUnits / static_cast<float>(Tamanho.X),
				FlowWidthUnits / static_cast<float>(Tamanho.Y),
				(Degrau * VulcaoDaIlha::SobreposicaoDoDerrame)
					/ static_cast<float>(Tamanho.Z)));
			Pedaco.SetRotation(FRotator(0.0f, Angulo, 0.0f).Quaternion());
			Pedaco.SetLocation(FVector(
				FMath::Cos(EmRadianos) * Raio,
				FMath::Sin(EmRadianos) * Raio,
				ZDoPasso));

			Flows->AddInstance(Pedaco);
			FlowSteps.Add(Pedaco);
		}
	}
}

void AVolcano::RaisePlume(uint32 Seed)
{
	if (!Plume->GetStaticMesh() || PlumePuffCount <= 0 || PlumeHeightUnits <= 0.0f)
	{
		return;
	}

	const float Cume = GetHeightUnits();
	const float RaioDaCratera = GetCraterRadiusUnits();
	const float Degrau = PlumeHeightUnits / static_cast<float>(PlumePuffCount);

	for (int32 Bafo = 0; Bafo < PlumePuffCount; ++Bafo)
	{
		// Quanto mais alto, mais larga: fumaça que sobe com a mesma espessura
		// é um cano, e cano não conta que aquilo está se dissipando.
		const float Subida = static_cast<float>(Bafo) / static_cast<float>(PlumePuffCount);
		const float Largura = RaioDaCratera * VulcaoDaIlha::LarguraDoPrimeiroBafo
			* FMath::Lerp(1.0f, VulcaoDaIlha::AlargamentoDaColuna, Subida);

		const int32 Fluxo = VulcaoDaIlha::PrimeiroFluxoDoBafo + Bafo * 2;
		const float Deriva = RaioDaCratera * VulcaoDaIlha::DerivaDaColuna * Subida;

		FTransform Bola;
		Bola.SetLocation(FVector(
			BattleSpread::Between(-Deriva, Deriva, BattleSpread::Fraction(Seed, Fluxo)),
			BattleSpread::Between(-Deriva, Deriva, BattleSpread::Fraction(Seed, Fluxo + 1)),
			Cume + Degrau * (static_cast<float>(Bafo) + 0.5f)));
		Bola.SetScale3D(FVector(Largura * 2.0f / VulcaoDaIlha::LadoDaEsfera));
		Plume->AddInstance(Bola);
	}
}

UPointLightComponent* AVolcano::GetCraterGlow() const
{
	return CraterGlow;
}

UHierarchicalInstancedStaticMeshComponent* AVolcano::GetPlume() const
{
	return Plume;
}
