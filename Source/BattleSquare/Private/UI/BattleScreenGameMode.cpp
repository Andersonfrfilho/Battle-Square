// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleScreenGameMode.h"
#include "Net/BattleSquarePlayerController.h"
#include "Debug/BattleDebugHUD.h"
#include "Debug/BattleDebugScreen.h"
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
	// As teclas de depuração (F8/F9/F10) vivem neste controlador. Sem
	// declará-lo aqui, a tela de batalha caía no APlayerController padrão e
	// elas não existiam — na tela onde o jogador passa o tempo todo.
	PlayerControllerClass = ABattleSquarePlayerController::StaticClass();

	// A tela de batalha não tem mundo para percorrer: nenhum pawn de
	// exploração precisa nascer aqui.
	PrimaryActorTick.bCanEverTick = true;
	// O pai tica 1x/s (checagem de sala). Aqui a casa do pet precisa
	// acompanhar o turno, e um segundo de atraso já aparece na tela.
	PrimaryActorTick.TickInterval = 0.0f;

	// Painel de depuração em tela (CLAUDE.md): o jogador é o instrumento de
	// medição mais eficiente deste projeto, e ler da tela é mais barato que
	// abrir o Log de Saída.
	HUDClass = ABattleDebugHUD::StaticClass();
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

	// A tela precisa saber onde o pet do jogador está para não oferecer um
	// movimento que sairia do tabuleiro. Quem sabe isso é a arena.
	for (const FPetState& Pet : ScreenArena->GetCurrentState().Pets)
	{
		if (Pet.Side == ScreenArena->LocalPlayerSide)
		{
			ActionSelector->SetOwningPetCell(Pet.Column, Pet.Row);
			break;
		}
	}

	ActionSelector->AddToViewport();

	// DEPOIS de entrar na viewport, senão não há Slate widget para focar.
	ActionSelector->SetKeyboardFocus();

	// Tecla que ninguém sabe que existe não existe. Isto fica FIXO no painel
	// (Key própria, sem empilhar) porque o usuário apertou F9 sem retorno e
	// procurou por botões de controlar o oponente que, por desenho, não
	// existem — no modo dos dois lados usam-se os MESMOS botões.
	FBattleDebugScreen::Show(
		TEXT("F7 escolhe pelos DOIS lados (mesmos botões, sua vez e a dele)"),
		0.0f, FColor::Orange, /*Key=*/10);
	FBattleDebugScreen::Show(
		TEXT("C camufla | V voa | B submerge  (sem botão no widget ainda)"),
		0.0f, FColor::Orange, /*Key=*/11);
	FBattleDebugScreen::Show(
		TEXT("F9 copia este painel e grava Saved/BattleDebug.txt | F10 limpa"),
		0.0f, FColor::Silver, /*Key=*/12);

	// A tela é o jogo agora: o mouse precisa clicar nos botões.
	PlayerController->bShowMouseCursor = true;
	PlayerController->SetInputMode(FInputModeGameAndUI().SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));

	return FString();
}

void ABattleScreenGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ActionSelector || !ScreenArena)
	{
		return;
	}

	// A casa do pet muda a cada turno, e a tela usa isso para não oferecer
	// um movimento que sairia do tabuleiro. Informar só uma vez, na abertura
	// da batalha, deixava a tela presa na casa inicial: quem andasse até um
	// canto continuava vendo as oito direções habilitadas.
	//
	// Lado QUE ESTÁ ESCOLHENDO, não o do jogador: com bs.ControlOpponent a
	// segunda escolha é pelo oponente, e oferecer as direções do pet errado
	// deixaria escolher um movimento impossível.
	const uint8 LadoEscolhendo = ScreenArena->GetSideBeingChosen();
	const bool bEhOponente = (LadoEscolhendo != ScreenArena->LocalPlayerSide);

	// O NOME do pet, não "lado 1": quem joga reconhece o bicho, não o índice.
	FString NomeDoPet;
	for (const FPetState& Pet : ScreenArena->GetCurrentState().Pets)
	{
		if (Pet.Side == LadoEscolhendo)
		{
			NomeDoPet = ScreenArena->GetPresentationNameForPet(Pet.PetId);
			break;
		}
	}
	if (NomeDoPet.IsEmpty())
	{
		NomeDoPet = bEhOponente ? TEXT("oponente") : TEXT("seu pet");
	}

	ActionSelector->SetChoosingForLabel(FText::FromString(
		bEhOponente
			? FString::Printf(TEXT("▶ escolhendo pelo OPONENTE: %s"), *NomeDoPet)
			: FString::Printf(TEXT("▶ escolhendo por VOCÊ: %s"), *NomeDoPet)));

	// Também no painel, com Key fixa: a linha se atualiza no lugar em vez de
	// empilhar, e sobrevive a olhar para outro canto da tela.
	FBattleDebugScreen::Show(
		FString::Printf(TEXT("controlando: %s (%s)"),
			*NomeDoPet, bEhOponente ? TEXT("OPONENTE") : TEXT("você")),
		0.0f, bEhOponente ? FColor::Orange : FColor::Cyan, /*Key=*/13);
	for (const FPetState& Pet : ScreenArena->GetCurrentState().Pets)
	{
		if (Pet.Side == LadoEscolhendo)
		{
			ActionSelector->SetOwningPetCell(Pet.Column, Pet.Row);
			return;
		}
	}
}
