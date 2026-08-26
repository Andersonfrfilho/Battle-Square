// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleScreenGameMode.h"
#include "UI/BattleActionSelectorWidget.h"
#include "Battle/BattleArena.h"
#include "Battle/BattleActionQueueComponent.h"
#include "Data/PetDataLoader.h"
#include "World/EncounterMatchAssembler.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

ABattleScreenGameMode::ABattleScreenGameMode()
{
	// A tela de batalha não tem mundo para percorrer: nenhum pawn de
	// exploração precisa nascer aqui.
	PrimaryActorTick.bCanEverTick = true;
}

void ABattleScreenGameMode::BeginPlay()
{
	Super::BeginPlay();

	const FString Problem = StartScreenBattle();
	if (!Problem.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABattleScreenGameMode: não abriu a batalha — %s"), *Problem);
	}
}

FString ABattleScreenGameMode::StartScreenBattle()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return TEXT("sem UWorld");
	}

	TArray<FLoadedPetRecord> Pets;
	if (const FString MirrorProblem = LoadConfiguredMirrorPets(Pets); !MirrorProblem.IsEmpty())
	{
		return MirrorProblem;
	}
	if (Pets.Num() < 2)
	{
		return TEXT("o espelho tem menos de dois pets — não dá para montar uma batalha");
	}

	// Ids vazios são conveniência de desenvolvimento, não configuração
	// silenciosa: o padrão é explícito e está documentado no cabeçalho.
	const FString ResolvedPlayerId = PlayerCatalogId.IsEmpty() ? Pets[0].Id : PlayerCatalogId;
	FString ResolvedOpponentId = OpponentCatalogId;
	if (ResolvedOpponentId.IsEmpty())
	{
		for (const FLoadedPetRecord& Pet : Pets)
		{
			if (Pet.Id != ResolvedPlayerId)
			{
				ResolvedOpponentId = Pet.Id;
				break;
			}
		}
	}

	FEncounterMatchParams MatchParams;
	MatchParams.AvailablePets = Pets;
	MatchParams.PetCollectionSlotName = PetCollectionSlotName;
	MatchParams.PlayerCatalogId = ResolvedPlayerId;
	MatchParams.EncounterCatalogId = ResolvedOpponentId;

	FBattleState InitialState;
	TArray<FPetPresentationInfo> Presentations;
	if (!FEncounterMatchAssembler::AssembleFromEncounter(MatchParams, InitialState, Presentations))
	{
		return FString::Printf(TEXT("montagem recusada (jogador='%s', oponente='%s')"),
			*ResolvedPlayerId, *ResolvedOpponentId);
	}

	ScreenArena = World->SpawnActor<ABattleArena>(ABattleArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (!ScreenArena)
	{
		return TEXT("a arena não spawnou");
	}
	if (!ScreenArena->BeginBattle(InitialState, Presentations))
	{
		return TEXT("BeginBattle recusou a montagem");
	}

	// Sem classe de widget configurada a batalha ainda roda — só fica sem
	// interface. Degrada, não quebra (mesmo princípio de DP-trav-06).
	if (!ActionSelectorWidgetClassPath.IsValid())
	{
		return TEXT("ActionSelectorWidgetClassPath não configurado — batalha montada, mas sem interface");
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return TEXT("sem PlayerController para receber a interface");
	}

	// A visão precisa ir para a arena, senão a batalha acontece e o jogador
	// olha para o cenário vazio — exatamente o que aconteceu na transição do
	// mundo aberto, e que só apareceu ao rodar de verdade.
	PlayerController->SetViewTarget(ScreenArena);

	UClass* WidgetClass = ActionSelectorWidgetClassPath.TryLoadClass<UBattleActionSelectorWidget>();
	if (!WidgetClass)
	{
		return FString::Printf(TEXT("classe de widget não carregou (%s)"), *ActionSelectorWidgetClassPath.ToString());
	}

	ActionSelector = CreateWidget<UBattleActionSelectorWidget>(PlayerController, WidgetClass);
	if (!ActionSelector)
	{
		return TEXT("CreateWidget devolveu nulo");
	}

	ActionSelector->BindToQueue(ScreenArena->PlayerActionQueue);
	ActionSelector->AddToViewport();

	// A tela é o jogo agora: o mouse precisa clicar nos botões.
	PlayerController->bShowMouseCursor = true;
	PlayerController->SetInputMode(FInputModeGameAndUI().SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));

	return FString();
}
