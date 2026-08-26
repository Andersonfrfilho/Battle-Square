// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleRoomRegistry.h"
#include "Net/BattleNetConstants.h"

namespace
{
	bool HasSideAbandonedByTimeout(const FBattleRoomOccupant& Occupant, double CurrentTimeSeconds)
	{
		if (!Occupant.bOccupied || Occupant.bConnected)
		{
			return false;
		}
		const double Elapsed = CurrentTimeSeconds - Occupant.DisconnectedAtSeconds;
		return Elapsed >= static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds);
	}
}

FString UBattleRoomRegistry::CreateRoom(FGuid& OutCreatorSecret)
{
	FString Code;
	do
	{
		Code = GenerateCandidateRoomCode();
	} while (ActiveRooms.Contains(Code));

	FBattleRoomState NewRoom;
	OutCreatorSecret = FGuid::NewGuid();
	NewRoom.Side0.bOccupied = true;
	NewRoom.Side0.bConnected = true;
	NewRoom.Side0.Secret = OutCreatorSecret;

	ActiveRooms.Add(Code, NewRoom);
	return Code;
}

EBattleRoomJoinResult UBattleRoomRegistry::JoinRoom(const FString& Code, FGuid& OutJoinerSecret)
{
	FBattleRoomState* Room = ActiveRooms.Find(Code);
	if (!Room)
	{
		return EBattleRoomJoinResult::NotFound;
	}

	if (Room->Side1.bOccupied)
	{
		return EBattleRoomJoinResult::Full;
	}

	OutJoinerSecret = FGuid::NewGuid();
	Room->Side1.bOccupied = true;
	Room->Side1.bConnected = true;
	Room->Side1.Secret = OutJoinerSecret;

	OnRoomReady.Broadcast(Code);

	return EBattleRoomJoinResult::Success;
}

void UBattleRoomRegistry::MarkDisconnected(const FString& Code, uint8 Side, double CurrentTimeSeconds)
{
	FBattleRoomState* Room = ActiveRooms.Find(Code);
	if (!Room)
	{
		return;
	}

	FBattleRoomOccupant& Occupant = (Side == 0) ? Room->Side0 : Room->Side1;
	if (!Occupant.bOccupied)
	{
		return;
	}

	Occupant.bConnected = false;
	Occupant.DisconnectedAtSeconds = CurrentTimeSeconds;
}

bool UBattleRoomRegistry::TryReconnect(const FString& Code, const FGuid& Secret)
{
	FBattleRoomState* Room = ActiveRooms.Find(Code);
	if (!Room)
	{
		return false;
	}

	for (FBattleRoomOccupant* Occupant : { &Room->Side0, &Room->Side1 })
	{
		if (Occupant->bOccupied && !Occupant->bConnected && Occupant->Secret == Secret)
		{
			Occupant->bConnected = true;
			return true;
		}
	}

	return false;
}

void UBattleRoomRegistry::CheckAbandonment(double CurrentTimeSeconds)
{
	for (TPair<FString, FBattleRoomState>& Pair : ActiveRooms)
	{
		FBattleRoomState& Room = Pair.Value;
		if (Room.bAbandonmentDeclared)
		{
			continue;
		}

		const bool bSide0Abandoned = HasSideAbandonedByTimeout(Room.Side0, CurrentTimeSeconds);
		const bool bSide1Abandoned = HasSideAbandonedByTimeout(Room.Side1, CurrentTimeSeconds);

		// Só declara vencedor quando exatamente um lado abandonou e o
		// outro está presente (ocupado e conectado). Os dois abandonando
		// juntos não tem "lado presente" — vira caso de sala vazia (T6).
		if (bSide0Abandoned && !bSide1Abandoned && Room.Side1.bOccupied && Room.Side1.bConnected)
		{
			Room.bAbandonmentDeclared = true;
			OnRoomAbandoned.Broadcast(Pair.Key, /*PresentSide=*/1);
		}
		else if (bSide1Abandoned && !bSide0Abandoned && Room.Side0.bOccupied && Room.Side0.bConnected)
		{
			Room.bAbandonmentDeclared = true;
			OnRoomAbandoned.Broadcast(Pair.Key, /*PresentSide=*/0);
		}
	}
}

void UBattleRoomRegistry::CheckEmptyRooms(double CurrentTimeSeconds)
{
	TArray<FString> RoomsToRemove;

	for (const TPair<FString, FBattleRoomState>& Pair : ActiveRooms)
	{
		const FBattleRoomState& Room = Pair.Value;
		if (!Room.Side1.bOccupied)
		{
			// Sala com só o criador nunca é considerada "abandonada por
			// completo" (SALA-11 fala em "os dois jogadores saíram") —
			// não existe segundo jogador para ter saído.
			continue;
		}

		if (HasSideAbandonedByTimeout(Room.Side0, CurrentTimeSeconds) && HasSideAbandonedByTimeout(Room.Side1, CurrentTimeSeconds))
		{
			RoomsToRemove.Add(Pair.Key);
		}
	}

	for (const FString& Code : RoomsToRemove)
	{
		ActiveRooms.Remove(Code);
	}
}

bool UBattleRoomRegistry::IsRoomFull(const FString& Code) const
{
	const FBattleRoomState* Room = ActiveRooms.Find(Code);
	return Room && Room->Side1.bOccupied;
}
