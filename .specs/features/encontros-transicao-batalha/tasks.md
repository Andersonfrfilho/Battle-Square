# Encontros e Transição para Batalha — Tarefas

**Design:** `.specs/features/encontros-transicao-batalha/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** `BattleSquare` (C++, T1–T5) **e** Editor/`Content/` (T6, via `unreal-mcp`). `BattleSim` não é tocado.

---

## Plano de Execução

### Fase 1 — Detecção, C++ puro (sequencial, testável headless)
> 🤖 Modelo: `sonnet`

```
T1 → T2
```

### Fase 2 — Transição (depende da Fase 1)
> 🤖 Modelo: `sonnet` — **T4 é 🧠** (ordem de operações da restauração é onde o bug de P1/critério 4 nasce)

```
T3 → T4
```

### Fase 3 — Fiação e conteúdo (depende da Fase 2)
> 🤖 Modelo: `sonnet` para T5 · trabalho de Editor em T6

```
T5 → T6
```

### Fase 4 — Verificação (depende da Fase 3)
> 🤖 Modelo: `sonnet` para T7 · `haiku` para T8

```
T7 → T8
```

---

## T1 — `FEncounterDetector`, função pura de disparo

**Arquivos:** `Source/BattleSquare/Public/World/EncounterDetector.h`, `.../Private/World/EncounterDetector.cpp`, teste em `.../Private/Tests/EncounterDetectorTest.cpp`.
**O que fazer:** `FEncounterCandidate` (posição, `EncounterRadiusUnits`, `CatalogId`, `bIsResolved`) e `FEncounterDetector::FindTriggeredEncounter(const FEncounterDetectionParams&)` estático, devolvendo índice ou `INDEX_NONE`. Distância **ao quadrado**, sem `Sqrt`. Desempate: mais próximo vence; empate exato resolve pelo menor índice (DP-enc-02).
**Testes:** entra no raio dispara; passa fora não dispara; candidato `bIsResolved` é ignorado mesmo dentro do raio; dois candidatos no raio → o mais próximo; empate exato → menor índice; array vazio devolve `INDEX_NONE` sem crash.
**Pronto quando:** `Automation RunTests BattleSquare.World.EncounterDetector` passa; nenhum `UWorld` é construído em teste algum.

---

## T2 — `AWorldEncounterActor`

**Arquivos:** `Source/BattleSquare/Public/World/WorldEncounterActor.h`, `.../Private/World/WorldEncounterActor.cpp`.
**O que fazer:** `AActor` com `CatalogId`, `EncounterRadiusUnits`, `bIsResolved` e um `UStaticMeshComponent` como raiz. Sem `Tick`, sem componente de colisão/overlap (DP-enc-01). Um método nomeado para colher o `FEncounterCandidate` correspondente.
**Pronto quando:** compila, aparece no Content Browser como classe spawnável, e um teste headless confirma que o `FEncounterCandidate` colhido reflete `CatalogId`/raio/resolvido do ator.

---

## T3 — `UEncounterDetectionComponent`

**Arquivos:** `Source/BattleSquare/Public/World/EncounterDetectionComponent.h`, `.../Private/World/EncounterDetectionComponent.cpp`, teste em `.../Private/Tests/EncounterDetectionComponentTest.cpp`.
**O que fazer:** `UActorComponent` que, por `TickComponent`, colhe os `AWorldEncounterActor` do mundo, monta os candidatos, chama `FEncounterDetector` e dispara um delegate `OnEncounterTriggered` com o ator disparado. Precisa poder ser **desligado e religado** (é o que DP-enc-03 exige na transição).
**Testes:** com a lista de candidatos injetada (não colhida do mundo), o delegate dispara exatamente uma vez ao entrar no raio; não dispara enquanto desligado; religar depois de o candidato virar `bIsResolved` não dispara.
**Pronto quando:** `Automation RunTests BattleSquare.World.EncounterDetectionComponent` passa, e a coleta do mundo é separável da regra (a regra é testada sem `UWorld`).

---

## T4 — `UWorldBattleTransitionService` 🧠

**Arquivos:** `Source/BattleSquare/Public/World/WorldBattleTransitionService.h`, `.../Private/World/WorldBattleTransitionService.cpp`, teste em `.../Private/Tests/WorldBattleTransitionServiceTest.cpp`.
**O que fazer:** captura de `FTransform` do pawn, desligamento da detecção, spawn do `ABattleArena` em `BattleArenaWorldOffsetUnits` (constante nomeada, `*.constant.ts` equivalente em C++ — nunca literal solto), montagem pelos caminhos existentes, e ao `BatalhaEncerrada`: destruir arena → restaurar transform → marcar `bIsResolved` → **só então** religar a detecção (DP-enc-03).
**Testes:** transform restaurado é idêntico ao capturado; a arena é destruída **depois** de `BatalhaEncerrada` ser processado (nunca antes — senão XP/captura se perdem, DP-enc-04); um tick com o pawn parado em cima do pet resolvido não dispara segundo encontro; a montagem carrega o `CatalogId` do encontro no lado do oponente.
**Pronto quando:** `Automation RunTests BattleSquare.World.WorldBattleTransitionService` passa, e nenhuma linha desta classe calcula atributo, dano ou XP.

---

## T5 — Fiação no fluxo de jogo

**O que fazer:** ligar `UEncounterDetectionComponent` → `UWorldBattleTransitionService` no caminho de jogo real (pawn do mundo + `ABattleSquareGameMode`), sem criar um segundo caminho de montagem de partida paralelo ao que M2 já tem.
**Pronto quando:** um teste headless leva um pawn do mundo até um encontro e chega a um `ABattleArena` montado com os dois `CatalogId` corretos, passando pelo `ABattleSquareGameMode` existente.

---

## T6 — Encontros no nível `WorldStreamingTest`

**Como:** via `unreal-mcp` (Editor aberto). **Aplicar L-024:** salvar a cada passo que muda asset, e não escrever propriedade de asset que o design não exige.
**O que fazer:** posicionar alguns `AWorldEncounterActor` sobre a rota do `DebugRoutePawn` (que passa por `(-3600,-3600)` → `(3600,-3600)` → `(3600,3600)` → `(-3600,3600)`), com `CatalogId` de pets do catálogo de fixture, e adicionar o `UEncounterDetectionComponent` ao `DebugRoutePawn`.
**Pronto quando:** os atores existem no nível, salvos, e o `DebugRoutePawn` tem o componente de detecção anexado.

---

## T7 — Roteiro de verificação manual

**Arquivo:** `docs/verification/encontros-transicao-batalha.md`.
**O que fazer:** documentar os itens não-automatizáveis de DP-enc-05 — ausência de tela de loading na transição, e `stat streaming` provando que as células do mundo continuam carregadas durante a batalha. Mesmo padrão de `streaming-de-mundo.md`: cada item com passo concreto e número a anotar, marcado "não verificado ainda".
**Pronto quando:** o roteiro existe e é executável sem depender do contexto desta sessão.

---

## T8 — Regressão completa

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` — Success == total (77 + os novos), Fail == 0
- [ ] `Automation RunTests BattleSim` — 52 Success, Fail == 0, zero linha tocada
- [ ] `./Tools/audit_determinism.sh`, `./Tools/audit_no_recalculation.sh`, `./Tools/probe_isolation.sh` — todos `exit 0`
- [ ] **L-020 aplicada:** rebuild real depois de `probe_isolation.sh`, antes dos testes que contam
