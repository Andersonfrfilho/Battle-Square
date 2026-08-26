# Streaming de Mundo — Tarefas

**Design:** `.specs/features/streaming-de-mundo/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** `BattleSquare` (C++, T1) **e** Editor/`Content/` (T2–T5, via `unreal-mcp`). Primeira feature do projeto com trabalho de Editor real — T2–T5 não são verificáveis por `Automation RunTests`, só pelo roteiro de T7 (ver DP-streaming-05).

---

## Plano de Execução

### Fase 1 — Componente de rota, C++ puro (sequencial, testável headless)
> 🤖 Modelo: `sonnet`

```
T1
```

### Fase 2 — Nível e streaming (depende da Fase 1, trabalho de Editor)
> 🤖 Modelo: `sonnet` — **T2 é 🧠** (primeira vez que World Partition é configurado no projeto)

```
T2 → T3 → T4
```

### Fase 3 — HLOD (depende da Fase 2)
> 🤖 Modelo: `sonnet`

```
T5
```

### Fase 4 — Verificação (depende da Fase 3)
> 🤖 Modelo: `sonnet` para T6 · `haiku` para T7

```
T6 → T7
```

---

## T1 — `UDebugRouteMoverComponent`

**Arquivos:** `Source/BattleSquare/Public/World/DebugRouteMoverComponent.h`, `.../Private/World/DebugRouteMoverComponent.cpp`, teste em `Source/BattleSquare/Private/Tests/DebugRouteMoverComponentTest.cpp`.
**O que fazer:** `UActorComponent` com `TArray<FVector> Waypoints`, `float SpeedUnitsPerSecond`, `TickComponent` avançando `Owner`'s posição em linha reta até o waypoint atual (tolerância nomeada `WaypointReachedToleranceUnits`), avança índice ao alcançar, PARA no último (sem loop). Zero dependência de `BattleSim`.
**Testes:** rota de 3 waypoints, tempo simulado por chamadas manuais de `TickComponent` (nunca tempo real) — cobre: chega ao primeiro waypoint, avança para o segundo, para no último sem ultrapassar, rota vazia não crasha.
**Pronto quando:** `Automation RunTests BattleSquare.DebugRouteMover` passa, determinístico (mesma sequência de `TickComponent` produz sempre a mesma posição final).

---

## T2 — Nível `WorldStreamingTest` com World Partition 🧠

**Como:** via `unreal-mcp` (Editor aberto, plugin `ModelContextProtocol` + `AllToolsets` ativos, `ModelContextProtocol.StartServer` rodando).
**O que fazer:** criar nível novo em `Content/Maps/WorldStreamingTest`, habilitar World Partition na criação, configurar `WorldPartitionRuntimeHashSettings` com tamanho de célula 512x512uu e raio de streaming 1024uu (DP-streaming-02).
**Pronto quando:** o nível abre no Editor, World Partition está habilitado (visível no painel World Partition do Editor), configuração de célula/raio confere com o design.

---

## T3 — Conteúdo de placeholder

**O que fazer:** grid regular de Static Mesh Actors (cubo padrão) distribuído nas células do nível, um material de cor distinta por célula (facilita verificação visual em T6).
**Pronto quando:** o grid está visível no nível, cores distinguem células adjacentes na janela do Editor.

---

## T4 — Pawn de debug com rota

**O que fazer:** um `Pawn` mínimo (sem gameplay de batalha) com `UDebugRouteMoverComponent` (T1) anexado, `Waypoints` cobrindo uma rota que atravessa várias células do nível (extremidade a extremidade da área de referência), `SpeedUnitsPerSecond` definido para completar a rota num tempo razoável de verificação manual (ordem de 30–60s).
**Pronto quando:** o pawn existe no nível, a rota está definida e cobre a área de referência ponta a ponta.

---

## T5 — HLOD

**O que fazer:** criar uma `HLODLayer` e atribuí-la às malhas de placeholder (T3); rodar o build de HLOD (`WorldPartitionHLODsBuilder`, via Editor ou commandlet).
**Pronto quando:** o build de HLOD completa sem erro; regiões distantes do pawn de debug mostram a representação de HLOD em vez da geometria completa (visível no Editor com o modo de visualização de HLOD ativo).

---

## T6 — Roteiro de verificação manual

**Arquivo:** `docs/verification/streaming-de-mundo.md`.
**O que fazer:** documentar o passo a passo para reproduzir manualmente os itens não-automatizáveis (DP-streaming-05): rodar o pawn de debug pela rota, observar `stat streaming` nos extremos, confirmar ausência de tela de loading, comparar `stat unit`/`stat gpu` com e sem HLOD. Cada item marcado "não verificado ainda" até alguém rodar de fato — mesmo padrão de `apresentacao-combate-visual.md`.
**Pronto quando:** o roteiro existe e é executável por qualquer pessoa com o Editor aberto, sem depender de contexto desta sessão.

---

## T7 — Regressão completa

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` — Success == total (127 + o(s) novo(s) teste(s) de T1), Fail == 0
- [ ] `Automation RunTests BattleSim` (52 testes) — continua limpo, zero linha tocada
- [ ] `./Tools/audit_determinism.sh`, `./Tools/audit_no_recalculation.sh`, `./Tools/probe_isolation.sh` — todos `exit 0`
- [ ] **L-020 aplicada:** rebuild de verdade depois de `probe_isolation.sh`, antes de rodar testes
