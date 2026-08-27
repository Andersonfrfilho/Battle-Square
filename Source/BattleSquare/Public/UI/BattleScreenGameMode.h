// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/BattleSquareGameMode.h"
#include "BattleScreenGameMode.generated.h"

class ABattleArena;
class UBattleActionSelectorWidget;

/**
 * Abre uma batalha DIRETO, sem atravessar o mundo aberto (DP-ui-04).
 *
 * Herda de ABattleSquareGameMode para reusar a configuração do espelho de pets
 * e o helper de montagem — não para reusar salas, que aqui ficam ociosas.
 * Existe porque o combate está pronto desde M1 e o mundo ainda é placeholder:
 * esperar o mundo para poder jogar seria esperar pela parte errada.
 */
UCLASS(config = Game)
class BATTLESQUARE_API ABattleScreenGameMode : public ABattleSquareGameMode
{
	GENERATED_BODY()

public:
	ABattleScreenGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Vazio = o primeiro pet do espelho. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Tela de Batalha")
	FString PlayerCatalogId;

	/** Vazio = o primeiro pet do espelho DIFERENTE do jogador. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Tela de Batalha")
	FString OpponentCatalogId;

	UPROPERTY(config, EditDefaultsOnly, Category = "Tela de Batalha")
	FSoftClassPath ActionSelectorWidgetClassPath;

	/**
	 * Começar a batalha já escolhendo pelos DOIS lados.
	 *
	 * Existe porque a tecla falhou três vezes e o console é atrito: com isto
	 * em DefaultGame.ini a capacidade fica disponível sem depender de input
	 * nenhum. Ambiente de desenvolvimento, batalha local contra bot.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Depuração")
	bool bStartControllingBothSides = false;

	UPROPERTY()
	TObjectPtr<ABattleArena> ScreenArena;

	UPROPERTY()
	TObjectPtr<UBattleActionSelectorWidget> ActionSelector;

	/** Motivo em caso de falha, vazio em caso de sucesso — nunca falha calado. */
	FString StartScreenBattle();
};
