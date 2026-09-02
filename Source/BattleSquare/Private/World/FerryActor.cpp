// Copyright 2026 Anderson. All Rights Reserved.

#include "World/FerryActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/ScenaryPalette.h"
#include "UObject/ConstructorHelpers.h"
#include "World/TrailLayout.h"

namespace Balsa
{
	const TCHAR* MalhaDoConves = TEXT("/Engine/BasicShapes/Cube.Cube");

	/**
	 * Quanto ela flutua acima da lâmina.
	 *
	 * Baixo: a balsa ANDA SOBRE a água, não voa sobre ela. E tem de ficar bem
	 * abaixo do vão de uma ponte — construída na altura de um tabuleiro, ela
	 * viraria uma ponte que se move, e a distinção que o traçado fez entre
	 * "largo demais para ponte" e "ponte" desapareceria.
	 */
	constexpr float BordaLivre = 30.0f;

	/** A travessia leva tempo, e é isso que faz esperar por ela ser jogo. */
	constexpr float VelocidadeUnidadesPorSegundo = 400.0f;

	/** O lado da malha básica da engine. */
	constexpr float LadoDaMalhaBasica = 100.0f;

	/** A espessura do convés, como fração da largura. */
	constexpr float Espessura = 0.15f;
}

float AFerryActor::FreeboardUnits()
{
	return Balsa::BordaLivre;
}

float AFerryActor::SpeedUnitsPerSecond()
{
	return Balsa::VelocidadeUnidadesPorSegundo;
}

AFerryActor::AFerryActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Deck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FerryDeck"));
	SetRootComponent(Deck);

	// MALHA E COR NO CONSTRUTOR. Componente que nasce sem asset passa em todo
	// teste de lógica e não existe na tela — três vezes neste projeto.
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(Balsa::MalhaDoConves);
	if (Cubo.Succeeded())
	{
		Deck->SetStaticMesh(Cubo.Object);
	}

	ScenaryPalette::PaintComponent(Deck, EScenaryRole::DeadWood);

	// SÓLIDA, e MÓVEL. As duas coisas juntas são o que faz dela balsa:
	// sólida para carregar quem sobe e esbarrar no que houver, móvel para
	// poder andar. Um convés estático atravessaria a geometria por dentro em
	// vez de bater nela.
	Deck->SetMobility(EComponentMobility::Movable);
	Deck->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Deck->SetCollisionResponseToAllChannels(ECR_Block);

	// Quem está em cima vai junto: sem isto o jogador fica parado no ar
	// enquanto a balsa sai de baixo dele.
	Deck->SetCanEverAffectNavigation(false);
	Deck->bFillCollisionUnderneathForNavmesh = false;
}

bool AFerryActor::ConfigureFor(const FVector2D& Meio, const FVector2D& Eixo,
	float Vao, float LaminaZ, EFluidKind Fluido)
{
	// SÓ HÁ BALSA ONDE ELA BOIA.
	//
	// A altura dela vinha da lâmina, e por isso ela flutuaria igual em
	// qualquer coisa — inclusive num fluido menos denso que ela, onde uma
	// balsa de verdade iria ao fundo. O empuxo depende dos DOIS lados, e é o
	// registro que sabe os dois.
	//
	// Recusar é diferente de nascer parada: uma balsa que afunda não é uma
	// travessia ruim, é uma travessia que não existe.
	if (!FluidRegistry::FloatsOn(DeckDensityPerMille, Fluido))
	{
		return false;
	}

	Center = Meio;
	Axis = Eixo.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : Eixo.GetSafeNormal();
	HalfSpanUnits = FMath::Max(Vao, 1.0f) * 0.5f;
	SurfaceZ = LaminaZ;
	OffsetUnits = -HalfSpanUnits;
	Heading = 1;

	// O tamanho vem da largura da trilha que ela continua: a balsa é o pedaço
	// de caminho que falta, e um convés mais estreito que a trilha faria a
	// pessoa cair ao pisar nele.
	const float Largura = (TrailLayout::HalfWidthUnits() * 2.0f)
		/ Balsa::LadoDaMalhaBasica;
	Deck->SetWorldScale3D(
		FVector(Largura, Largura, Largura * Balsa::Espessura));

	SetActorLocation(FVector(
		Center.X + Axis.X * OffsetUnits,
		Center.Y + Axis.Y * OffsetUnits,
		SurfaceZ + Balsa::BordaLivre));

	return true;
}

