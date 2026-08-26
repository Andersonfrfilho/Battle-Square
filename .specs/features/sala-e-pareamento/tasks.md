# Sala e Pareamento Simples — Tarefas

**Design:** `.specs/features/sala-e-pareamento/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`, `Net/`. `UBattleRoomRegistry` (T1–T6) é lógica pura, testável headless. `ABattleSquareGameMode` (T7–T9) é fiação de ator — parcialmente testável (ver design.md, Limite de Ferramenta).

---

## Plano de Execução

### Fase 1 — Código de sala, criar e entrar (sequencial)
> 🤖 Modelo: `sonnet`

```
T1 → T2 → T3
```

### Fase 2 — Desconexão, reconexão, abandono (sequencial, depende da Fase 1)
> 🤖 Modelo: `sonnet` — **T5 é 🧠**

```
T4 → T5 → T6
```

### Fase 3 — GameMode, fiação com a arena real (sequencial, depende da Fase 2)
> 🤖 Modelo: `sonnet`

```
T7 → T8 → T9
```

### Fase 4 — Verificação (paralelo após Fase 3)
> 🤖 Modelo: `sonnet` para T10 · `haiku` para T11

```
T9 ──┬→ T10 [P]
     └→ T11 [P]
```

---

## Tarefas

### T1: Tipos de sala e geração de código
**O quê:** `FBattleRoomOccupant`, `FBattleRoomState`, `EBattleRoomJoinResult` (enum: `Success`, `NotFound`, `Full`). `GenerateCandidateCode()` — código curto, alfanumérico, sem `0`/`O`/`1`/`I`.
**Onde:** `Source/BattleSquare/Public/Net/BattleRoomTypes.h`
**Depende de:** nada
**Requisito:** SALA-01, SALA-02

**Pronto quando:**
- [ ] `GenerateCandidateCode` nunca produz `0`, `O`, `1` ou `I`
- [ ] Formato e tamanho do código são constantes nomeadas (mesmo padrão de `BattleNetConstants.h`), não números mágicos
- [ ] Compila

**Verificar:** `Automation RunTests BattleSquare.Net.RoomCode.ExcludesAmbiguousCharacters`
**Commit:** `feat(battlesquare): tipos de sala e geração de código`

---

### T2: `UBattleRoomRegistry::CreateRoom`
**O quê:** cria uma sala com Side 0 ocupado, garante que o código não colide com nenhuma sala **viva** (gera de novo se colidir).
**Onde:** `Source/BattleSquare/Public/Net/BattleRoomRegistry.h` + `.cpp`
**Depende de:** T1
**Requisito:** SALA-01, SALA-02, DP-sala-04 (segredo)

**Pronto quando:**
- [ ] `CreateRoom` retorna um código e preenche `OutCreatorSecret` com um `FGuid` válido (não zero)
- [ ] Duas chamadas seguidas nunca produzem o mesmo código enquanto ambas as salas estiverem vivas
- [ ] A sala criada tem `bHasSide1 = false` — ninguém mais entrou ainda

**Verificar:** `Automation RunTests BattleSquare.Net.RoomRegistry.CreateRoomIsUnique`
**Commit:** `feat(battlesquare): criação de sala com código único`

---

### T3: `UBattleRoomRegistry::JoinRoom`
**O quê:** entra numa sala existente com 1 vaga; distingue `NotFound` de `Full`.
**Onde:** mesmo arquivo de T2
**Depende de:** T2
**Requisito:** SALA-03, SALA-04, SALA-07, edge case "dois entrando ao mesmo tempo"

**Pronto quando:**
- [ ] Código válido com 1 ocupante → `Success`, `OutJoinerSecret` preenchido, sala passa a `bHasSide1 = true`
- [ ] Código inexistente → `NotFound`, sem side effect
- [ ] Código de sala já cheia (2 ocupantes) → `Full`, sem side effect, **nunca** um terceiro lado
- [ ] Quem cria é sempre Side 0, quem entra é sempre Side 1 — sem exceção (DP-sala-03)

**Verificar:** `Automation RunTests BattleSquare.Net.RoomRegistry.JoinRoomDistinguishesNotFoundFromFull`
**Commit:** `feat(battlesquare): entrada em sala com distinção not-found/cheia`

---

### T4: Desconexão e reconexão por segredo
**O quê:** `MarkDisconnected(Code, Side, CurrentTime)`, `TryReconnect(Code, Secret)`.
**Onde:** mesmo arquivo de T2
**Depende de:** T3
**Requisito:** SALA-09 (parte 1), SALA-10

