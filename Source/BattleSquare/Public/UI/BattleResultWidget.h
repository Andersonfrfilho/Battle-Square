// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Battle/BattleEvent.h"
#include "BattleResultWidget.generated.h"

// T12 (tasks.md, PRES-14): resultado exposto a Blueprint, lido a partir
// do evento BatalhaEncerrada — nunca recalculado. Value do evento é
// WinningSide (0, 1, ou 0xFF para empate — BattleOutcome.cpp); este
// widget só traduz isso para o ponto de vista do jogador local.
UENUM(BlueprintType)
enum class EBattleResultOutcome : uint8
{
	Nenhum = 0,
	Vitoria,
	Derrota,
	Empate
};

/**
 * Traduz o evento de fim de batalha para o PONTO DE VISTA do jogador local.
 *
 * Função livre, e não método do widget, porque há dois consumidores: a tela de
 * resultado e a mensagem que a arena escreve no feed. Eu tinha escrito esta
 * mesma escada de if dentro da arena — duas verdades sobre "quem venceu", e
 * cópias concordam até a primeira edição (L-032, L-033, e o defeito de
 * direção).
 */
BATTLESQUARE_API EBattleResultOutcome BattleOutcomeForLocalPlayer(
	const FBattleEvent& Event, uint8 LocalPlayerSide);

UCLASS()
class BATTLESQUARE_API UBattleResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Não exposto ao Blueprint: FBattleEvent não é BlueprintType (mesmo
	// racional de APetView::ApplyEvent). Quem chama isto é C++
	// (ABattleArena, T10), ligado ao delegate de UBattleTracePlayer.
	void ApplyBattleEndedEvent(const FBattleEvent& Event, uint8 LocalPlayerSide);

	UPROPERTY(BlueprintReadOnly, Category = "Battle|Result")
	EBattleResultOutcome Outcome = EBattleResultOutcome::Nenhum;
};
