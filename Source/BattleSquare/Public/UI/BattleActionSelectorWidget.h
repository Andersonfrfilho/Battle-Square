// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Battle/BattleActionQueueComponent.h"
#include "BattleActionSelectorWidget.generated.h"

// T11 (tasks.md, PRES-01 a PRES-04): camada de exposição a Blueprint do
// UBattleActionQueueComponent — toda decisão de "pode ou não pode"
// continua no componente (T2–T4); este widget só encaminha chamadas e
// espelha o estado dele em propriedades BlueprintReadOnly. Nenhum layout
// UMG é criado aqui — ver design.md, Limite de Ferramenta: a autoria
// visual (posição dos 6 botões de tipo + roseta de direção) é DP-08,
// feita no UMG Designer, fora do que este C++ cobre.
UCLASS()
class BATTLESQUARE_API UBattleActionSelectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Associa este widget ao componente de fila de um pet específico —
	// chamado por quem monta a cena (ABattleArena/Blueprint), nunca cria
	// o componente sozinho. Liga os delegates de T2–T4 para manter o
	// estado espelhado atualizado.
	UFUNCTION(BlueprintCallable, Category = "Battle|ActionSelector")
	void BindToQueue(UBattleActionQueueComponent* Queue);

	UFUNCTION(BlueprintCallable, Category = "Battle|ActionSelector")
	bool BeginSelectingType(EActionType Type);

	UFUNCTION(BlueprintCallable, Category = "Battle|ActionSelector")
	bool ConfirmDirection(EBattleDirection Direction);

	UFUNCTION(BlueprintCallable, Category = "Battle|ActionSelector")
	bool CancelPendingSelection();

	UFUNCTION(BlueprintCallable, Category = "Battle|ActionSelector")
	bool RemoveLastAction();

	UFUNCTION(BlueprintCallable, Category = "Battle|ActionSelector")
	bool Commit();

	// Estado espelhado do componente — atualizado via delegate, nunca
	// recalculado aqui.
	UPROPERTY(BlueprintReadOnly, Category = "Battle|ActionSelector")
	EBattleActionSelectionStep CurrentStep = EBattleActionSelectionStep::ChoosingType;

	UPROPERTY(BlueprintReadOnly, Category = "Battle|ActionSelector")
	int32 ConfirmedActionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Battle|ActionSelector")
	bool bIsCommitted = false;

private:
	UPROPERTY()
	TObjectPtr<UBattleActionQueueComponent> BoundQueue;

	UFUNCTION()
	void RefreshStateFromQueue();
};
