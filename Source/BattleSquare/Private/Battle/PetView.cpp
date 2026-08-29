// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetView.h"
#include "Battle/BattleTypes.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace PetSilhueta
{
	constexpr int32 NumeroDePatas = 4;
	constexpr float PataAfastamentoX = 16.0f;
	constexpr float PataAfastamentoY = 13.0f;
	constexpr float PataEspessura = 0.11f;

	/** Quanto a pata ENTRA no corpo, para não sobrar costura entre os dois. */
	constexpr float EncaixeDaPataNoCorpo = 4.0f;

	// Ovalado, não redondo: a esfera perfeita não tem frente nem costas, e
	// era por isso que o pet precisava de um cubo orbitando para mostrar
	// para onde olhava.
	const FVector CorpoEscala = FVector(0.66f, 0.52f, 0.48f);
	constexpr float CabecaEscala = 0.36f;
	constexpr float FocinhoEscala = 0.14f;
	const FVector FocinhoLocal = FVector(18.0f, 0.0f, -4.0f);
	constexpr float AdornoAfastamentoY = 10.0f;
	constexpr float AdornoAlturaZ = 14.0f;

	/** Quanto o adorno ENTRA na cabeça, para não sobrar costura entre os dois. */
	constexpr float EncaixeDoAdornoNaCabeca = 3.0f;

	const FVector CaudaEscala = FVector(0.16f, 0.16f, 0.40f);
	const FVector CaudaLocal = FVector(-34.0f, 0.0f, 44.0f);
	const FRotator CaudaRotacao = FRotator(30.0f, 0.0f, 0.0f);

	/** Patas e focinho ficam mais escuros que o corpo, para o contorno aparecer. */
	constexpr float EscurecimentoDasPatas = 0.55f;
	const FLinearColor CorDoFocinho = FLinearColor(0.10f, 0.09f, 0.09f);

	void Pintar(UStaticMeshComponent* Componente, const FLinearColor& Cor)
	{
		if (!Componente)
		{
			return;
		}

		// Reaproveita a instância dinâmica que já estiver no componente: criar
		// uma por chamada vazaria uma material nova a cada evento do trace.
		UMaterialInstanceDynamic* Dinamico = Cast<UMaterialInstanceDynamic>(Componente->GetMaterial(0));
		if (!Dinamico)
		{
			Dinamico = Componente->CreateDynamicMaterialInstance(0);
		}
		if (Dinamico)
		{
			Dinamico->SetVectorParameterValue(TEXT("Color"), Cor);
		}
	}
}

APetView::APetView()
{
	PrimaryActorTick.bCanEverTick = false;

	// Conteúdo da engine, não vendorizado — mesmo princípio de AD-019.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	SphereAsset = Esfera.Succeeded() ? Esfera.Object : nullptr;
	CubeAsset = Cubo.Succeeded() ? Cubo.Object : nullptr;
	ConeAsset = Cone.Succeeded() ? Cone.Object : nullptr;
	CylinderAsset = Cilindro.Succeeded() ? Cilindro.Object : nullptr;

	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	SetRootComponent(BodyRoot);

	BodyPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyPivot"));
	BodyPivot->SetupAttachment(BodyRoot);

	BuildBody();
	BuildHead();
	BuildLegs();
	BuildHealthBar();

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialBasico(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MaterialBasico.Succeeded())
	{
		TInlineComponentArray<UStaticMeshComponent*> Malhas(this);
		for (UStaticMeshComponent* Malha : Malhas)
		{
			Malha->SetMaterial(0, MaterialBasico.Object);
		}
	}
}

void APetView::BuildBody()
{
	using namespace PetSilhueta;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(BodyPivot);
	// Sem colisão: o pet é apresentação, e o núcleo é quem decide onde ele
	// está. Um corpo que empurra outro seria uma segunda fonte de verdade.
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetStaticMesh(SphereAsset);
	BodyMesh->SetRelativeScale3D(CorpoEscala);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BodyCenterUnits));

	TailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailMesh"));
	TailMesh->SetupAttachment(BodyPivot);
	TailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TailMesh->SetStaticMesh(ConeAsset);
	TailMesh->SetRelativeScale3D(CaudaEscala);
	TailMesh->SetRelativeLocationAndRotation(CaudaLocal, CaudaRotacao);
}

