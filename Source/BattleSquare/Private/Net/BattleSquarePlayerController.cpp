// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleSquarePlayerController.h"
#include "Debug/BattleDebugScreen.h"
#include "Battle/BattleArena.h"
#include "EngineUtils.h"
#include "Components/InputComponent.h"
#include "Net/BattleSquareGameMode.h"

ABattleSquarePlayerController::ABattleSquarePlayerController()
{
	NetCommitComponent = CreateDefaultSubobject<UBattleNetCommitComponent>(TEXT("NetCommitComponent"));
}

void ABattleSquarePlayerController::Server_CreateRoom_Implementation()
{
	ABattleSquareGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ABattleSquareGameMode>() : nullptr;
	if (!GameMode || !GameMode->RoomRegistry)
	{
		return;
	}

	FGuid Secret;
	const FString Code = GameMode->RoomRegistry->CreateRoom(Secret);

	CurrentRoomCode = Code;
	CurrentSide = 0;
	ReconnectSecret = Secret;
	GameMode->RegisterControllerForRoom(Code, /*Side=*/0, this);
}

void ABattleSquarePlayerController::Server_JoinRoom_Implementation(const FString& Code)
{
	ABattleSquareGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ABattleSquareGameMode>() : nullptr;
	if (!GameMode || !GameMode->RoomRegistry)
	{
		return;
	}

	FGuid Secret;
	const EBattleRoomJoinResult Result = GameMode->RoomRegistry->JoinRoom(Code, Secret);
	if (Result != EBattleRoomJoinResult::Success)
	{
		return;
	}

	CurrentRoomCode = Code;
	CurrentSide = 1;
	ReconnectSecret = Secret;
	GameMode->RegisterControllerForRoom(Code, /*Side=*/1, this);
}

void ABattleSquarePlayerController::Server_ReconnectToRoom_Implementation(const FString& Code, const FGuid& Secret)
{
	ABattleSquareGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ABattleSquareGameMode>() : nullptr;
	if (!GameMode || !GameMode->RoomRegistry)
	{
		return;
	}

	if (!GameMode->RoomRegistry->TryReconnect(Code, Secret))
	{
		return;
	}

	const FBattleRoomState* Room = GameMode->RoomRegistry->GetRoomState(Code);
	if (!Room)
	{
		return;
	}

	const uint8 Side = (Room->Side0.Secret == Secret) ? 0 : 1;
	CurrentRoomCode = Code;
	CurrentSide = Side;
	ReconnectSecret = Secret;
	GameMode->RegisterControllerForRoom(Code, Side, this);
}

void ABattleSquarePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	// bConsumeInput=false: teclas de depuração não podem engolir input do jogo.
	InputComponent->BindKey(EKeys::F8, IE_Pressed, this,
		&ABattleSquarePlayerController::ToggleControllingBothSides).bConsumeInput = false;
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this,
		&ABattleSquarePlayerController::CopyBattleDebugPanel).bConsumeInput = false;
	InputComponent->BindKey(EKeys::F10, IE_Pressed, this,
		&ABattleSquarePlayerController::ClearBattleDebugPanel).bConsumeInput = false;
}

void ABattleSquarePlayerController::CopyBattleDebugPanel()
{
	FBattleDebugScreen::CopyToClipboard();
}

void ABattleSquarePlayerController::ClearBattleDebugPanel()
{
	FBattleDebugScreen::Clear();
}

void ABattleSquarePlayerController::ToggleControllingBothSides()
{
#if !UE_BUILD_SHIPPING
	// Por tecla, e não só por console, pelo mesmo motivo do F9: abrir o
	// console no meio de uma partida é atrito suficiente para a verificação
	// simplesmente não acontecer.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ABattleArena> It(World); It; ++It)
	{
		It->SetControllingBothSides(!It->IsControllingBothSides());
	}
#endif
}
