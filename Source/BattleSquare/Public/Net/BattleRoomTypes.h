// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BattleRoomTypes.generated.h"

// T1 (tasks.md, Sala e Pareamento Simples, SALA-01/02): código curto de
// sala e estado por ocupante. Formato/tamanho como constantes nomeadas
// — mesmo padrão de BattleNetConstants.h.
namespace BattleRoomConstants
{
	constexpr int32 RoomCodeLength = 5;

	// Alfabeto sem caracteres ambíguos ao digitar/ler em voz alta:
	// exclui 0/O e 1/I (spec.md, SALA-01, critério 1).
	inline const FString RoomCodeAlphabet = TEXT("23456789ABCDEFGHJKLMNPQRSTUVWXYZ");
}

UENUM()
enum class EBattleRoomJoinResult : uint8
{
	Success = 0,
	NotFound,
	Full
};

// Estado de um lado dentro de uma sala — segredo de reconexão (DP-sala-04)
// e o momento da desconexão, se houver.
USTRUCT()
struct FBattleRoomOccupant
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Secret;

	UPROPERTY()
	bool bOccupied = false;

	UPROPERTY()
	bool bConnected = false;

	// Só tem significado quando bConnected é false.
	UPROPERTY()
	double DisconnectedAtSeconds = 0.0;
};

// Estado completo de uma sala. Side0 é sempre quem criou (DP-sala-03).
USTRUCT()
struct FBattleRoomState
{
	GENERATED_BODY()

	UPROPERTY()
	FBattleRoomOccupant Side0;

	UPROPERTY()
	FBattleRoomOccupant Side1;

	UPROPERTY()
	bool bAbandonmentDeclared = false;
};

// T1: gera um código candidato — não garante unicidade sozinho, quem
// chama (UBattleRoomRegistry) checa contra as salas vivas.
BATTLESQUARE_API FString GenerateCandidateRoomCode();