void APetView::BuildHead()
{
	using namespace PetSilhueta;

	HeadPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivot"));
	HeadPivot->SetupAttachment(BodyPivot);
	HeadPivot->SetRelativeLocation(FVector(HeadForwardUnits, 0.0f, HeadCenterUnits));

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(HeadPivot);
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetStaticMesh(SphereAsset);
	HeadMesh->SetRelativeScale3D(FVector(CabecaEscala));

	GazeMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GazeMarker"));
	GazeMarker->SetupAttachment(HeadPivot);
	GazeMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GazeMarker->SetStaticMesh(SphereAsset);
	GazeMarker->SetRelativeScale3D(FVector(FocinhoEscala));
	GazeMarker->SetRelativeLocation(FocinhoLocal);

	CrestLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrestLeft"));
	CrestLeft->SetupAttachment(HeadPivot);
	CrestLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrestLeft->SetStaticMesh(ConeAsset);

	CrestRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrestRight"));
	CrestRight->SetupAttachment(HeadPivot);
	CrestRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrestRight->SetStaticMesh(ConeAsset);

	RefreshCrests();
}

float APetView::BodyUnderSurfaceAtLegUnits()
{
	using namespace PetSilhueta;

	// As primitivas da engine têm 100uu de lado/diâmetro: o semieixo é
	// metade disso vezes a escala.
	const float SemiEixoX = CorpoEscala.X * CubeSizeUnits * 0.5f;
	const float SemiEixoY = CorpoEscala.Y * CubeSizeUnits * 0.5f;
	const float SemiEixoZ = CorpoEscala.Z * CubeSizeUnits * 0.5f;

	const float Normalizado = FMath::Square(PataAfastamentoX / SemiEixoX)
		+ FMath::Square(PataAfastamentoY / SemiEixoY);

	const float Queda = SemiEixoZ * FMath::Sqrt(FMath::Max(0.0f, 1.0f - Normalizado));
	return BodyCenterUnits - Queda;
}

float APetView::BodyLowestPointUnits()
{
	using namespace PetSilhueta;
	return BodyCenterUnits - CorpoEscala.Z * CubeSizeUnits * 0.5f;
}

float APetView::LegHeightUnits()
{
	return BodyUnderSurfaceAtLegUnits() + PetSilhueta::EncaixeDaPataNoCorpo;
}

void APetView::BuildLegs()
{
	using namespace PetSilhueta;

	static const TCHAR* NomesDasPatas[NumeroDePatas] =
	{
		TEXT("LegFrontLeft"), TEXT("LegFrontRight"), TEXT("LegBackLeft"), TEXT("LegBackRight")
	};

	for (int32 Indice = 0; Indice < NumeroDePatas; ++Indice)
	{
		UStaticMeshComponent* Pata = CreateDefaultSubobject<UStaticMeshComponent>(NomesDasPatas[Indice]);
		Pata->SetupAttachment(BodyPivot);
		Pata->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pata->SetStaticMesh(CylinderAsset);
		const float Altura = LegHeightUnits();
		Pata->SetRelativeScale3D(FVector(PataEspessura, PataEspessura, Altura / CubeSizeUnits));

		const float Frente = (Indice < 2) ? PataAfastamentoX : -PataAfastamentoX;
		const float Lado = (Indice % 2 == 0) ? -PataAfastamentoY : PataAfastamentoY;
		Pata->SetRelativeLocation(FVector(Frente, Lado, Altura * 0.5f));

		Legs.Add(Pata);
	}
}

