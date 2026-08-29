// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WorldExplorerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/**
	 * Guarda-queda: abaixo do limite, devolve o explorador à última terra
	 * firme conhecida.
	 *
	 * O usuário atravessou os blocos e caiu ETERNAMENTE. Sem chão infinito e
	 * sem um piso autorado perfeito, cair é sempre possível — o que não pode
	 * existir é a queda que não termina, porque ela não devolve o jogo a
	 * ninguém e a única saída vira fechar o programa.
	 *
	 * Isto NÃO substitui consertar o piso; ele é a rede embaixo do trapézio.
	 */
	void CheckFallGuard();

	/** Registra a posição atual como terra firme, se ele estiver no chão. */
	void RememberSafeGround();

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

public:
	/** Correr enquanto a tecla estiver pressionada. */
	void StartSprinting();
	void StopSprinting();

	/** Alterna terceira pessoa → primeira pessoa → de cima → terceira. */
	void CycleCameraMode();

protected:

	/**
	 * Sonda de diagnóstico, por binding de tecla CRU (caminho legado), ao lado
	 * do Enhanced Input. Ela separa duas perguntas que o silêncio confundia:
	 * "a tecla chega ao jogo?" e "o Enhanced Input a roteia?".
	 */
	void LogRawKeyProbe();
	void HandleLook(const FInputActionValue& Value);

private:
	// Bem abaixo de qualquer relevo jogável: acionar cedo demais impediria
	// pular de um degrau alto.
	/**
	 * Modos de câmera.
	 *
	 * Terceira pessoa é o padrão porque o encontro exige ver o pet A DISTÂNCIA
	 * para decidir abordar (DP-enc-02). Primeira pessoa esconde isso, e por
	 * isso não pode ser o padrão — mas é opção, que foi o que o usuário pediu.
	 */
	enum class ECameraMode : uint8
	{
		TerceiraPessoa = 0,
		PrimeiraPessoa,
		DeCima,
		MAX
	};

	void ApplyCameraMode();

	ECameraMode CameraMode = ECameraMode::TerceiraPessoa;
	float WalkSpeedBeforeSprint = 0.0f;

	static constexpr float SprintSpeedMultiplier = 1.8f;
	static constexpr float FirstPersonArmLengthUnits = 0.0f;
	static constexpr float TopDownArmLengthUnits = 900.0f;

	static constexpr float BodyScale = 0.9f;
	static constexpr float FacingMarkerForwardUnits = 55.0f;

	static constexpr float FallGuardZLimit = -2000.0f;

	// Um pouco acima da marca, para ele não renascer dentro do piso.
	static constexpr float RespawnLiftUnits = 100.0f;

	FVector LastSafeGround = FVector::ZeroVector;
	bool bHasSafeGround = false;

public:
	/**
	 * Corpo visível do explorador.
	 *
	 * ACharacter traz um componente de malha ESQUELETAL, mas sem asset — e
	 * sem asset o jogador dirige um corpo invisível em terceira pessoa. É o
	 * mesmo defeito de APetView e dos inimigos do mundo, e nenhum teste de
	 * lógica o acusa. Malha estática da engine resolve sem depender de
	 * autoria; o modelo 3D substitui isto quando chegar.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> FacingMarker;

private:
	UPROPERTY(VisibleAnywhere, Category = "Câmera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Câmera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Encontro")
	TObjectPtr<UEncounterDetectionComponent> EncounterDetection;
};
