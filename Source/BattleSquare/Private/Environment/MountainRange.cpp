// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/MountainRange.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/IslandGeography.h"
#include "Environment/ScenaryPalette.h"
#include "UObject/ConstructorHelpers.h"

namespace SerraDoHorizonte
{
	/**
	 * A rocha esculpida do kit, e o cone da engine como reserva.
	 *
	 * Foi só o cone por muito tempo, e o jogador leu a serra inteira como
	 * "montanhas pontudas extremamente esquisitas": 22 picos com três cones
	 * cada dá 66 chapéus de festa no horizonte. O cone passa em todo teste
	 * numérico porque teste não olha silhueta.
	 *
	 * A reserva não é zelo: componente sem asset é invisível, e uma serra
	 * invisível some do horizonte sem quebrar um teste sequer.
	 */
	const TCHAR* MalhaDoCorpo = TEXT("/Game/Environment/Nature/rock_tallA.rock_tallA");
	const TCHAR* MalhaDeReserva = TEXT("/Engine/BasicShapes/Cone.Cone");

	/** Quantas montanhas em volta. */
	constexpr int32 NumeroDePicos = 22;

	/** Contrafortes por montanha: é o que tira o cone de ser um cone. */
	constexpr int32 ContrafortesPorPico = 2;

	/**
	 * Quantos sorteios cada pico consome.
	 *
	 * Reservado com folga de propósito: um fluxo por índice significa que um
	 * sorteio novo entra no fim sem deslocar os que já existem, e a serra que
	 * já foi vista continua a mesma.
	 */
	constexpr int32 SorteiosPorPico = 16;

	// A faixa de tamanhos. Elas mandam na neve: com a linha da neve do bioma
	// temperado em ~2461 m, só a parte de cima desta faixa congela — que é
	// exatamente "montanha DEPENDENDO DO TAMANHO forma gelo", e não "montanha
	// tem gelo".
	constexpr float AlturaMinimaMetros = 900.0f;
	constexpr float AlturaMaximaMetros = 3300.0f;

	/** A que distância a serra fica. Longe o bastante para a atmosfera azular. */
	constexpr float DistanciaMinimaMetros = 6000.0f;
	constexpr float DistanciaMaximaMetros = 11000.0f;

	/** Raio da base como fração da altura: montanha larga contra montanha esguia. */
	constexpr float RaioMinimoPorAltura = 0.72f;
	constexpr float RaioMaximoPorAltura = 1.15f;

	/** Achatamento no eixo Y — a base deixa de ser círculo e vira elipse. */
	constexpr float AchatamentoMinimo = 0.70f;
	constexpr float AchatamentoMaximo = 1.38f;

	/** Altura do contraforte como fração do corpo principal. */
	constexpr float OmbroMinimoPorAltura = 0.36f;
	constexpr float OmbroMaximoPorAltura = 0.68f;

	/** Quanto o contraforte se afasta, como fração da soma dos dois raios. */
	constexpr float AfastamentoMinimo = 0.45f;
	constexpr float AfastamentoMaximo = 0.82f;

	/**
	 * O gelo é desenhado 2% mais largo que a rocha que ele cobre.
	 *
	 * Duas superfícies exatamente coincidentes brigam pelo mesmo pixel e
	 * piscam conforme a câmera anda. Dois centésimos resolvem, e a essa
	 * distância ninguém mede a borda.
	 */
	constexpr float FolgaDoGelo = 1.06f;

	/** Os sorteios do corpo principal. */
	enum class ESorteio : int32
	{
		Angulo = 0,
		Distancia,
		Altura,
		Raio,
		Achatamento,
		Giro,
		/** A partir daqui, cinco por contraforte. */
		PrimeiroContraforte
	};

	constexpr int32 SorteiosPorContraforte = 5;

	int32 Fluxo(int32 Pico, ESorteio Sorteio)
	{
		return Pico * SorteiosPorPico + static_cast<int32>(Sorteio);
	}

	int32 FluxoDoContraforte(int32 Pico, int32 Contraforte, int32 Passo)
	{
		return Pico * SorteiosPorPico
			+ static_cast<int32>(ESorteio::PrimeiroContraforte)
			+ Contraforte * SorteiosPorContraforte
			+ Passo;
	}
}

