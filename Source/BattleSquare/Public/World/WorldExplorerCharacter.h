// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WorldExplorerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UEncounterDetectionComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

namespace WorldTraversal
{
	inline constexpr float CameraArmLengthUnits = 450.0f;
	inline constexpr float CameraPitchMinDegrees = -70.0f;
	inline constexpr float CameraPitchMaxDegrees = 45.0f;
	inline constexpr float CharacterRotationRateYawDegreesPerSecond = 540.0f;
}

/**
 * Pawn do jogador no mundo aberto. NÃO substitui o DebugRoutePawn: aquele é a
 * régua determinística dos roteiros de verificação, este é quem o jogador dirige.
 */
UCLASS()
class BATTLESQUARE_API AWorldExplorerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWorldExplorerCharacter();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	UEncounterDetectionComponent* GetEncounterDetection() const { return EncounterDetection; }

protected:
	// Assets de input ficam FORA do C++ (DP-trav-06): a decisão de teclas não
	// exige recompilar. Ausência degrada para "não anda", nunca para crash.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> TraversalMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);

private:
	UPROPERTY(VisibleAnywhere, Category = "Câmera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Câmera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Encontro")
	TObjectPtr<UEncounterDetectionComponent> EncounterDetection;
};
