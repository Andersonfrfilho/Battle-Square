// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Battle/BattleState.h"
#include "Battle/PetView.h"
#include "Battle/BattleActionQueueComponent.h"
#include "Battle/BattleTracePlayer.h"
#include "Net/BattleTurnCoordinator.h"
#include "Data/BattleDataTranslator.h"
#include "BattleArena.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UMaterialInterface;

// T9 (tasks.md, PRES-06/07/08): scaffold da cena de combate — câmera fixa
// enquadrando a grade 3x3, spawn de APetView a partir do estado inicial.
// Estética final (tilt-shift, material fosco de verdade) é verificação
// visual manual — ver T14. O que este ator garante por código é: as 9
// casas caem dentro do frustum da câmera, e a grade usa um material
// configurável (nunca cor hardcoded).
UCLASS()
class BATTLESQUARE_API ABattleArena : public AActor
{
	GENERATED_BODY()

public:
	ABattleArena();

	// Tamanho de uma casa da grade, em unidades do mundo — a única fonte
	// de verdade para o espaçamento; nunca um número mágico espalhado.
	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	float CellSize = 150.0f;

	// Material da grade — configurável por Blueprint/instância, nunca uma
	// cor hex fixa no C++.
	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	TSoftObjectPtr<UMaterialInterface> GridMaterial;

	// Centro (em espaço de mundo) da casa (Column, Row), Column/Row em [0,2].
	UFUNCTION(BlueprintPure)
	FVector GetCellWorldLocation(uint8 Column, uint8 Row) const;

	// Cria um APetView por pet do estado inicial, posicionado na casa
	// correspondente. Não exposto ao Blueprint — FBattleState/
	// FPetPresentationInfo não são BlueprintType.
	void SpawnPetViews(const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations);

	const TArray<TObjectPtr<APetView>>& GetPetViews() const { return SpawnedPetViews; }

	// Checagem programática (PRES-06/07): projeta os 9 centros de casa no
	// espaço da câmera e confirma que caem dentro do FOV configurado.
	// Não depende de PlayerController/Viewport — só de FOV+AspectRatio+
	// transform da câmera, para ser testável headless.
	UFUNCTION(BlueprintCallable)
	bool AreAllGridCellsInCameraFrustum() const;

	// T10: fiação de ponta a ponta. Guarda o estado inicial, spawna as
	// views e deixa a fila do jogador (PlayerActionQueue) pronta para
	// receber seleções. Assim que o jogador commitar, a IA gera o commit
	// dela, o resolvedor real roda, e o trace resultante anima as views.
	// Não exposto ao Blueprint pelo mesmo motivo de SpawnPetViews.
	//
	// T7 (arenas-variadas, ARENA-02): retorna false, sem montar nada, se
	// algum pet estivesse posicionado numa casa Blocked — erro de
	// configuração, nunca reposicionado silenciosamente.
	bool BeginBattle(const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations);

	const FBattleState& GetCurrentState() const { return CurrentState; }

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBattleActionQueueComponent> PlayerActionQueue;

	// T8 (tasks.md, Combate Online, NET-09/NET-10): registra que o
	// oponente é um jogador humano real, servido por InCoordinator — a
	// partir daqui, HandlePlayerCommitted delega ao coordenador em vez de
	// chamar FDumbOpponentAI. Presença de oponente real decide o caminho,
	// nunca uma flag de "modo online/offline" separada. Sem esta chamada,
	// o comportamento continua byte a byte o de antes (Standalone).
	void ConfigureNetworkedOpponent(UBattleTurnCoordinator* InCoordinator);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ArenaRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> ArenaCamera;

private:
	UPROPERTY()
	TArray<TObjectPtr<APetView>> SpawnedPetViews;

	UPROPERTY()
	TObjectPtr<UBattleTracePlayer> TracePlayer;

	UPROPERTY()
	FBattleState CurrentState;

	// Presença, não flag: null quando o oponente é FDumbOpponentAI
	// (Standalone/sem oponente humano); setado por ConfigureNetworkedOpponent
	// quando um jogador real está do outro lado.
	UPROPERTY()
	TObjectPtr<UBattleTurnCoordinator> ServerCoordinator;

	bool IsPointInCameraFrustum(const FVector& WorldPoint) const;

	UFUNCTION()
	void HandlePlayerCommitted();

	void HandleCoordinatorTurnResolved(const FBattleState& NextState, const TArray<FBattleEvent>& Trace);

	void DispatchEventToPetViews(const FBattleEvent& Event);
};
