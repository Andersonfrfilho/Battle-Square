// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/AuroraCurtain.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/IslandGeography.h"
#include "Environment/ScenaryPalette.h"
#include "UObject/ConstructorHelpers.h"

namespace AuroraDaGeleira
{
	/**
	 * O cubo da engine, e não o plano.
	 *
	 * O plano seria o formato certo e o objeto errado: ele tem UMA face, e a
	 * cortina desapareceria para quem chegasse à geleira pelo outro lado —
	 * defeito que só aparece a quem der a volta, ou seja, tarde.
	 */
	const TCHAR* MalhaDaFita = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cube);

	/** Abaixo disto a aurora não se vê, e mancha verde no céu é pior que nada. */
	constexpr float ForcaMinimaVisivel = 0.02f;

	/** Onde a geleira fica, em fração do raio da ilha. Nem na praia, nem no miolo. */
	constexpr float RaioDaGeleiraEmFracao = 0.62f;

	/** O primeiro fluxo de sorteio da faixa de cima, longe do da faixa de baixo. */
	constexpr int32 PrimeiroFluxoDoVerde = 0;
	constexpr int32 PrimeiroFluxoDoRoxo = 9000;

	/** O quanto a onda de uma fita sai de fase da anterior, em voltas. */
	constexpr float DesencontroEntreFitas = 0.37f;
}

AAuroraCurtain::AAuroraCurtain()
{
	PrimaryActorTick.bCanEverTick = false;

	CurtainRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AuroraRoot"));
	SetRootComponent(CurtainRoot);

	// Malha atribuída no construtor, não na montagem: componente sem asset passa
	// em todo teste de lógica e não existe na tela.
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(AuroraDaGeleira::MalhaDaFita);

	Veil = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("AuroraVeil"));
	Veil->SetupAttachment(CurtainRoot);

	Crown = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("AuroraCrown"));
	Crown->SetupAttachment(CurtainRoot);

	if (Cubo.Succeeded())
	{
		Veil->SetStaticMesh(Cubo.Object);
		Crown->SetStaticMesh(Cubo.Object);
	}

	// Nada de colisão, de sombra e de navegação: a cortina está a nove mil
	// unidades do chão, e uma sombra dura vinda dela riscaria a geleira inteira
	// com a silhueta de um cubo — que é o defeito que a pluma do vulcão já
	// tinha ensinado.
	for (UHierarchicalInstancedStaticMeshComponent* Faixa : { Veil.Get(), Crown.Get() })
	{
		Faixa->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Faixa->SetCastShadow(false);
		Faixa->SetCanEverAffectNavigation(false);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("AuroraGlow"));
	Glow->SetupAttachment(CurtainRoot);
	Glow->SetLightColor(ScenaryPalette::ColorFor(EScenaryRole::AuroraVeil, NAME_None));
	Glow->SetCastShadows(false);

	// Nasce apagada. Uma aurora que aparece no instante do spawn e só depois
	// obedece ao relógio pisca verde ao meio-dia na primeira partida.
	SetStrength(0.0f);
}

FVector2D AAuroraCurtain::SkyCenterUnits()
{
	const float PassoDoSetor = 360.0f / static_cast<float>(IslandGeography::SectorCount);

	for (int32 Setor = 0; Setor < IslandGeography::SectorCount; ++Setor)
	{
		if (IslandGeography::BiomeOfSector(Setor) != EIslandBiome::Glacier)
		{
			continue;
		}

		const float Rumo = FMath::DegreesToRadians(
			PassoDoSetor * (static_cast<float>(Setor) + 0.5f));
		const float Raio = IslandGeography::LandRadiusUnits()
			* AuroraDaGeleira::RaioDaGeleiraEmFracao;

		return FVector2D(FMath::Cos(Rumo), FMath::Sin(Rumo)) * Raio;
	}

	// Ilha sem geleira: sem lugar de aurora, e o centro é o único ponto que não
	// finge um setor que não existe.
	return FVector2D::ZeroVector;
}

void AAuroraCurtain::BuildCurtain(uint32 Seed)
{
	Veil->ClearInstances();
	Crown->ClearInstances();
	VeilSegments.Reset();
	CrownSegments.Reset();

	HangBand(Veil, VeilSegments, AltitudeUnits, VeilHeightUnits,
		Seed, AuroraDaGeleira::PrimeiroFluxoDoVerde);

	// O roxo pousa exatamente onde o verde termina: uma folga entre as duas
	// faixas apareceria como uma linha preta atravessando a cortina no céu.
	HangBand(Crown, CrownSegments, AltitudeUnits + VeilHeightUnits, CrownHeightUnits,
		Seed, AuroraDaGeleira::PrimeiroFluxoDoRoxo);

	// A luz mora na BASE da cortina, não no meio dela: no meio, metade do
	// alcance se gasta iluminando o céu vazio acima.
	Glow->SetRelativeLocation(FVector(0.0, 0.0, AltitudeUnits));
	Glow->SetAttenuationRadius(GlowReachUnits);

	TingeBands();
}

