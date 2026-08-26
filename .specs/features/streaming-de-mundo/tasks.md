# Streaming de Mundo — Tarefas

**Design:** `.specs/features/streaming-de-mundo/design.md`
**Status:** ✅ CONCLUÍDO — T1 a T7 executadas e verificadas (2026-08-26)
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

## T2 — Nível `WorldStreamingTest` com World Partition 🧠 ✅

**Como:** via `unreal-mcp` (Editor aberto, plugin `ModelContextProtocol` + `AllToolsets` ativos, `ModelContextProtocol.StartServer` rodando).
**O que fazer:** criar nível novo em `Content/Maps/WorldStreamingTest`, habilitar World Partition na criação, configurar `WorldPartitionRuntimeHashSettings` com tamanho de célula 512x512uu e raio de streaming 1024uu (DP-streaming-02).
**Pronto quando:** o nível abre no Editor, World Partition está habilitado (visível no painel World Partition do Editor), configuração de célula/raio confere com o design.
**Feito:** `/Game/Maps/WorldStreamingTest`, criado por duplicação de `/Engine/Maps/Templates/OpenWorld` (o mesmo template que o "New Level → Open World" do Editor usa). World Partition habilitado, `MainGrid` (`URuntimePartitionLHGrid`) com **cellSize 1600 / loadingRange 3200**, `bIs2D` ligado.
**Desvio do design, documentado em DP-streaming-02a:** os 512uu pedidos são inalcançáveis — `URuntimePartitionLHGrid::PostEditChangeProperty` faz `CellSize = FMath::Max<int32>(CellSize, 1600)`. Escrever 512 retorna sucesso e a leitura seguinte devolve 1600. Adotado o piso da engine mantendo a intenção original (raio = 2 células, vizinhança 5x5).

---

## T3 — Conteúdo de placeholder ✅

**O que fazer:** grid regular de Static Mesh Actors (cubo padrão) distribuído nas células do nível, um material de cor distinta por célula (facilita verificação visual em T6).
**Pronto quando:** o grid está visível no nível, cores distinguem células adjacentes na janela do Editor.
**Feito:** 225 `StaticMeshActor` (cubo padrão, escala 3), 9 por célula numa grade 5x5 de células de 1600uu centrada na origem — área de referência 8000x8000uu. 25 `MaterialInstanceConstant` (`/Game/Streaming/Materials/MI_StreamingCell_R{0..4}C{0..4}`) derivadas de `/Engine/BasicShapes/BasicShapeMaterial`, matiz distribuída para que células vizinhas nunca caiam em matizes próximas. Organizados no Outliner em `StreamingPlaceholder/R{n}C{n}` e marcados com a tag `StreamingPlaceholder`.

---

## T4 — Pawn de debug com rota ✅

**O que fazer:** um `Pawn` mínimo (sem gameplay de batalha) com `UDebugRouteMoverComponent` (T1) anexado, `Waypoints` cobrindo uma rota que atravessa várias células do nível (extremidade a extremidade da área de referência), `SpeedUnitsPerSecond` definido para completar a rota num tempo razoável de verificação manual (ordem de 30–60s).
**Pronto quando:** o pawn existe no nível, a rota está definida e cobre a área de referência ponta a ponta.
**Feito:** `DebugRoutePawn` (`ADefaultPawn`, pasta `StreamingDebug/`) com `UDebugRouteMoverComponent` anexado. Começa em `(-3600,-3600,300)`; waypoints `(3600,-3600,300)` → `(3600,3600,300)` → `(-3600,3600,300)`, percorrendo três lados da área de referência. `SpeedUnitsPerSecond = 400` → 21.600uu / 400 = **54s** de rota, dentro da faixa de 30–60s pedida. `AutoPossessPlayer = Player0` para que o pawn seja a fonte de streaming em PIE sem input humano.

---

## T5 — HLOD ✅

**O que fazer:** criar uma `HLODLayer` e atribuí-la às malhas de placeholder (T3); rodar o build de HLOD (`WorldPartitionHLODsBuilder`, via Editor ou commandlet).
**Pronto quando:** o build de HLOD completa sem erro; regiões distantes do pawn de debug mostram a representação de HLOD em vez da geometria completa (visível no Editor com o modo de visualização de HLOD ativo).
**Feito:** `HLODLayer` `/Game/Streaming/HLOD/HLOD_StreamingPlaceholder` (tipo *Instancing*, sempre carregada), atribuída aos 225 cubos, referenciada como `defaultHLODLayer` do World Partition e no `hLODSetups` do `WorldPartitionRuntimeHashSet`. Build via `WorldPartitionBuilderCommandlet -Builder=WorldPartitionHLODsBuilder`: **36 atores de HLOD construídos**, sem erro. A segunda metade do critério (ver HLOD no lugar da geometria) é visual e vive em STREAM-03 do roteiro de T6 — não é verificável headless.

---

## T6 — Roteiro de verificação manual ✅

**Arquivo:** `docs/verification/streaming-de-mundo.md`.
**O que fazer:** documentar o passo a passo para reproduzir manualmente os itens não-automatizáveis (DP-streaming-05): rodar o pawn de debug pela rota, observar `stat streaming` nos extremos, confirmar ausência de tela de loading, comparar `stat unit`/`stat gpu` com e sem HLOD. Cada item marcado "não verificado ainda" até alguém rodar de fato — mesmo padrão de `apresentacao-combate-visual.md`.
**Pronto quando:** o roteiro existe e é executável por qualquer pessoa com o Editor aberto, sem depender de contexto desta sessão.
**Feito:** `docs/verification/streaming-de-mundo.md`, com preparação comum e cinco itens (STREAM-01 células carregam/descarregam, STREAM-02 ausência de tela de loading, STREAM-03 HLOD em regiões distantes, STREAM-04 custo com e sem HLOD, orçamento de memória de DP-streaming-01). Todos marcados **não verificado ainda**, no mesmo padrão de `apresentacao-combate-visual.md`.

---

## T7 — Regressão completa ✅

**Pronto quando:**
- [x] `Automation RunTests BattleSquare` — **77 Success, 0 Fail** (75 anteriores + 2 novos de T1)
- [x] `Automation RunTests BattleSim` — **52 Success, 0 Fail**, zero linha tocada (total 129/129)
- [x] `./Tools/audit_determinism.sh`, `./Tools/audit_no_recalculation.sh`, `./Tools/probe_isolation.sh` — todos `exit 0`
- [x] **L-020 aplicada:** rebuild real (`Build.sh BattleSquareEditor Mac Development`, `Result: Succeeded`) depois de `probe_isolation.sh` e antes dos testes
