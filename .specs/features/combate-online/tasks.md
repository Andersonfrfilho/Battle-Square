# Combate Online — Tarefas

**Design:** `.specs/features/combate-online/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`, subpasta `Net/` nova. `BattleSim` não é tocado em nenhuma tarefa — `FBattleResolver::ResolveTurn` é consumido, nunca modificado.

**Nota de modelo:** sessão trocou para `sonnet` por instrução explícita do usuário antes desta fase (registrado — não repetir a pergunta). Fica valendo `sonnet` para todas as tarefas abaixo, com T6 marcada 🧠 pelo mesmo motivo já registrado em design.md (lógica de corrida é fácil de errar sutilmente).

---

## Plano de Execução

### Fase 1 — Tipo de fio e validação (sequencial)
> 🤖 Modelo: `sonnet`

```
T1 → T2 → T3
```

### Fase 2 — Coordenador de turno, servidor (sequencial, depende da Fase 1)
> 🤖 Modelo: `sonnet` — **T6 é 🧠**

```
T4 → T5 → T6
```

### Fase 3 — Componentes de rede e fiação (sequencial, depende da Fase 2)
> 🤖 Modelo: `sonnet`

```
T7 → T8 → T9
```

### Fase 4 — Sondas e investigação de teste real de rede (paralelo após Fase 3)
> 🤖 Modelo: `sonnet` para T10/T11 · `haiku` para T14 (roteiro, mecânico)

```
T9 ──┬→ T10 [P]
     ├→ T11 → T12 [P]
     └→ T13 [P]
T11/T12/T13 ──→ T14
```

---

## Tarefas

### T1: `FNetTurnCommit` — tipo de fio
**O quê:** struct com 3 campos nomeados (`ActionA`, `ActionB`, `ActionC`, cada um `FBattleAction`) — nunca um array C estático. `BattleNetConstants.h` com `CommitTimeoutSeconds = 45` e `AbandonTimeoutSeconds = 120`.
**Onde:** `Source/BattleSquare/Public/Net/BattleNetTypes.h`, `Source/BattleSquare/Public/Net/BattleNetConstants.h`
**Depende de:** nada
**Requisito:** design.md — "A decisão técnica que a investigação mudou"

**Pronto quando:**
- [ ] `FNetTurnCommit` não contém nenhum array C estático (grep confirma: nenhum `[3]` ou `ActionsPerTurn` no arquivo)
- [ ] Compila, `USTRUCT()` válido para uso em `UFUNCTION(Server, ...)`
- [ ] `BattleNetConstants.h` declara as duas constantes como `constexpr int32`, nomeadas — nunca número solto em outro arquivo

**Verificar:** build verde; grep manual confirmando ausência de array estático
**Commit:** `feat(battlesquare): tipo de fio do commit de rede`

---

### T2: Validação de `FNetTurnCommit`
**O quê:** `bool ValidateNetTurnCommit(const FNetTurnCommit& Commit)` — rejeita enum fora de range (proteção contra payload malformado de cliente adversarial).
**Onde:** mesmo arquivo de T1
**Depende de:** T1
**Requisito:** NET-(edge case) "commit malformado"

**Pronto quando:**
- [ ] Rejeita `EActionType`/`EBattleDirection` com valor fora do enum (cast de `uint8` arbitrário)
- [ ] Aceita qualquer combinação válida de tipo+direção, mesmo as que o núcleo trataria como sem efeito (ex.: `Atacar` sem alvo) — validação de rede não é validação de regra de jogo, só de forma
- [ ] Função pura, sem estado, sem I/O

**Verificar:** `Automation RunTests BattleSquare.Net.ValidateNetTurnCommit` — inclui um caso com enum fabricado fora de range (`static_cast<EActionType>(200)`)
**Commit:** `feat(battlesquare): validação de commit de rede recebido`

---

### T3: Conversão `FNetTurnCommit` ↔ `FTurnCommit`
**O quê:** `FTurnCommit ToTurnCommit(const FNetTurnCommit&)` e o inverso, `FNetTurnCommit ToNetTurnCommit(const FTurnCommit&)`.
**Onde:** mesmo arquivo de T1
**Depende de:** T1
**Requisito:** design.md — "elimina a incerteza inteira"

