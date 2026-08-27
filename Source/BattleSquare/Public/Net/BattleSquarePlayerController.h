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
	 * F7 alterna o controle dos dois lados; F9 copia o painel; F10 o esvazia.
	 *
	 * CONVENIÊNCIA, não o caminho principal: as teclas falharam três vezes em
	 * PIE e eu não consegui provar por quê. Quem faz estas ações de forma
	 * confiável é a BARRA DE BOTÕES (FBattleDebugToolbar) — clique é o que
	 * comprovadamente funciona aqui.
	 */
	virtual void SetupInputComponent() override;

private:
	void ToggleControllingBothSides();
	void CopyBattleDebugPanel();
	void ClearBattleDebugPanel();

public:
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
