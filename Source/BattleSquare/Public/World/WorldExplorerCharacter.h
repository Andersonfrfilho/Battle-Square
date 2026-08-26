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

	/**
	 * É AQUI que o Enhanced Input se registra, não em BeginPlay: um pawn
	 * SPAWNADO roda BeginPlay ANTES de ser possuído, então GetController()
	 * ainda é nulo e o mapping context nunca entraria. Sintoma: o personagem
	 * nasce e não responde a tecla nem a mouse.
	 */
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	UEncounterDetectionComponent* GetEncounterDetection() const { return EncounterDetection; }

protected:
	// Assets de input ficam FORA do C++ (DP-trav-06): a decisão de teclas não
	// exige recompilar. Ausência degrada para "não anda", nunca para crash.
	//
	// EditAnywhere, não EditDefaultsOnly: este ator é colocado DIRETO no nível,
	// sem Blueprint — com EditDefaultsOnly não existiria lugar nenhum onde
	// atribuir os assets, e o personagem ficaria permanentemente sem input.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> TraversalMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	void HandleMove(const FInputActionValue& Value);

	/**
	 * Sonda de diagnóstico, por binding de tecla CRU (caminho legado), ao lado
	 * do Enhanced Input. Ela separa duas perguntas que o silêncio confundia:
	 * "a tecla chega ao jogo?" e "o Enhanced Input a roteia?".
	 */
	void LogRawKeyProbe();
	void HandleLook(const FInputActionValue& Value);

private:
	UPROPERTY(VisibleAnywhere, Category = "Câmera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Câmera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Encontro")
	TObjectPtr<UEncounterDetectionComponent> EncounterDetection;
};