bool AFerryActor::MoverPara(float NovoOffset)
{
	const FVector Destino(
		Center.X + Axis.X * NovoOffset,
		Center.Y + Axis.Y * NovoOffset,
		SurfaceZ + Balsa::BordaLivre);

	// VARRENDO, e não teleportando. É a diferença entre esbarrar no que
	// estiver no caminho e atravessar por dentro dele — e atravessar por
	// dentro é exatamente o que uma geometria sólida não pode fazer.
	FHitResult Bateu;
	SetActorLocation(Destino, /*bSweep=*/true, &Bateu);

	if (Bateu.bBlockingHit)
	{
		// Bateu: volta para o outro lado. Insistir contra o obstáculo faria a
		// balsa tremer no lugar até alguém tirar a coisa da frente.
		Heading = -Heading;
		OffsetUnits = FMath::Clamp(
			static_cast<float>(
				(FVector2D(GetActorLocation()) - Center) | Axis),
			-HalfSpanUnits, HalfSpanUnits);
		return false;
	}

	OffsetUnits = NovoOffset;
	return true;
}

void AFerryActor::SetCurrent(const FVector2D& Rumo, int32 ForcaPorMil)
{
	CurrentDirection = Rumo.IsNearlyZero() ? FVector2D::ZeroVector : Rumo.GetSafeNormal();
	CurrentStrengthPerMille = CurrentDirection.IsNearlyZero()
		? 0
		: FMath::Max(0, ForcaPorMil);
}

float AFerryActor::CurrentSpeedUnitsPerSecond() const
{
	// SÓ A COMPONENTE AO LONGO DO EIXO conta. A parte perpendicular empurra a
	// balsa de lado, e empurrar de lado não muda quanto tempo ela leva para
	// chegar do outro lado — é deriva, não atraso.
	//
	// O sinal do rumo entra junto: indo a favor, a componente soma; voltando,
	// a mesma componente subtrai. É o que faz a ida e a volta durarem
	// diferente sem nenhuma regra dizendo "a volta é mais lenta".
	const float AoLongo = static_cast<float>(
		(CurrentDirection | (Axis * static_cast<float>(Heading))));

	const float DaCorrente = Balsa::VelocidadeUnidadesPorSegundo
		* (static_cast<float>(CurrentStrengthPerMille) / 1000.0f) * AoLongo;

	// Piso em um décimo: uma corrente forte o bastante para ZERAR o passo
	// deixaria a balsa parada para sempre no meio do rio, e travessia que não
	// termina não é travessia — é armadilha silenciosa.
	return FMath::Max(Balsa::VelocidadeUnidadesPorSegundo * 0.1f,
		Balsa::VelocidadeUnidadesPorSegundo + DaCorrente);
}

bool AFerryActor::AdvanceBy(float DeltaSeconds)
{
	if (HalfSpanUnits <= 0.0f || DeltaSeconds <= 0.0f)
	{
		return false;
	}

	const float Antes = OffsetUnits;
	float Andado = OffsetUnits
		+ Heading * CurrentSpeedUnitsPerSecond() * DeltaSeconds;

	// Chegou na margem: vira. Ela vai e VOLTA — uma balsa que só vai deixa a
	// outra margem sem travessia para sempre.
	if (Andado >= HalfSpanUnits)
	{
		Andado = HalfSpanUnits;
		Heading = -1;
	}
	else if (Andado <= -HalfSpanUnits)
	{
		Andado = -HalfSpanUnits;
		Heading = 1;
	}

	MoverPara(Andado);

	return !FMath::IsNearlyEqual(OffsetUnits, Antes);
}

void AFerryActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceBy(DeltaSeconds);
}