void AAuroraCurtain::HangBand(
	UHierarchicalInstancedStaticMeshComponent* Band,
	TArray<FTransform>& Registro,
	float BottomUnits,
	float HeightUnits,
	uint32 Seed,
	int32 PrimeiroFluxo)
{
	if (!Band || !Band->GetStaticMesh() || RibbonCount <= 0 || SegmentCount <= 0)
	{
		return;
	}

	const FVector Tamanho = Band->GetStaticMesh()->GetBoundingBox().GetSize();
	if (Tamanho.X <= 0.0 || Tamanho.Y <= 0.0 || Tamanho.Z <= 0.0)
	{
		return;
	}

	const float MeioArco = ArcSpanDegrees * 0.5f;
	const float ZDoMeio = BottomUnits + HeightUnits * 0.5f;

	for (int32 Fita = 0; Fita < RibbonCount; ++Fita)
	{
		const float RaioDaFita = ArcRadiusUnits
			+ RibbonSpacingUnits * static_cast<float>(Fita);

		// A fase sorteada é o que impede as fitas de ondularem juntas: em fase,
		// as três sobem e descem como uma só, e a cortina volta a ser a faixa
		// única que ela existe para não ser.
		const float Fase = BattleSpread::Fraction(Seed, PrimeiroFluxo + Fita) * UE_TWO_PI
			+ AuroraDaGeleira::DesencontroEntreFitas * UE_TWO_PI * static_cast<float>(Fita);

		// O ponto da fita na fração pedida do arco. Um só lugar calcula isto,
		// e é dele que saem a posição, o comprimento E o rumo do pedaço: rumo
		// calculado por outra conta desencontra da posição na primeira edição.
		const auto Ponto = [&](float Fracao) -> FVector2D
		{
			const float Rumo = FMath::DegreesToRadians(
				FMath::Lerp(-MeioArco, MeioArco, Fracao));
			const float Serpente = FMath::Sin(Fase + UE_TWO_PI * WaveCycles * Fracao)
				* WaveAmplitudeUnits;
			const float Raio = RaioDaFita + Serpente;

			return FVector2D(FMath::Cos(Rumo), FMath::Sin(Rumo)) * Raio;
		};

		for (int32 Pedaco = 0; Pedaco < SegmentCount; ++Pedaco)
		{
			const FVector2D Comeco = Ponto(
				static_cast<float>(Pedaco) / static_cast<float>(SegmentCount));
			const FVector2D Fim = Ponto(
				static_cast<float>(Pedaco + 1) / static_cast<float>(SegmentCount));

			const FVector2D Corda = Fim - Comeco;
			const float Comprimento = static_cast<float>(Corda.Size());
			if (Comprimento <= 0.0f)
			{
				continue;
			}

			const FVector2D Meio = (Comeco + Fim) * 0.5f;

			FTransform Fatia;
			Fatia.SetScale3D(FVector(
				SegmentThicknessUnits / static_cast<float>(Tamanho.X),
				(Comprimento * SegmentOverlap) / static_cast<float>(Tamanho.Y),
				HeightUnits / static_cast<float>(Tamanho.Z)));
			Fatia.SetRotation(FRotator(0.0f,
				FMath::RadiansToDegrees(FMath::Atan2(Corda.Y, Corda.X)), 0.0f).Quaternion());
			Fatia.SetLocation(FVector(Meio.X, Meio.Y, ZDoMeio));

			Band->AddInstance(Fatia);
			Registro.Add(Fatia);
		}
	}
}

void AAuroraCurtain::SetStrength(float NovaForca)
{
	Strength = FMath::Clamp(NovaForca, 0.0f, 1.0f);

	const bool bAcesa = Strength > AuroraDaGeleira::ForcaMinimaVisivel;

	Veil->SetVisibility(bAcesa);
	Crown->SetVisibility(bAcesa);
	Glow->SetVisibility(bAcesa);
	Glow->SetIntensity(GlowIntensity * Strength);

	TingeBands();
}

void AAuroraCurtain::TingeBands()
{
	// A matiz sai da paleta; daqui sai só QUANTO dela. O nome do parâmetro de
	// cor mora num arquivo só, e é por isso que a modulação é `TintComponent` e
	// não uma tinta montada aqui.
	//
	// Faixa sem pedaço nenhum não tem o que pintar, e é esse o caso do
	// construtor: instância dinâmica de material criada ali ficaria presa ao
	// arquétipo da CLASSE, e valeria para toda aurora que nascesse depois.
	if (Veil->GetInstanceCount() > 0)
	{
		ScenaryPalette::TintComponent(Veil, EScenaryRole::AuroraVeil, Strength);
	}

	if (Crown->GetInstanceCount() > 0)
	{
		ScenaryPalette::TintComponent(Crown, EScenaryRole::AuroraCrown, Strength);
	}
}

UPointLightComponent* AAuroraCurtain::GetGlow() const
{
	return Glow;
}