AMountainRange::AMountainRange()
{
	PrimaryActorTick.bCanEverTick = false;

	RangeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RangeRoot"));
	SetRootComponent(RangeRoot);

	// A malha é atribuída AQUI, no construtor, e não na montagem. Componente
	// criado sem asset passa em todo teste de lógica e não existe na tela —
	// o padrão que já custou três defeitos a este projeto.
	ConstructorHelpers::FObjectFinder<UStaticMesh> Rocha(SerraDoHorizonte::MalhaDoCorpo);
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(SerraDoHorizonte::MalhaDeReserva);

	RockPeaks = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("MountainRockPeaks"));
	RockPeaks->SetupAttachment(RangeRoot);

	SnowCaps = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("MountainSnowCaps"));
	SnowCaps->SetupAttachment(RangeRoot);

	UStaticMesh* Silhueta = Rocha.Succeeded() ? Rocha.Object
		: (Cone.Succeeded() ? Cone.Object : nullptr);
	if (Silhueta)
	{
		RockPeaks->SetStaticMesh(Silhueta);
		SnowCaps->SetStaticMesh(Silhueta);
	}

	for (UHierarchicalInstancedStaticMeshComponent* Grupo : { RockPeaks.Get(), SnowCaps.Get() })
	{
		// Cenário a quilômetros não empurra ninguém, e uma sombra de três
		// quilômetros estouraria as cascatas do sol para nada aparecer.
		Grupo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Grupo->SetCastShadow(false);
		Grupo->SetCanEverAffectNavigation(false);
	}
}

void AMountainRange::BuildRange(EScenaryClimate Climate, uint32 Seed)
{
	BuildRangeWith(Seed, [Climate](const FVector&) { return Climate; });
}

void AMountainRange::BuildRangeAcrossIsland(uint32 Seed)
{
	// A base do pico é RELATIVA ao ator; a geografia responde em coordenada do
	// mundo. Somar a posição do ator é o que faz a serra plantada no norte
	// perguntar pelo norte.
	const FVector Origem = GetActorLocation();
	BuildRangeWith(Seed, [Origem](const FVector& Base)
	{
		return IslandGeography::SectorClimateAt(FVector2D(Origem + Base));
	});
}

void AMountainRange::BuildRangeWith(uint32 Seed,
	TFunctionRef<EScenaryClimate(const FVector&)> ClimateAtBase)
{
	if (!RockPeaks || !SnowCaps)
	{
		return;
	}

	RockPeaks->ClearInstances();
	SnowCaps->ClearInstances();
	PeakHeightsMeters.Reset();

	ScenaryPalette::PaintComponent(RockPeaks, EScenaryRole::MountainRock);
	ScenaryPalette::PaintComponent(SnowCaps, EScenaryRole::MountainSnow);

	using namespace SerraDoHorizonte;

	const float PassoAngular = 2.0f * PI / static_cast<float>(NumeroDePicos);

	for (int32 Pico = 0; Pico < NumeroDePicos; ++Pico)
	{
		const float Desvio = BattleSpread::Between(-0.6f, 0.6f,
			BattleSpread::Fraction(Seed, Fluxo(Pico, ESorteio::Angulo))) * PassoAngular;
		const float Angulo = Pico * PassoAngular + Desvio;

		const float DistanciaMetros = BattleSpread::Between(
			DistanciaMinimaMetros, DistanciaMaximaMetros,
			BattleSpread::Fraction(Seed, Fluxo(Pico, ESorteio::Distancia)));

		const float AlturaMetros = BattleSpread::Between(
			AlturaMinimaMetros, AlturaMaximaMetros,
			BattleSpread::Fraction(Seed, Fluxo(Pico, ESorteio::Altura)));

		const float RaioMetros = AlturaMetros * BattleSpread::Between(
			RaioMinimoPorAltura, RaioMaximoPorAltura,
			BattleSpread::Fraction(Seed, Fluxo(Pico, ESorteio::Raio)));

		const float Achatamento = BattleSpread::Between(
			AchatamentoMinimo, AchatamentoMaximo,
			BattleSpread::Fraction(Seed, Fluxo(Pico, ESorteio::Achatamento)));

		const float Giro = BattleSpread::Between(0.0f, 360.0f,
			BattleSpread::Fraction(Seed, Fluxo(Pico, ESorteio::Giro)));

		const FVector Base(
			FMath::Cos(Angulo) * DistanciaMetros * UnitsPerMeter,
			FMath::Sin(Angulo) * DistanciaMetros * UnitsPerMeter,
			0.0f);

		// O clima sai do PICO, uma vez, e os contrafortes herdam: eles são a
		// saia da mesma montanha, e perguntar por corpo deixaria um ombro
		// nevado ao lado de um cume pelado do mesmo pico.
		const EScenaryClimate Climate = ClimateAtBase(Base);

		RaiseBody(Base, AlturaMetros, RaioMetros, Achatamento, Giro, Climate);

		for (int32 Ombro = 0; Ombro < ContrafortesPorPico; ++Ombro)
		{
			const float AlturaDoOmbro = AlturaMetros * BattleSpread::Between(
				OmbroMinimoPorAltura, OmbroMaximoPorAltura,
				BattleSpread::Fraction(Seed, FluxoDoContraforte(Pico, Ombro, 0)));

			const float RaioDoOmbro = AlturaDoOmbro * BattleSpread::Between(
				RaioMinimoPorAltura, RaioMaximoPorAltura,
				BattleSpread::Fraction(Seed, FluxoDoContraforte(Pico, Ombro, 1)));

			// O contraforte sai de LADO, na tangente da roda de montanhas, e
			// não na direção da câmera: é assim que ele alarga a silhueta em
			// vez de se esconder atrás do corpo principal.
			const float Lado = (Ombro % 2 == 0) ? 1.0f : -1.0f;
			const float Torto = BattleSpread::Between(-0.4f, 0.4f,
				BattleSpread::Fraction(Seed, FluxoDoContraforte(Pico, Ombro, 2)));
			const float AnguloDoOmbro = Angulo + Lado * (HALF_PI + Torto);

			const float Afastamento = (RaioMetros + RaioDoOmbro) * BattleSpread::Between(
				AfastamentoMinimo, AfastamentoMaximo,
				BattleSpread::Fraction(Seed, FluxoDoContraforte(Pico, Ombro, 3)));

			const FVector BaseDoOmbro = Base + FVector(
				FMath::Cos(AnguloDoOmbro) * Afastamento * UnitsPerMeter,
				FMath::Sin(AnguloDoOmbro) * Afastamento * UnitsPerMeter,
				0.0f);

			const float GiroDoOmbro = BattleSpread::Between(0.0f, 360.0f,
				BattleSpread::Fraction(Seed, FluxoDoContraforte(Pico, Ombro, 4)));

			RaiseBody(BaseDoOmbro, AlturaDoOmbro, RaioDoOmbro,
				Achatamento, GiroDoOmbro, Climate);
		}
	}
}

