// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "BattleTurnCoordinator.generated.h"

// T4–T6 (tasks.md, Combate Online): lado SERVIDOR do commit às cegas.
// Acumula o commit de cada lado sem revelar nada a ninguém (AD-005,
// BTL-02), e só chama o resolvedor real (FBattleResolver::ResolveTurn,
// BattleSim — inalterado) quando os dois estão presentes ou o timeout
// estoura.
//
// REGRA INQUEBRÁVEL (design.md, "O erro que esta arquitetura existe
// para não cometer"): nenhum campo de commit aqui é UPROPERTY(Replicated)
// nem aparece em GetLifetimeReplicatedProps. Replicar o commit antes da
// resolução entregaria a fila de um jogador ao outro sem produzir erro
// nenhum — é auditado por Tools/audit_no_commit_replication.sh (T10).
//
// Tempo é sempre INJETADO pelo chamador (nunca FPlatformTime nem
// GetWorld()->GetTimeSeconds() lido daqui) — é o que permite testar a
// lógica de timeout e de corrida (T6) sem depender de tempo real.
DECLARE_MULTICAST_DELEGATE_TwoParams(FBattleTurnResolvedSignature, const FBattleState& /*NextState*/, const TArray<FBattleEvent>& /*Trace*/);

// T13 (NET-11): dispara quando um lado é declarado abandonado. Carrega
// o mesmo FBattleEvent BatalhaEncerrada que o núcleo já usa — nenhum
// tipo de evento novo, mesmo vocabulário de sempre (BTL-22).
DECLARE_MULTICAST_DELEGATE_OneParam(FBattleAbandonmentSignature, const FBattleEvent& /*BatalhaEncerradaEvent*/);

UCLASS()
class BATTLESQUARE_API UBattleTurnCoordinator : public UObject
{
	GENERATED_BODY()

public:
	FBattleTurnResolvedSignature OnTurnResolved;

	// Inicia um novo turno: guarda o estado de partida e a hora de início
	// (injetada). Limpa qualquer commit pendente do turno anterior.
	void BeginTurn(const FBattleState& State, double StartTimeSeconds);

	// Registra o commit de um lado. Retorna false se: o turno já
	// resolveu, ou este lado já tinha commitado neste turno (idempotência
	// — nunca sobrescreve). Se os dois lados estiverem presentes depois
	// desta chamada, resolve IMEDIATAMENTE, sem esperar o próximo
	// CheckTimeout — é o que garante que um commit que chega antes do
	// timeout nunca perde para ele (T6).
	bool SubmitCommit(uint8 Side, const FTurnCommit& Commit);

	// Chamado periodicamente pelo dono (ex.: Tick do servidor) com o
	// tempo atual injetado. Se o turno ainda não resolveu e o tempo
	// decorrido desde BeginTurn passou de BattleNetConstants::
	// CommitTimeoutSeconds, preenche o(s) lado(s) ausente(s) com 3x
	// Aguardar e resolve.
	void CheckTimeout(double CurrentTimeSeconds);

	bool IsTurnResolved() const { return bResolved; }

	// T13 (NET-07, DP-online-03): estado para um jogador que reconecta —
	// entrega o FBattleState atual direto, NUNCA replay do trace
	// acumulado. Quem chama isto (fora de escopo desta feature: sessão/
	// GameMode) usa o resultado para reconstruir a visão local via
	// ABattleArena::BeginBattle (SpawnPetViews + SetInitialState, já
	// existentes) — mesmo caminho que já existe, sem componente novo.
	const FBattleState& GetCurrentBattleState() const { return CurrentState; }

	FBattleAbandonmentSignature OnAbandonment;

	// T13 (NET-11): declara abandono do lado ausente — jogador presente
	// vence. Reaproveita o vocabulário existente do trace (BatalhaEncerrada,
	// Value = lado vencedor), nunca inventa um evento novo.
	void DeclareAbandonment(uint8 PresentSide);

	static FBattleEvent MakeAbandonmentEvent(uint8 PresentSide);

private:
	void TryResolveIfBothPresent();
	void ResolveWithCommits(const FTurnCommit& CommitSide0, const FTurnCommit& CommitSide1);
	static FTurnCommit MakeWaitOnlyCommit();

	FBattleState CurrentState;

	FTurnCommit PendingCommitSide0;
	bool bHasCommitSide0 = false;

	FTurnCommit PendingCommitSide1;
	bool bHasCommitSide1 = false;

	double TurnStartTimeSeconds = 0.0;
	bool bResolved = false;
};