**Pronto quando:**
- [ ] `MarkDisconnected` registra o lado como desconectado, com o tempo injetado
- [ ] `TryReconnect` com código + segredo batendo com o lado desconectado retorna `true` e marca como reconectado
- [ ] `TryReconnect` com segredo errado (não bate com nenhum lado daquela sala) retorna `false`, sem alterar nada
- [ ] `TryReconnect` num lado que nunca desconectou é rejeitado (nada para reconectar)

**Verificar:** `Automation RunTests BattleSquare.Net.RoomRegistry.DisconnectAndReconnect`
**Commit:** `feat(battlesquare): desconexão e reconexão por segredo de sessão`

---

### T5: Timeout de abandono 🧠
**O quê:** `CheckAbandonment(CurrentTime)` — para cada sala com um lado desconectado há mais que `AbandonTimeoutSeconds`, dispara `OnRoomAbandoned(Code, PresentSide)` exatamente uma vez.
**Onde:** mesmo arquivo de T2
**Depende de:** T4
**Requisito:** SALA-09 (parte 2)

**Por que 🧠:** mesma classe de risco de T6 do Combate Online — reconexão chegando bem perto do timeout precisa vencer a corrida corretamente, e o delegate não pode disparar mais de uma vez pela mesma sala.

**Pronto quando:**
- [ ] Desconexão + tempo injetado além de `AbandonTimeoutSeconds` sem reconexão → `OnRoomAbandoned` dispara com o lado presente
- [ ] Reconexão chegando ANTES do timeout (mesmo que perto) cancela o abandono — `CheckAbandonment` depois disso não dispara
- [ ] `CheckAbandonment` chamado múltiplas vezes após o abandono já ter disparado NÃO dispara de novo (idempotência)
- [ ] Os dois lados desconectados ao mesmo tempo: nenhum dos dois é "o presente" — `OnRoomAbandoned` não dispara (vira caso de sala vazia, T6)

**Verificar:** `Automation RunTests BattleSquare.Net.RoomRegistry.AbandonmentTimeoutRaceOrdering`
**Commit:** `fix(battlesquare): timeout de abandono sem disparo duplo nem corrida perdida`

---

### T6: Sala vazia é destruída
**O quê:** `CheckEmptyRooms(CurrentTime)` — sala com os dois lados desconectados além do timeout é removida do registro; o código fica livre para reuso.
**Onde:** mesmo arquivo de T2
**Depende de:** T5
**Requisito:** SALA-11

**Pronto quando:**
- [ ] Sala com os dois lados desconectados além do timeout deixa de existir no registro
- [ ] Código de uma sala destruída pode ser gerado de novo por `CreateRoom` sem colisão falsa
- [ ] Sala com pelo menos um lado ainda conectado (ou dentro do timeout) NUNCA é destruída

**Verificar:** `Automation RunTests BattleSquare.Net.RoomRegistry.EmptyRoomIsDestroyedAndCodeReused`
**Commit:** `feat(battlesquare): salas abandonadas por completo são destruídas`

---

### T7: `ABattleSquareGameMode` — scaffold e RPCs
**O quê:** `AGameModeBase` dono de um `UBattleRoomRegistry`; `Logout` chama `MarkDisconnected`. RPCs finos em `APlayerController` (`Server_CreateRoom`, `Server_JoinRoom`) repassando para o GameMode.
**Onde:** `Source/BattleSquare/Public/Net/BattleSquareGameMode.h` + `.cpp`, extensão mínima de `APlayerController` (novo `Source/BattleSquare/Public/Net/BattleSquarePlayerController.h`)
**Depende de:** T6
**Requisito:** SALA-01, SALA-03, SALA-09 (integração)

**Pronto quando:**
- [ ] `ABattleSquareGameMode` compila e instancia sem crash num mundo de teste (mesmo padrão `CreateHeadlessTestWorld` de `BattleArenaTest.cpp`)
- [ ] `Logout` chama `MarkDisconnected` com o lado correto do jogador que saiu
- [ ] RPCs não têm lógica própria — só repassam para `RoomRegistry`, mesmo espírito fino de `UBattleNetCommitComponent::Server_SubmitCommit`

**Verificar:** teste headless confirmando `Logout` chama `MarkDisconnected` (via mundo de teste); **RPC atravessando rede real fica para o roteiro manual (T11)**
**Commit:** `feat(battlesquare): GameMode e RPCs finos de sala`

---

