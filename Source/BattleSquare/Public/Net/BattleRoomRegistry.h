// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/BattleRoomTypes.h"
#include "BattleRoomRegistry.generated.h"

// T2–T6 (tasks.md, Sala e Pareamento Simples): lógica pura de sala —
// código, ocupação, desconexão/reconexão, abandono. Sem AActor, sem
// UWorld, tempo sempre INJETADO — mesmo padrão de UBattleTurnCoordinator
// (Combate Online). ABattleSquareGameMode (T7) é quem liga isto a
// jogadores/RPCs reais; este objeto não sabe que rede existe.
UCLASS()
class BATTLESQUARE_API UBattleRoomRegistry : public UObject
{
	GENERATED_BODY()

public:
	// T2: cria uma sala com Side0 ocupado. Retorna o código; preenche
	// OutCreatorSecret com o segredo de reconexão do criador (DP-sala-04).
	// Garante que o código não colide com nenhuma sala VIVA.
	FString CreateRoom(FGuid& OutCreatorSecret);

	// T3: entra numa sala com 1 vaga. NotFound e Full são distintos —
	// nunca um terceiro lado é aceito.
	EBattleRoomJoinResult JoinRoom(const FString& Code, FGuid& OutJoinerSecret);

	// T4: marca um lado como desconectado, tempo injetado.
	void MarkDisconnected(const FString& Code, uint8 Side, double CurrentTimeSeconds);

	// T4: reconecta se Code+Secret bate com um lado desconectado daquela
	// sala. Retorna false sem alterar nada em qualquer outro caso.
	bool TryReconnect(const FString& Code, const FGuid& Secret);

	// T5 🧠: para cada sala com um lado desconectado há mais que
	// BattleNetConstants::AbandonTimeoutSeconds, dispara OnRoomAbandoned
	// exatamente uma vez (bAbandonmentDeclared evita repetição).
	void CheckAbandonment(double CurrentTimeSeconds);

	// T6: remove salas com os dois lados desconectados além do timeout —
	// o código fica livre para reuso.
	void CheckEmptyRooms(double CurrentTimeSeconds);

	bool DoesRoomExist(const FString& Code) const { return ActiveRooms.Contains(Code); }
	bool IsRoomFull(const FString& Code) const;

	// T8: sala cheia (SALA-05/06). Quem ouve consulta o estado por
	// GetRoomState e monta a partida.
	DECLARE_MULTICAST_DELEGATE_OneParam(FRoomReadySignature, const FString& /*Code*/);
	FRoomReadySignature OnRoomReady;

	// T9: abandono real de uma sala. PresentSide é quem vence.
	DECLARE_MULTICAST_DELEGATE_TwoParams(FRoomAbandonedSignature, const FString& /*Code*/, uint8 /*PresentSide*/);
	FRoomAbandonedSignature OnRoomAbandoned;

	const FBattleRoomState* GetRoomState(const FString& Code) const { return ActiveRooms.Find(Code); }

private:
	TMap<FString, FBattleRoomState> ActiveRooms;
};
