// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "World/EncounterMatchAssembler.h"
#include "WorldEncounterFlow.generated.h"

class AActor;
class ABattleArena;
class AWorldEncounterActor;
class UEncounterDetectionComponent;
class UWorldBattleTransitionService;

/**
 * Fiação de ponta a ponta (T5): detecção → montagem → transição → batalha.
 * Não duplica nenhum passo — cada peça continua sendo a que já existe e já
 * é testada isoladamente.
 */
UCLASS()
class BATTLESQUARE_API UWorldEncounterFlow : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AActor* InWorldPawn,
		UEncounterDetectionComponent* InDetection,
		TSubclassOf<ABattleArena> InArenaClass,
		const FEncounterMatchParams& InMatchParams);

	ABattleArena* HandleEncounterTriggered(AWorldEncounterActor* Encounter);

	/**
	 * Avisa que uma batalha COMEÇOU no mundo, com a arena já montada.
	 *
	 * O fluxo não cria interface: quem sabe qual widget usar é o GameMode.
	 * Sem este aviso a batalha do mundo abria SEM botões de ação — o jogador
	 * caía na luta e não tinha como jogar, que é pior que não ter a transição.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FWorldBattleStartedSignature, ABattleArena*);
	FWorldBattleStartedSignature OnWorldBattleStarted;

	UWorldBattleTransitionService* GetTransitionService() const { return TransitionService; }

private:
	void HandleEncounterTriggeredInternal(AWorldEncounterActor* Encounter) { HandleEncounterTriggered(Encounter); }

	UPROPERTY()
	TObjectPtr<AActor> WorldPawn;

	UPROPERTY()
	TObjectPtr<UEncounterDetectionComponent> Detection;

	UPROPERTY()
	TObjectPtr<UWorldBattleTransitionService> TransitionService;

	TSubclassOf<ABattleArena> ArenaClass;

	FEncounterMatchParams MatchParams;
};
