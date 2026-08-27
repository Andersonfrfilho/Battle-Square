// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Net/BattleNetCommitComponent.h"
#include "BattleSquarePlayerController.generated.h"

// T7 (tasks.md, Sala e Pareamento Simples): RPCs finos — só repassam
// para ABattleSquareGameMode/UBattleRoomRegistry, sem lógica própria
// (mesmo espírito de UBattleNetCommitComponent::Server_SubmitCommit).
UCLASS()
class BATTLESQUARE_API ABattleSquarePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/**
	 * As teclas de depuração (F7 dois lados, F9 copia, F10 limpa) NÃO são
	 * amarradas aqui.
	 *
	 * Amarrar no InputComponent depende de foco e de modo de input, e foi
	 * assim que elas não chegaram: o widget de ações tem o foco, o modo é
	 * GameAndUI, e a tecla morria antes. Agora quem escuta é um ouvinte de
	 * pré-input do Slate, instalado aqui e removido no EndPlay — ele vê a
	 * tecla antes de qualquer camada poder engoli-la.
	 *
	 * Por tecla e não só por console: o painel é DESENHADO, então o mouse
	 * nunca consegue selecioná-lo, e abrir o console no meio de uma partida é
	 * atrito suficiente para a informação não ser capturada.
	 */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	ABattleSquarePlayerController();

	UFUNCTION(Server, Reliable)
	void Server_CreateRoom();

	UFUNCTION(Server, Reliable)
	void Server_JoinRoom(const FString& Code);

	UFUNCTION(Server, Reliable)
	void Server_ReconnectToRoom(const FString& Code, const FGuid& Secret);

	// Preenchido pelo servidor ao criar/entrar com sucesso — de quem é
	// este controller é responsabilidade do GameMode saber ao chamar
	// Logout (T7, MarkDisconnected).
	UPROPERTY()
	FString CurrentRoomCode;

	UPROPERTY()
	uint8 CurrentSide = 0;

	UPROPERTY()
	FGuid ReconnectSecret;

	// Canal de commit deste jogador — criado no construtor, conectado ao
	// UBattleTurnCoordinator da sala quando a partida é montada (T8).
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBattleNetCommitComponent> NetCommitComponent;
};
