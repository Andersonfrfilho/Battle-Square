// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/BattleNetTypes.h"
#include "Net/BattleTurnCoordinator.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "BattleNetCommitComponent.generated.h"

// T7 (tasks.md, Combate Online, NET-01/NET-04): lado CLIENTE do commit —
// envia o FTurnCommit local (já montado por UBattleActionQueueComponent,
// existente) como FNetTurnCommit via RPC, e repassa o resultado recebido
// do servidor para quem consome (ABattleArena → UBattleTracePlayer, sem
// mudança nenhuma nesse consumidor).
//
// Limite de Ferramenta (design.md): a lógica de validação e
// encaminhamento (ValidateIncomingCommit/ProcessValidatedCommit) é
// testável headless, sem rede real — é código puro. O DISPACHO do RPC
// em si (Server_SubmitCommit atravessando uma UNetConnection de
// verdade) não é — isso é o que T11/T12 investigam.
DECLARE_MULTICAST_DELEGATE_TwoParams(FBattleNetTurnResultReceivedSignature, const FBattleState& /*NextState*/, const TArray<FBattleEvent>& /*Trace*/);

UCLASS(ClassGroup = (BattleSquare), meta = (BlueprintSpawnableComponent))
class BATTLESQUARE_API UBattleNetCommitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBattleNetCommitComponent();

	// Só o SERVIDOR chama isto — associa este componente ao lado (0 ou 1)
	// e ao coordenador que vai receber o commit validado.
	void SetServerCoordinator(UBattleTurnCoordinator* InCoordinator, uint8 InSide);

	// Cliente: envia o commit local (já montado por
	// UBattleActionQueueComponent::BuildCommit) para o servidor.
	void SubmitLocalCommit(const FTurnCommit& Commit);

	// Testável headless, sem RPC: valida a FORMA do payload (T2).
	bool ValidateIncomingCommit(const FNetTurnCommit& Commit) const;

	// Testável headless, sem RPC: converte e repassa ao coordenador do
	// servidor. Único ponto que toca UBattleTurnCoordinator::SubmitCommit
	// a partir da rede.
	void ProcessValidatedCommit(const FNetTurnCommit& Commit);

	// Cliente: disparado quando o resultado (estado + trace) chega do
	// servidor. ABattleArena assina isto para alimentar UBattleTracePlayer.
	FBattleNetTurnResultReceivedSignature OnResultReceived;

	// Testável headless, sem RPC real: a lógica de "resultado chegou,
	// expor via delegate". Pública de propósito — é a peça que T7 pede
	// para ser verificável sem uma UNetConnection de verdade (ver
	// design.md, Limite de Ferramenta). Não é chamada diretamente em
	// produção: o RPC Client_ReceiveTurnResult é o caminho real.
	void Client_ReceiveTurnResult_Implementation(FBattleState NextState, const TArray<FBattleEvent>& Trace);

private:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SubmitCommit(FNetTurnCommit Commit);
	bool Server_SubmitCommit_Validate(FNetTurnCommit Commit);
	void Server_SubmitCommit_Implementation(FNetTurnCommit Commit);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveTurnResult(FBattleState NextState, const TArray<FBattleEvent>& Trace);

	// Ponte entre o delegate (const-ref nos dois parâmetros) do
	// coordenador e a assinatura do RPC Client (que exige FBattleState
	// por valor, convenção de parâmetro de RPC da Unreal).
	void HandleCoordinatorResolved(const FBattleState& NextState, const TArray<FBattleEvent>& Trace);

	UPROPERTY()
	TObjectPtr<UBattleTurnCoordinator> Coordinator;

	uint8 Side = 0;
};
