// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldExplorerCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/BattleDebugScreen.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/WorldTraversalMotion.h"
#include "World/EncounterDetectionComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

AWorldExplorerCharacter::AWorldExplorerCharacter()
{
	// Sem isto o guarda-queda nunca roda, e a queda infinita continua.
	PrimaryActorTick.bCanEverTick = true;

	// CORPO VISÍVEL. ACharacter traz malha esqueletal sem asset: o jogador
	// dirigiria um corpo invisível em terceira pessoa, e nenhum teste acusa.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	// Sem colisão: quem colide é a cápsula do Character. Uma segunda forma
	// colidindo brigaria com ela e produziria empurrões inexplicáveis.
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
	FacingMarker->SetupAttachment(RootComponent);
	FacingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CorpoMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CorpoMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CorpoMesh.Object);
		BodyMesh->SetRelativeScale3D(FVector(BodyScale));
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MarcaMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (MarcaMesh.Succeeded())
	{
		// A esfera é simétrica: sem uma marca à frente, não dá para saber para
		// onde o personagem está virado — e direção é o que mais importa num
		// mundo onde encostar em alguém inicia uma batalha.
		FacingMarker->SetStaticMesh(MarcaMesh.Object);
		FacingMarker->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
		FacingMarker->SetRelativeLocation(FVector(FacingMarkerForwardUnits, 0.0f, 0.0f));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CorpoMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CorpoMaterial.Succeeded())
	{
		BodyMesh->SetMaterial(0, CorpoMaterial.Object);
		FacingMarker->SetMaterial(0, CorpoMaterial.Object);
	}

	// DP-trav-04: a câmera orbita livre e o corpo vira para onde anda. Sem
	// isto o personagem fica travado apontando para a câmera e "anda de
	// lado", que é comportamento de shooter, não de explorador.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0, WorldTraversal::CharacterRotationRateYawDegreesPerSecond, 0.0);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = WorldTraversal::CameraArmLengthUnits;
	CameraBoom->bUsePawnControlRotation = true;
	// É isto, e só isto, que impede a câmera de entrar na parede.
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// A rotação vive num lugar só: o braço.
	FollowCamera->bUsePawnControlRotation = false;

	// DP-trav-05: encontro é o componente da feature anterior, sem exceção.
	EncounterDetection = CreateDefaultSubobject<UEncounterDetectionComponent>(TEXT("EncounterDetection"));
}

void AWorldExplorerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] NotifyControllerChanged: controller=%s IMC=%s Move=%s Look=%s"),
		PlayerController ? TEXT("ok") : TEXT("NULO"),
		TraversalMappingContext ? TEXT("ok") : TEXT("NULO"),
		MoveAction ? TEXT("ok") : TEXT("NULO"),
		LookAction ? TEXT("ok") : TEXT("NULO"));

	if (!PlayerController)
	{
		return;
	}

	PlayerController->PlayerCameraManager->ViewPitchMin = WorldTraversal::CameraPitchMinDegrees;
	PlayerController->PlayerCameraManager->ViewPitchMax = WorldTraversal::CameraPitchMaxDegrees;

	// Contexto ausente é configuração incompleta, não motivo para cair.
	if (!TraversalMappingContext)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TRAVERSAL] subsistema de Enhanced Input NULO"));
		return;
	}

	const int32 MappingsNoAsset = TraversalMappingContext->GetMappings().Num();
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] mapeamentos no asset: %d"), MappingsNoAsset);

	// Reserva: um asset sem mapeamento não move ninguém, e descobrir isso só
	// pelo silêncio custou caro. Construir o contexto pela API da engine
	// (MapKey) não depende de como a ferramenta de autoria escreveu o asset.
	UInputMappingContext* ContextToApply = TraversalMappingContext;
	if (MappingsNoAsset == 0 && MoveAction && LookAction)
	{
		UInputMappingContext* Fallback = NewObject<UInputMappingContext>(this);
		Fallback->MapKey(MoveAction, EKeys::W);
		Fallback->MapKey(MoveAction, EKeys::S).Modifiers.Add(NewObject<UInputModifierNegate>(Fallback));

		FEnhancedActionKeyMapping& Right = Fallback->MapKey(MoveAction, EKeys::D);
		Right.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Fallback));

		FEnhancedActionKeyMapping& Left = Fallback->MapKey(MoveAction, EKeys::A);
		Left.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Fallback));
		Left.Modifiers.Add(NewObject<UInputModifierNegate>(Fallback));

		Fallback->MapKey(LookAction, EKeys::Mouse2D);

		ContextToApply = Fallback;
		UE_LOG(LogTemp, Warning, TEXT("[TRAVERSAL] asset sem mapeamentos — usando contexto de RESERVA construído em C++ (%d)"),
			Fallback->GetMappings().Num());
	}

	Subsystem->AddMappingContext(ContextToApply, /*Priority=*/0);
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] mapping context REGISTRADO (%d mapeamentos)"),
		ContextToApply->GetMappings().Num());
}

void AWorldExplorerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] SetupPlayerInputComponent: enhanced=%s Move=%s"),
		EnhancedInput ? TEXT("ok") : TEXT("NULO"), MoveAction ? TEXT("ok") : TEXT("NULO"));
	if (!EnhancedInput)
	{
		return;
	}

	// Sonda crua, no caminho legado: dispara se a tecla chegar ao jogo, mesmo
	// que o Enhanced Input não roteie nada.
	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &AWorldExplorerCharacter::LogRawKeyProbe);

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWorldExplorerCharacter::HandleMove);
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWorldExplorerCharacter::HandleLook);
	}
}

void AWorldExplorerCharacter::HandleMove(const FInputActionValue& Value)
{
	const AController* OwningController = GetController();
	if (!OwningController)
	{
		return;
	}

	FWorldTraversalMotionParams Params;
	Params.MovementInput = Value.Get<FVector2D>();
	Params.CameraRotation = OwningController->GetControlRotation();

	const FVector MoveDirection = FWorldTraversalMotion::ComputeMoveDirection(Params);
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] HandleMove entrada=(%.2f,%.2f) direcao=(%.2f,%.2f,%.2f)"),
		Params.MovementInput.X, Params.MovementInput.Y, MoveDirection.X, MoveDirection.Y, MoveDirection.Z);
	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	AddMovementInput(MoveDirection);
}

void AWorldExplorerCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AWorldExplorerCharacter::LogRawKeyProbe()
{
	UE_LOG(LogTemp, Warning, TEXT("[TRAVERSAL] SONDA: a tecla W CHEGOU ao jogo (binding cru)."));
}

void AWorldExplorerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Azul, contra o laranja dos inimigos: a cor é o que separa você deles a
	// distância, antes de qualquer detalhe de forma.
	if (BodyMesh)
	{
		if (UMaterialInstanceDynamic* Material = BodyMesh->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.45f, 0.95f));
		}
	}
	if (FacingMarker)
	{
		if (UMaterialInstanceDynamic* Material = FacingMarker->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.95f, 0.95f));
		}
	}
}

void AWorldExplorerCharacter::RememberSafeGround()
{
	LastSafeGround = GetActorLocation();
	bHasSafeGround = true;
}

void AWorldExplorerCharacter::CheckFallGuard()
{
	if (GetActorLocation().Z > FallGuardZLimit)
	{
		return;
	}

	// Sem marca conhecida, o ponto de partida é o melhor palpite disponível —
	// e é infinitamente melhor que continuar caindo.
	const FVector Destino = bHasSafeGround
		? LastSafeGround + FVector(0.0f, 0.0f, RespawnLiftUnits)
		: FVector(0.0f, 0.0f, RespawnLiftUnits);

	SetActorLocation(Destino);

	// Zerar a velocidade: sem isso ele reaparece já caindo na mesma
	// velocidade acumulada e atravessa o piso de novo no quadro seguinte.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	FBattleDebugScreen::Show(
		TEXT("caiu para fora do mundo — devolvido à última terra firme"),
		6.0f, FColor::Orange, /*Key=*/700);
}

void AWorldExplorerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Terra firme só conta quando ele está DE FATO no chão: gravar durante a
	// queda registraria o próprio buraco como lugar seguro.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (Movement->IsMovingOnGround())
		{
			RememberSafeGround();
		}
	}

	CheckFallGuard();
}
