// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleSquarePlayerController.h"
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