**Pronto quando:**
- [ ] Ida e volta (`ToNetTurnCommit(ToTurnCommit(X))`) preserva as 3 ações exatamente, para um `FNetTurnCommit` construído com 3 ações distintas
- [ ] `ToTurnCommit` alimenta `FBattleResolver::ResolveTurn` real sem erro (ponta a ponta, mesmo padrão de `BuildCommitFeedsRealResolver`)

**Verificar:** `Automation RunTests BattleSquare.Net.RoundTripConversion`
**Commit:** `feat(battlesquare): conversão entre commit de rede e commit do núcleo`

---

### T4: `UBattleTurnCoordinator` — acumula os dois commits
**O quê:** `SubmitCommit(uint8 Side, const FTurnCommit&)` — guarda por lado, **nunca** em `UPROPERTY(Replicated)`. Recusa segundo commit do mesmo lado no mesmo turno (idempotência).
**Onde:** `Source/BattleSquare/Public/Net/BattleTurnCoordinator.h` + `.cpp`
**Depende de:** T1
**Requisito:** NET-01, NET-02, edge case "commit duplo"

**Pronto quando:**
- [ ] `SubmitCommit` de um lado não revela nada do outro lado (nenhum getter público expõe o commit antes de `ResolveTurn` disparar)
- [ ] Segundo `SubmitCommit` do mesmo lado no mesmo turno é rejeitado (retorna `false`, não sobrescreve)
- [ ] Nenhum campo de commit é `UPROPERTY(Replicated)` — checável por grep no arquivo

**Verificar:** `Automation RunTests BattleSquare.Net.TurnCoordinatorSubmit`
**Commit:** `feat(battlesquare): coordenador de turno acumula commits sem revelar`

---

### T5: `UBattleTurnCoordinator` — dispara resolução e timeout
**O quê:** quando os dois commits chegam, chama `FBattleResolver::ResolveTurn` real e expõe o resultado via delegate. Timeout preenche o lado ausente com `Aguardar` nas 3 ações (mesma regra de `UBattleActionQueueComponent::Commit`).
**Onde:** mesmo arquivo de T4
**Depende de:** T4
**Requisito:** NET-03, NET-05, NET-06

**Pronto quando:**
- [ ] Com os dois commits presentes, dispara `ResolveTurn` real e emite o resultado (estado + trace) — verificável com um teste que consome ambos
- [ ] Tempo é **injetado** (parâmetro ou campo testável), nunca lido de `FPlatformTime`/`GetWorld()->GetTimeSeconds()` direto — para o teste headless não depender de tempo real
- [ ] Timeout preenche o lado ausente com 3x `Aguardar`, nunca com um commit "adivinhado"
- [ ] Preenchimento por timeout não inventa `EBattleEventType` novo — o registro de "demorou" (se houver) fica fora do vocabulário do trace, num campo/delegate separado

**Verificar:** `Automation RunTests BattleSquare.Net.TurnCoordinatorResolvesAndTimeout` — inclui um caso onde só um lado commita e o tempo injetado ultrapassa `CommitTimeoutSeconds`
**Commit:** `feat(battlesquare): coordenador dispara resolução real e aplica timeout`

---

### T6: Corrida entre os dois commits 🧠
**O quê:** garantir que dois `SubmitCommit` chegando no mesmo instante (ou um chegando exatamente quando o timeout do outro lado estava prestes a disparar) sempre resolvem com os dois commits reais — nunca com um completado por timeout se o outro já tinha chegado, só atrasado.
**Onde:** mesmo arquivo de T4/T5
**Depende de:** T5
**Requisito:** edge case "corrida entre os dois commits"

**Por que 🧠:** é o tipo de bug que só aparece sob timing específico — fácil de escrever um coordenador que "funciona" em todo teste sequencial óbvio e falha só na ordem de chamada exata que corresponde à corrida real.

