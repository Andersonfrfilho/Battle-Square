// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Battle/BattleActionSelection.h"
#include "Battle/BattleTypes.h"
#include "BattleActionQueueComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBattleActionQueueChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBattleActionQueueCommittedSignature);

// T2–T4 (tasks.md): máquina de estado da fila de ações do jogador,
// isolada de qualquer coisa visual — testável headless, mesmo princípio
// que isolou BattleSim do Engine (design.md, Limite de Ferramenta).
// O widget (UMG, fora deste componente) só consome os UFUNCTIONs e
// reage aos delegates; toda decisão de "pode ou não pode" mora aqui.
UCLASS(ClassGroup = (BattleSquare), meta = (BlueprintSpawnableComponent))
class BATTLESQUARE_API UBattleActionQueueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Passo 1: escolhe o tipo. Se o tipo não precisar de direção
	// (Defender/Esquivar/Aguardar), confirma na hora. Se precisar
	// (Mover/Atacar/Magia), avança para ChoosingDirection sem
	// adicionar à fila ainda.
	// Retorna false se a fila já estiver em 3/3 ou já commitada.
	UFUNCTION(BlueprintCallable, Category = "Battle|ActionQueue")
	bool BeginSelectingType(EActionType Type);

	// Passo 2: confirma a direção do tipo escolhido em BeginSelectingType.
	// Fora do passo ChoosingDirection, é ignorado (retorna false, sem
	// efeito colateral).
	UFUNCTION(BlueprintCallable, Category = "Battle|ActionQueue")
	bool ConfirmDirection(EBattleDirection Direction);

	// Volta do passo 2 para o passo 1, sem alterar ações já confirmadas.
	UFUNCTION(BlueprintCallable, Category = "Battle|ActionQueue")
	bool CancelPendingSelection();

	// Desfaz só a última ação confirmada. Sem efeito em fila vazia ou
	// já commitada.
	UFUNCTION(BlueprintCallable, Category = "Battle|ActionQueue")
	bool RemoveLastAction();

	// Trava a fila. Se houver menos de 3 ações, completa com Aguardar
	// antes de travar (mesma regra do núcleo, BTL-01/BTL-20).
	UFUNCTION(BlueprintCallable, Category = "Battle|ActionQueue")
	bool Commit();

	// Não exposto ao Blueprint: FTurnCommit tem um array C puro
	// (Actions[3]) que UHT não suporta como retorno Blueprint. Quem
	// consome isto é a fiação em C++ (ABattleArena, T10), não o widget.
	FTurnCommit BuildCommit() const;

	UFUNCTION(BlueprintPure, Category = "Battle|ActionQueue")
	int32 GetConfirmedActionCount() const { return ConfirmedActions.Num(); }

	UFUNCTION(BlueprintPure, Category = "Battle|ActionQueue")
	bool IsCommitted() const { return bCommitted; }

	UFUNCTION(BlueprintPure, Category = "Battle|ActionQueue")
	EBattleActionSelectionStep GetCurrentStep() const { return Pending.Step; }

	UPROPERTY(BlueprintAssignable, Category = "Battle|ActionQueue")
	FBattleActionQueueChangedSignature OnQueueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Battle|ActionQueue")
	FBattleActionQueueCommittedSignature OnCommitted;

private:
	UPROPERTY()
	TArray<FBattleAction> ConfirmedActions;

	UPROPERTY()
	FBattlePendingActionSelection Pending;

	UPROPERTY()
	bool bCommitted = false;
};
