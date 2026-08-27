// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Battle/BattleActionQueueComponent.h"
#include "BattleActionSelectorWidget.generated.h"

class UButton;
class UTextBlock;

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

	// --- Layout (DP-ui-03) ---
	// BindWidgetOptional, não BindWidget: o widget é criado sem árvore nos
	// testes headless, e exigir os botões ali quebraria a cobertura que
	// DP-ui-05 conta como automatizável. Cada uso é guardado por null.
	virtual void NativeConstruct() override;

	/**
	 * Teclado como caminho paralelo aos botões.
	 *
	 * Não é atalho de conveniência: enquanto o WBP não ganhar os botões de
	 * Camuflar/Voar/Submergir, é por aqui que o jogador alcança as ações que o
	 * oponente já usa contra ele. Sofrer uma regra sem poder usá-la é o pior
	 * estado possível para uma mecânica nova.
	 *
	 * Continua valendo DP-ui-01: a tecla ENCAMINHA ao componente, exatamente
	 * como o botão. Nenhuma regra mora aqui.
	 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Aguardar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Mover;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Atacar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Magia;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Defender;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Esquivar;

	// DP-ia-04. BindWidgetOptional segura a transição: enquanto o WBP não
	// ganhar os botões, a tela segue funcionando sem eles em vez de recusar
	// a criar o widget.
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Camuflar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Voar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Submergir;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_CimaEsquerda;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Cima;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_CimaDireita;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Esquerda;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Direita;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_BaixoEsquerda;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Baixo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_BaixoDireita;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Cancelar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Desfazer;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Commit;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Status;

public:
	/**
	 * De QUEM são as ações que estão sendo escolhidas agora.
	 *
	 * Vai para o Text_Status, e não para um campo novo: o WBP já tem esse
	 * campo, e um `BindWidgetOptional` novo nasceria invisível — informação
	 * que não aparece é o mesmo que informação que não existe.
	 */
	void SetChoosingForLabel(const FText& Label);

	const FText& GetChoosingForLabel() const { return ChoosingForLabel; }

private:
	FText ChoosingForLabel;

public:

	/**
	 * Casa do pet do jogador no início do turno. Quem sabe disso é a arena;
	 * a tela só usa para não OFERECER um movimento que sairia do tabuleiro.
	 */
	UFUNCTION(BlueprintCallable, Category = "Battle|ActionSelector")
	void SetOwningPetCell(uint8 Column, uint8 Row);

private:
	void WireButtons();

	/** Reflete o passo atual: tipos no passo 1, direções no passo 2. */
	void RefreshLayoutFromState();

	/** Desabilita as direções que tirariam o pet da grade (só ao Mover). */
	void RefreshDirectionAvailability();

	uint8 OwningPetColumn = 1;
	uint8 OwningPetRow = 1;

	void LogClick(const TCHAR* Rotulo);

	UFUNCTION() void OnClickAguardar();
	UFUNCTION() void OnClickMover();
	UFUNCTION() void OnClickAtacar();
	UFUNCTION() void OnClickMagia();
	UFUNCTION() void OnClickDefender();
	UFUNCTION() void OnClickEsquivar();
	UFUNCTION() void OnClickCamuflar();
	UFUNCTION() void OnClickVoar();
	UFUNCTION() void OnClickSubmergir();

	UFUNCTION() void OnClickCimaEsquerda();
	UFUNCTION() void OnClickCima();
	UFUNCTION() void OnClickCimaDireita();
	UFUNCTION() void OnClickEsquerda();
	UFUNCTION() void OnClickDireita();
	UFUNCTION() void OnClickBaixoEsquerda();
	UFUNCTION() void OnClickBaixo();
	UFUNCTION() void OnClickBaixoDireita();

	UFUNCTION() void OnClickCancelar();
	UFUNCTION() void OnClickDesfazer();
	UFUNCTION() void OnClickCommit();

	UPROPERTY()
	TObjectPtr<UBattleActionQueueComponent> BoundQueue;

	UFUNCTION()
	void RefreshStateFromQueue();
};