void APetView::BuildHealthBar()
{
	HealthBarBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthBarBackground"));
	HealthBarBackground->SetupAttachment(BodyRoot);
	HealthBarBackground->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthBarFill = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthBarFill"));
	HealthBarFill->SetupAttachment(BodyRoot);
	HealthBarFill->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthBarBackground->SetStaticMesh(CubeAsset);
	HealthBarFill->SetStaticMesh(CubeAsset);

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

UStaticMesh* APetView::CrestMeshFor(EPetCrestShape Shape) const
{
	switch (Shape)
	{
		case EPetCrestShape::Folha:
			return SphereAsset;
		case EPetCrestShape::Barbatana:
		case EPetCrestShape::Chama:
		case EPetCrestShape::Orelha:
		case EPetCrestShape::OrelhaCaida:
		default:
			return ConeAsset;
	}
}

float APetView::HeadRadiusUnits()
{
	return PetSilhueta::CabecaEscala * CubeSizeUnits * 0.5f;
}

float APetView::CrestEmbedUnits()
{
	return PetSilhueta::EncaixeDoAdornoNaCabeca;
}

FRotator APetView::CrestRotationForSide(const FRotator& CrestRotation, float LateralSign)
{
	// Os dois espelham o Roll: sem isso tombam para o mesmo lado, e o bicho
	// fica com a cabeça torta em vez de simétrica.
	//
	// E tombam para FORA, que é o que tira o adorno da frente da silhueta da
	// cabeça no ângulo do diorama. O sinal é NEGATIVO porque um Roll aplicado
	// a (0,0,h) rende Y = h*sen(Roll), e a tabela de tipos traz Roll negativo:
	// trocar o sinal "para ficar mais óbvio" deitou os dois para dentro, e o
	// teste mediu a ponta chegando mais perto do meio que a base.
	return FRotator(CrestRotation.Pitch, CrestRotation.Yaw, -LateralSign * CrestRotation.Roll);
}

FVector APetView::CrestRelativeLocation(const FPetAppearance& Appearance, float LateralSign)
{
	using namespace PetSilhueta;

	const FRotator Rotacao = CrestRotationForSide(Appearance.CrestRotation, LateralSign);
	const float MeiaAltura = Appearance.CrestScale.Z * CubeSizeUnits * 0.5f;
	const FVector DaBaseAoCentro = Rotacao.RotateVector(FVector(0.0f, 0.0f, MeiaAltura));

	// O ponto de encaixe é achado pela DIREÇÃO na esfera, não por um Z: a
	// base cai sempre na superfície, seja qual for a escala ou a inclinação
	// que o tipo pedir. Foi o Z fixo que enterrou o cone de 26uu na testa.
	const FVector Direcao =
		FVector(0.0f, LateralSign * AdornoAfastamentoY, AdornoAlturaZ).GetSafeNormal();
	const FVector Base = Direcao * (HeadRadiusUnits() - EncaixeDoAdornoNaCabeca);

	return Base + DaBaseAoCentro;
}

void APetView::RefreshCrests()
{
	using namespace PetSilhueta;

	if (!CrestLeft || !CrestRight)
	{
		return;
	}

	const FPetAppearance Aparencia = FPetAppearance::ForType(PetType);
	UStaticMesh* Malha = CrestMeshFor(Aparencia.CrestShape);

	CrestLeft->SetStaticMesh(Malha);
	CrestRight->SetStaticMesh(Malha);
	CrestLeft->SetRelativeScale3D(Aparencia.CrestScale);
	CrestRight->SetRelativeScale3D(Aparencia.CrestScale);

	CrestLeft->SetRelativeLocationAndRotation(
		CrestRelativeLocation(Aparencia, -1.0f),
		CrestRotationForSide(Aparencia.CrestRotation, -1.0f));
	CrestRight->SetRelativeLocationAndRotation(
		CrestRelativeLocation(Aparencia, 1.0f),
		CrestRotationForSide(Aparencia.CrestRotation, 1.0f));
}

void APetView::LookAtLocation(const FVector& TargetLocation)
{
	if (!BodyPivot)
	{
		return;
	}

	FVector Towards = TargetLocation - GetActorLocation();
	Towards.Z = 0.0f;

	if (Towards.IsNearlyZero())
	{
		// Em cima do outro: não há para onde olhar, e escolher uma direção
		// qualquer daria a impressão de que o pet se distraiu.
		return;
	}

	// O bicho inteiro vira. A cabeça é a frente, e é ela que diz para onde
	// ele olha — a barra de vida fica fora deste pivô e não acompanha.
	BodyPivot->SetWorldRotation(Towards.Rotation());
}

void APetView::GlideTo(const FVector& Destination)
{
	// Distância desprezível não merece animação: um deslize de meio
	// centímetro só atrasa o evento seguinte.
	if (FVector::DistSquared(GetActorLocation(), Destination) < 1.0f)
	{
		SetActorLocation(Destination);
		bIsGliding = false;
		return;
	}

	GlideStart = GetActorLocation();
	GlideTarget = Destination;
	GlideElapsed = 0.0f;
	bIsGliding = true;
}

void APetView::AdvanceGlide(float DeltaSeconds)
{
	if (!bIsGliding)
	{
		return;
	}

	GlideElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(GlideElapsed / GlideSeconds, 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(GlideStart, GlideTarget, Alpha));

	if (Alpha >= 1.0f)
	{
		bIsGliding = false;
	}
}

void APetView::LookUp()
{
	if (HeadPivot)
	{
		HeadPivot->SetRelativeRotation(FRotator(HeadPitchDegrees, 0.0f, 0.0f));
	}
}

void APetView::LookDown()
{
	if (HeadPivot)
	{
		HeadPivot->SetRelativeRotation(FRotator(-HeadPitchDegrees, 0.0f, 0.0f));
	}
}

void APetView::LoseSightOfTarget()
{
	// Perder de vista é NÃO mexer: o pet fica virado para a última casa
	// conhecida, que é exatamente a informação errada que ele tem. Girar para
	// o lugar certo entregaria ao jogador o que a camuflagem deveria esconder.
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

void APetView::SetMeshesVisible(bool bVisible)
{
	// Derrotado some do tabuleiro — o núcleo já o tirou da partida. Percorrer
	// os componentes em vez de listar cada malha à mão evita que uma parte
	// nova do bicho fique flutuando sozinha no lugar do pet morto.
	TInlineComponentArray<UStaticMeshComponent*> Malhas(this);
	for (UStaticMeshComponent* Malha : Malhas)
	{
		if (Malha != HealthBarBackground && Malha != HealthBarFill)
		{
			Malha->SetVisibility(bVisible);
		}
	}
}

void APetView::RefreshBodyAppearance()
{
	using namespace PetSilhueta;

	if (!BodyMesh)
	{
		return;
	}

	const FLinearColor CorDoLado = Side == 0 ? LocalSideColor : OpponentSideColor;
	const FPetAppearance Aparencia = FPetAppearance::ForType(PetType);

	Pintar(BodyMesh, CorDoLado);
	Pintar(HeadMesh, CorDoLado);
	for (UStaticMeshComponent* Pata : Legs)
	{
		Pintar(Pata, CorDoLado * EscurecimentoDasPatas);
	}

	Pintar(CrestLeft, Aparencia.AccentColor);
	Pintar(CrestRight, Aparencia.AccentColor);
	// A cauda usa a cor do LADO, não a de acento: branca sobre um corpo azul
	// ela lia como uma asa colada, uma peça de outro bicho. O tipo continua
	// sendo dito pelos adornos, que é onde se olha para saber o tipo.
	Pintar(TailMesh, CorDoLado * EscurecimentoDasPatas);
	Pintar(GazeMarker, CorDoFocinho);

	SetMeshesVisible(!bDefeated);
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

	// O tipo vinha na apresentação e era descartado aqui: dois pets de tipos
	// diferentes ficavam idênticos na tela, e a skill que só um deles tem não
	// tinha de onde ser adivinhada.
	PetType = Presentation.Type;

	RefreshCrests();
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
