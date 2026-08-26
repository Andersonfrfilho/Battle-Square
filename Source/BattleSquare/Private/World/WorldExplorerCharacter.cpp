// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldExplorerCharacter.h"
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

AWorldExplorerCharacter::AWorldExplorerCharacter()
{
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

void AWorldExplorerCharacter::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(TraversalMappingContext, /*Priority=*/0);
	}
}

void AWorldExplorerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

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