**Pronto quando:**
- [ ] Teste que injeta: commit do lado 0 no instante T, timeout configurado para T+45, commit do lado 1 chegando em T+44.9 → resolve com os dois commits reais, não com timeout
- [ ] Teste que injeta: só o lado 0 commita, tempo avança além do timeout, SÓ DEPOIS o lado 1 tenta commitar → commit do lado 1 é rejeitado (o turno já resolveu), sem crash
- [ ] Nenhuma condição de corrida real (thread/lock) envolvida — o coordenador roda no game thread do servidor, então "corrida" aqui é sobre ORDEM de chamada, não concorrência de fato; o teste documenta isso explicitamente

**Verificar:** `Automation RunTests BattleSquare.Net.TurnCoordinatorRaceOrdering`
**Commit:** `fix(battlesquare): garante ordem correta entre commit tardio e timeout`

---

### T7: `UBattleNetCommitComponent` — lado do cliente
**O quê:** componente que envia o `FTurnCommit` local (já montado por `UBattleActionQueueComponent`, existente) como `FNetTurnCommit` via `UFUNCTION(Server, Reliable, WithValidation)`, e recebe o resultado via `NetMulticast` ou `Client` RPC.
**Onde:** `Source/BattleSquare/Public/Net/BattleNetCommitComponent.h` + `.cpp`
**Depende de:** T3
**Requisito:** NET-01, NET-04

**Pronto quando:**
- [ ] `Server_SubmitCommit` valida com `ValidateNetTurnCommit` (T2) antes de aceitar — `WithValidation` desconecta o cliente se falhar
- [ ] No servidor, converte para `FTurnCommit` (T3) e repassa para `UBattleTurnCoordinator::SubmitCommit`
- [ ] Recebe resultado (estado + trace) e o expõe via delegate C++, para `ABattleArena` alimentar em `UBattleTracePlayer` sem mudança nenhuma nesse consumidor
- [ ] `HasAuthority()` guarda qualquer caminho que só faz sentido no servidor

**Verificar:** `Automation RunTests BattleSquare.Net.CommitComponentValidatesBeforeForwarding` — simula um payload malformado e confirma que `UBattleTurnCoordinator::SubmitCommit` nunca é chamado
**Commit:** `feat(battlesquare): componente de rede do commit, lado cliente`

---

### T8: Fiação em `ABattleArena` — oponente real substitui a IA sem branch de modo
**O quê:** `HandlePlayerCommitted` passa a delegar para `UBattleTurnCoordinator`. `FDumbOpponentAI` só é chamado quando não há oponente humano conectado — decidido por presença, não por flag.
**Onde:** `Source/BattleSquare/Private/Battle/BattleArena.cpp` (+ `.h` se precisar de novos membros)
**Depende de:** T5, T7
**Requisito:** NET-09, NET-10

**Pronto quando:**
- [ ] Com oponente humano presente, `HandlePlayerCommitted` nunca chama `FDumbOpponentAI`
- [ ] Sem oponente humano (Standalone, ou nenhum outro `PlayerController` na partida), comportamento é **byte a byte** o mesmo de hoje — mesma chamada a `FDumbOpponentAI::GenerateRandomValidCommit`
- [ ] `ResolveTurn` continua sendo chamado de um lugar só no código-fonte (grep confirma uma única ocorrência de `FBattleResolver::ResolveTurn` em `BattleArena.cpp` + `BattleTurnCoordinator.cpp`, nunca duplicada)

**Verificar:** `Automation RunTests BattleSquare.BattleArena.FullTurnEndToEnd` (já existente) continua passando sem modificação — prova que Standalone não regrediu
**Commit:** `feat(battlesquare): arena delega ao coordenador de turno, IA só sem oponente real`

---

