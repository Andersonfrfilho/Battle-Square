// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleTypes.h"
#include "BattleNetTypes.generated.h"

// T1–T3 (tasks.md, Combate Online): tipo de fio do commit de rede.
//
// Por que isto existe em vez de mandar FTurnCommit direto num RPC — ver
// design.md, "A decisão técnica que a investigação mudou": FTurnCommit
// tem um array C estático (FBattleAction Actions[3]), e não foi possível
// provar que FRepLayout expande esse array interno em vez de serializar
// só o elemento 0 — o mesmo tipo de array que já impediu BuildCommit()
// de ser exposto a Blueprint (UHT rejeita array estático nesse contexto).
// O modo de falha alternativo é silencioso: o jogo compilaria, conectaria
// e resolveria turnos, só que as ações 2 e 3 do oponente não existiriam.
// Três campos nomeados eliminam a dúvida inteira.
//
// ⚠️ N PETS NÃO REABRE ESSA DÚVIDA, e a escolha é deliberada: vários pets viajam
// como `TArray<FNetTurnCommit>`, e não como um array C maior. `TArray` de
// USTRUCT é justamente o caso que `FRepLayout` sabe expandir — o array C
// interno continua com três campos nomeados, e o crescimento acontece por fora.
//
// Reencontrar o mesmo modo de falhar, agora com o PET 2 em vez da AÇÃO 2, seria
// pagar duas vezes pela mesma investigação.
USTRUCT()
struct FNetTurnCommit
{
	GENERATED_BODY()

	/**
	 * DE QUEM sao estas acoes, por `PetId`.
	 *
	 * O fio carregava um commit por LADO, e o lado deixou de identificar
	 * alguem quando um lado passou a ter dois pets. O id acompanha o commit
	 * pelo fio porque o servidor precisa saber a quem aplicar cada um — deduzir
	 * pela ordem faria a acao trocar de dono se um pacote chegasse fora de
	 * ordem, e o sintoma seria "o pet errado atacou".
	 *
	 * Sentinela 0xFF: commit que ninguem enderecou nao pode valer para o pet 0.
	 */
	UPROPERTY()
	uint8 PetId = 0xFF;

	UPROPERTY()
	FBattleAction ActionA;

	UPROPERTY()
	FBattleAction ActionB;

	UPROPERTY()
	FBattleAction ActionC;
};

// T2: validação de forma — rejeita enum fora de range vindo de um cliente
// adversarial. Não valida REGRA de jogo (ex.: atacar sem alvo é uma
// combinação válida, só sem efeito) — só se os bytes recebidos mapeiam
// para valores de enum que existem.
BATTLESQUARE_API bool ValidateNetTurnCommit(const FNetTurnCommit& Commit);

// T3: conversão entre o tipo de fio e o tipo que o núcleo consome. Pura,
// sem efeito colateral — quem monta/consome o FTurnCommit continua sendo
// código já existente (BuildCommit, FBattleResolver::ResolveTurn).
BATTLESQUARE_API FTurnCommit ToTurnCommit(const FNetTurnCommit& NetCommit);
BATTLESQUARE_API FNetTurnCommit ToNetTurnCommit(const FTurnCommit& Commit);