### T8: `HandleRoomReady` — sala cheia vira partida real
**O quê:** ao `OnRoomReady`, instancia `ABattleArena`, cria `UBattleTurnCoordinator`, chama `ConfigureNetworkedOpponent`, monta `FBattleState` inicial com os 2 primeiros pets de `FPetDataLoader::LoadVerifiedPets()` (placeholder deliberado, ver design.md).
**Onde:** mesmo arquivo de T7
**Depende de:** T7
**Requisito:** SALA-06, SALA-07, SALA-08

**Pronto quando:**
- [ ] `ABattleArena` real é instanciado, com `UBattleTurnCoordinator` real conectado (não um coordenador de teste criado à mão)
- [ ] `FBattleState` inicial usa pets reais do espelho local — se o espelho estiver vazio/ausente, o erro já existente de `FPetDataLoader` propaga, nunca é escondido
- [ ] Side 0 é sempre quem criou a sala

**Verificar:** teste headless (mundo de teste) confirmando que, após `OnRoomReady`, existe um `ABattleArena` com `GetCurrentState().Pets.Num() == 2`
**Commit:** `feat(battlesquare): sala cheia monta arena e coordenador reais`

---

### T9: `HandleRoomAbandoned` — abandono real aciona o coordenador real
**O quê:** ao `OnRoomAbandoned`, chama `UBattleTurnCoordinator::DeclareAbandonment` do lado presente, na instância real montada por T8 (não mais só testável chamando a função direto).
**Onde:** mesmo arquivo de T7
**Depende de:** T8
**Requisito:** SALA-09 (fechamento do ciclo completo)

**Pronto quando:**
- [ ] `OnRoomAbandoned` disparado pelo registro aciona `DeclareAbandonment` no coordenador da sala correta (não uma sala qualquer, se houver mais de uma ativa)
- [ ] Teste de ponta a ponta: criar sala → entrar → desconectar um lado → avançar tempo injetado além do timeout → confirma `DeclareAbandonment` chamado, sem nenhuma chamada de teste direta ao coordenador

**Verificar:** `Automation RunTests BattleSquare.Net.GameMode.AbandonmentReachesRealCoordinator`
**Commit:** `feat(battlesquare): abandono real encadeado do registro ao coordenador`

---

### T10: Regressão completa [P]
**O quê:** rodar toda a bateria de `BattleSquare` + `BattleSim` junto com os novos testes.
**Onde:** n/a — verificação
**Depende de:** T9
**Requisito:** Success Criteria da spec

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` — Success == total, Fail == 0
- [ ] `Automation RunTests BattleSim` (44 testes) — continua limpo

**Verificar:** rodar os dois, ler o log
**Commit:** (nenhum — task de verificação)

---

### T11: Extensão do roteiro de verificação de rede [P]
**O quê:** adicionar a `docs/verification/combate-online-rede.md` os itens desta feature que só rede real prova — RPCs de sala atravessando conexão real, e o ciclo completo criar→entrar→jogar→abandonar com dois processos.
**Onde:** `docs/verification/combate-online-rede.md` (extensão, não documento novo)
**Depende de:** T9
> 🤖 `haiku` — é um checklist, não lógica

**Pronto quando:**
- [ ] Nova seção no documento existente, mesmo formato dos itens já lá (passo concreto, "não verificado ainda")
- [ ] Referencia o mesmo bloqueio de B-004 (Server Target não compila) em vez de repetir a explicação

**Verificar:** revisão humana do próprio roteiro
**Commit:** `docs: estende roteiro de verificação de rede para sala e pareamento`

---

## Cobertura de Requisitos

11 requisitos na spec · **11 mapeados** para tarefas.

| ID | Tarefa(s) |
|---|---|
| SALA-01 | T2, T7 |
| SALA-02 | T1, T2 |
| SALA-03 | T3, T7 |
| SALA-04 | T3 |
| SALA-05 | T8 (via `OnRoomReady` disparado quando `bHasSide1` vira true — T3) |
| SALA-06 | T8 |
| SALA-07 | T3, T8 |
| SALA-08 | T8 |
| SALA-09 | T4, T5, T7, T9 |
| SALA-10 | T4 |
| SALA-11 | T6 |

## Pergunta antes de executar

Mesma ressalva do Combate Online: T7–T9 tocam `AGameModeBase`/`APlayerController` pela primeira vez no projeto. A parte que decide (registro de sala) é 100% testável; a parte que só existe com rede real de verdade segue bloqueada por B-004 (engine sem suporte a `TargetType.Server`) — não é escopo novo a resolver aqui, é o mesmo limite já documentado.