### T9: Regressão completa — Standalone intacto
**O quê:** rodar a bateria inteira de `BattleSquare` (30 testes existentes) + os novos testes de `Net.*` juntos, headless.
**Onde:** n/a — verificação, não código
**Depende de:** T8
**Requisito:** NET-09, Success Criteria da spec ("sem nenhuma regressão nos 30 testes")

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` reporta Success == total, Fail == 0
- [ ] `Automation RunTests BattleSim` (44 testes) continua limpo — `BattleSim` não foi tocado, mas confirmar é barato e é a disciplina do projeto

**Verificar:** rodar os dois, ler o log, nunca presumir pelo exit code sozinho
**Commit:** (nenhum — task de verificação; se algo falhar, vira commit de correção)

---

### T10: Sonda `audit_no_commit_replication.sh` [P]
**O quê:** script que falha se `FNetTurnCommit`/`FTurnCommit` aparecer marcado `UPROPERTY(Replicated` ou dentro de `GetLifetimeReplicatedProps`, em qualquer arquivo de `BattleSquare`.
**Onde:** `Tools/audit_no_commit_replication.sh`
**Depende de:** T9
**Requisito:** design.md — "Riscos", commit vazar por replicação acidental

**Pronto quando:**
- [ ] Passa no código atual (nenhum commit é replicado)
- [ ] Falha se alguém plantar `UPROPERTY(Replicated) FNetTurnCommit X;` (testado plantando, mesmo padrão de `probe_isolation.sh`)
- [ ] Falha também se o campo aparecer dentro de `GetLifetimeReplicatedProps` (`DOREPLIFETIME` ou variantes), mesmo sem `UPROPERTY(Replicated` explícito na declaração

**Verificar:** `./Tools/audit_no_commit_replication.sh; echo $?` → `0`; plantar violação, rodar, esperar `1`; remover, confirmar `0` de novo
**Commit:** `chore(battlesquare): sonda anti-replicação do commit`

---

### T11: Investigação — Functional Test com PIE multiplayer real [P]
**O quê:** spike técnico: tentar rodar um `FFunctionalTest`/`AutomationTest` com 2 instâncias PIE (`-PIEMultiplayer` ou equivalente) resolvendo um turno de ponta a ponta, provando que `Actions[3]` atravessa o fio inteiro entre processos separados.
**Onde:** investigação — não necessariamente vira arquivo definitivo se não funcionar no ambiente
**Depende de:** T9
**Requisito:** design.md — "Limite de Ferramenta", as 3 linhas ❌

**Pronto quando:**
- [ ] Ou funciona: existe um teste automatizável (mesmo que não 100% headless) que prova as 3 ações do commit remoto chegando intactas no resolvedor do servidor
- [ ] Ou não funciona no ambiente atual: documentado por que (limitação do runner headless, do `-nullrhi`, do ambiente Mac), e a linha correspondente do design.md permanece ❌ explicitamente, sem forçar

**Verificar:** relatório honesto do resultado — sucesso ou limitação documentada, nunca "presumo que funciona"
**Commit:** `test(battlesquare): investigação de teste PIE multiplayer para commit de rede` (só se produzir algo versionável)

---

### T12: Teste real de commit atravessando 2 processos (condicional a T11) [P]
**O quê:** se T11 provar viável, formalizar o teste — 2 processos/instâncias resolvendo um turno com 3 ações distintas em cada lado, confirmando que o `FBattleState` resultante reflete todas as 6 ações.
**Onde:** a definir por T11 (provavelmente `Source/BattleSquare/Private/Tests/` ou um nível de teste dedicado)
**Depende de:** T11
**Requisito:** design.md — promove uma linha ❌ para ✅

**Pronto quando:**
- [ ] Se T11 viabilizou: teste roda e falha de propósito uma vez (ex.: truncando para 1 ação manualmente) para provar que ele PEGA o defeito, não só que passa no caminho feliz
- [ ] Se T11 não viabilizou: esta tarefa é pulada, e o roteiro de T14 cobre o caso manualmente

**Verificar:** ver acima
**Commit:** `test(battlesquare): commit real de 3 ações confirmado atravessando processos separados` (condicional)

---

### T13: Reconexão e abandono — lógica de coordenação [P]
**O quê:** `UBattleTurnCoordinator`/`GameMode` expõe o `FBattleState` atual para um jogador que reconecta; timeout de abandono (`AbandonTimeoutSeconds`) declara vitória ao jogador presente via `BatalhaEncerrada` já existente.
**Onde:** `Source/BattleSquare/Public/Net/BattleTurnCoordinator.h` (extensão) + integração com `ABattleArena`
**Depende de:** T9
**Requisito:** NET-07, NET-08, NET-11

**Pronto quando:**
- [ ] Reconexão simulada (chamada direta à função de reenvio de estado) entrega o `FBattleState` atual, sem exigir replay de trace (DP-online-03)
- [ ] Jogador desconectado continua sujeito ao mesmo `CommitTimeoutSeconds` — não há tratamento especial que prenda o outro jogador além do timeout normal de commit
- [ ] Abandono (tempo injetado além de `AbandonTimeoutSeconds` sem reconexão) gera `BatalhaEncerrada` com `WinningSide` do jogador presente, usando o evento já existente — nenhum `EBattleEventType` novo

**Verificar:** `Automation RunTests BattleSquare.Net.ReconnectionAndAbandon`
**Commit:** `feat(battlesquare): reconexão por estado completo e vitória por abandono`

---

### T14: Roteiro de verificação de rede real [P]
**O quê:** documento listando o que só se prova com dois processos/duas máquinas de verdade — mesmo espírito de `docs/verification/apresentacao-combate-visual.md`, mas para comportamento de rede em vez de visual.
**Onde:** `docs/verification/combate-online-rede.md`
**Depende de:** T12 (ou sua ausência, se T11 não viabilizou), T13
> 🤖 `haiku` — é um checklist, não lógica

**Pronto quando:**
- [ ] Lista os itens do design.md que ficaram ❌ na tabela de Limite de Ferramenta após T11/T12 (serialização real, dois processos, reconexão com queda de rede de verdade)
- [ ] Cada item tem um passo concreto (ex.: "subir `BattleSquareServer` numa máquina, cliente noutra na mesma rede, montar 3 ações distintas visivelmente diferentes dos 2 lados, confirmar as 6 no log do servidor")
- [ ] Documento explicitamente diz "não verificado ainda" até alguém rodar o roteiro

**Verificar:** revisão humana do próprio roteiro
**Commit:** `docs: roteiro de verificação de rede real do combate online`

---

## Checagem de Granularidade

| Tarefa | Escopo | Situação |
|---|---|---|
| T1 | 2 headers pequenos | ✅ |
| T2–T3 | 1 arquivo, funções puras relacionadas | ✅ |
| T4–T6 | 1 classe, 3 fatias de comportamento (acumular, resolver+timeout, corrida) | ✅ |
| T7 | 1 componente | ✅ |
| T8 | fiação num arquivo já existente | ✅ |
| T9 | verificação, sem código novo | ✅ |
| T10 | 1 script | ✅ |
| T11–T12 | spike + formalização condicional | ✅ — explicitamente pode não produzir código |
| T13 | extensão de 1 classe + integração | ✅ |
| T14 | 1 documento | ✅ |

---

## Cobertura de Requisitos

11 requisitos na spec · **11 mapeados** para tarefas.

| ID | Tarefa(s) |
|---|---|
| NET-01 | T4, T7 |
| NET-02 | T4 |
| NET-03 | T5 |
| NET-04 | T7 |
| NET-05 | T5 |
| NET-06 | T5 |
| NET-07 | T13 |
| NET-08 | T13 |
| NET-09 | T8, T9 |
| NET-10 | T8 |
| NET-11 | T13 |

Nenhum requisito ficou sem tarefa. Os 3 itens da tabela "Limite de Ferramenta" do design (serialização real de ponta a ponta, dois processos, reconexão com queda real) não são requisitos da spec — são **confiança na implementação dos requisitos**, e por isso viram T11/T12/T14, não itens da tabela acima.

---

## Pergunta antes de executar

**Sobre T11:** é o maior risco de escopo desta lista — pode não ser possível rodar PIE multiplayer automatizado no ambiente atual (headless, `-nullrhi`, Mac). Se travar, o plano é não insistir além de uma investigação honesta: documentar a limitação e seguir com T13/T14 normalmente, deixando o roteiro manual (T14) como a prova real até haver um ambiente de teste com rede de verdade.

**Sobre infraestrutura:** nenhuma tarefa aqui depende de dedicated server implantado — `BattleSquareServer.Target.cs` já existe e compila localmente; "implantar" fica fora de escopo (ver spec.md, Out of Scope).