void AMountainRange::RaiseBody(const FVector& BaseLocation, float HeightMeters,
	float BaseRadiusMeters, float FlatteningY, float YawDegrees, EScenaryClimate Climate)
{
	UStaticMesh* Malha = RockPeaks ? RockPeaks->GetStaticMesh().Get() : nullptr;
	if (!Malha || HeightMeters <= 0.0f || BaseRadiusMeters <= 0.0f)
	{
		return;
	}

	// A escala sai da caixa MEDIDA da malha, nunca de dimensão escrita à mão:
	// o dia em que o cone da engine mudar de tamanho, a serra continua com a
	// altura que o clima usou para decidir a neve.
	const FBox Caixa = Malha->GetBoundingBox();
	const FVector Tamanho = Caixa.GetSize();
	if (Tamanho.X <= 0.0 || Tamanho.Y <= 0.0 || Tamanho.Z <= 0.0)
	{
		return;
	}

	const float AlturaEmUnidades = HeightMeters * UnitsPerMeter;
	const float DiametroEmUnidades = 2.0f * BaseRadiusMeters * UnitsPerMeter;

	const float EscalaZ = AlturaEmUnidades / static_cast<float>(Tamanho.Z);
	const float EscalaX = DiametroEmUnidades / static_cast<float>(Tamanho.X);
	const float EscalaY = (DiametroEmUnidades / static_cast<float>(Tamanho.Y)) * FlatteningY;

	const FQuat Rotacao = FRotator(0.0f, YawDegrees, 0.0f).Quaternion();
	const double RecuoDaBase = Caixa.Min.Z;

	FTransform Corpo;
	Corpo.SetScale3D(FVector(EscalaX, EscalaY, EscalaZ));
	Corpo.SetRotation(Rotacao);
	Corpo.SetLocation(BaseLocation - FVector(0.0, 0.0, RecuoDaBase * EscalaZ));
	RockPeaks->AddInstance(Corpo);

	PeakHeightsMeters.Add(HeightMeters);

	const float Fatia = ScenaryClimate::SnowCapFraction(Climate, HeightMeters);
	if (Fatia <= 0.0f || !SnowCaps)
	{
		return;
	}

	// O cume é o PRÓPRIO corpo reduzido à fatia que passa da linha da neve, e
	// erguido até ela. Com o cone o encaixe era exato, porque o cone afina em
	// linha reta; com a rocha esculpida ele é APROXIMADO — a folga existe para
	// o gelo transbordar um pouco em vez de deixar rocha aparecendo por fora,
	// que é o erro que se enxerga.
	const float EscalaZDoGelo = EscalaZ * Fatia;
	FTransform Cume;
	Cume.SetScale3D(FVector(
		EscalaX * Fatia * SerraDoHorizonte::FolgaDoGelo,
		EscalaY * Fatia * SerraDoHorizonte::FolgaDoGelo,
		EscalaZDoGelo));
	Cume.SetRotation(Rotacao);
	Cume.SetLocation(BaseLocation + FVector(
		0.0, 0.0,
		(1.0f - Fatia) * AlturaEmUnidades - RecuoDaBase * EscalaZDoGelo));
	SnowCaps->AddInstance(Cume);
}

int32 AMountainRange::GetPeakCount() const
{
	return PeakHeightsMeters.Num();
}

int32 AMountainRange::GetSnowCapCount() const
{
	return SnowCaps ? SnowCaps->GetInstanceCount() : 0;
}
