# Streaming de Mundo — Design

**Status:** Aprovado para implementação
**Spec:** `spec.md`

---

## DP-streaming-01: Orçamento de performance (a decisão mais pesada)

**Decisão:** PC como plataforma de referência (AD-001/M6 — mobile é marco separado, validado depois). Orçamento:

| Métrica | Valor |
|---|---|
| Memória de streaming (células carregadas simultaneamente) | 512 MB |
| Frame time alvo | 16,6 ms (60 FPS) em PC de referência (desktop médio, sem GPU dedicada de ponta) |
| Raio de streaming (distância de carregamento ao redor do jogador) | derivado em DP-streaming-02 |

**Razão:** 512 MB é generoso o bastante para provar streaming real (várias células, conteúdo de placeholder não-trivial) sem já comprometer o orçamento total de uma sessão de jogo em PC (tipicamente 4–8 GB disponíveis para o processo inteiro). 60 FPS é o piso aceitável para um jogo de ação/exploração — não é negociado nesta feature, é herdado como padrão geral do projeto.
**Nota honesta:** este número é uma decisão de partida, não uma medição — não há hardware de referência formalmente definido ainda (Todo em `STATE.md`). Ele existe para DAR um número do qual derivar o resto (DP-streaming-02), evitando a armadilha de decidir tamanho de célula "no olho". Revisão esperada quando M6 (Mobile) definir o piso real de dispositivo.

## DP-streaming-02: Tamanho de célula e raio de streaming

**Decisão:** célula de World Partition de 512x512 unidades Unreal (5,12m x 5,12m em escala 1uu=1cm), raio de streaming de 2 células (1024uu) ao redor do jogador — 5x5 células carregadas na grade quando o jogador está no centro de uma.
**Razão:** célula pequena o bastante para que o teste de streaming (P1) demonstre carregar/descarregar dentro de uma rota curta, sem exigir um mundo artificialmente enorme só para provar o mecanismo.

## DP-streaming-03: Conteúdo de placeholder

**Decisão:** um grid regular de cubos estáticos (Static Mesh Actor, cubo padrão da engine) espalhados nas células, com um material de cor distinta por célula (facilita verificação visual de qual célula está carregada em screenshot/roteiro manual). Sem terreno/landscape nesta feature — landscape é autoria de conteúdo, fora de escopo (spec.md).
**Razão:** custo mínimo de autoria, mas suficiente para: (a) provar que geometria real entra/sai de memória com a célula, (b) ser visualmente distinguível em verificação manual.

## DP-streaming-04: Movimento do pawn de debug

**Decisão:** um `Pawn` de debug simples (`ADefaultPawn` ou equivalente da engine, sem gameplay próprio) com um componente C++ novo, `UDebugRouteMoverComponent`, que percorre uma lista fixa de waypoints (`TArray<FVector>`) a velocidade constante, LOOP nunca — termina no último waypoint. Determinístico: mesma rota, mesmo tempo, sempre.
**Razão:** DP-streaming-05 exige uma rota REPRODUZÍVEL para que o teste headless (Automation) possa medir memória/atores carregados em pontos fixos da rota, sem depender de input humano. Componente é C++ puro, testável fora do Editor da mesma forma que o resto do código deste projeto.

## DP-streaming-05: O que é automatizável vs. manual

**Decisão, com a franqueza que a spec já pede:**

| Verificação | Automatizável? | Como |
|---|---|---|
| `UDebugRouteMoverComponent` percorre waypoints na ordem certa, determinístico | ✅ Sim | Teste headless puro (`Automation RunTests`), sem precisar de streaming real — só a lógica do componente |
| Streaming de World Partition de fato carrega/descarrega células | ❌ Não, de forma confiável | A API de streaming da Unreal (`UWorldPartitionSubsystem`) existe e é consultável em runtime, mas medir isso de forma determinística num teste `nullrhi` headless é frágil — streaming depende de threads de I/O assíncronas que não são o alvo do determinismo que este projeto já pratica (AD-004 é sobre `BattleSim`, não sobre o motor de streaming da engine). **Decisão: verificação MANUAL**, via roteiro em `docs/verification/`, com o Editor aberto e o pawn de debug rodando a rota, observando o contador de atores carregados (`stat streaming` / painel de estatísticas de World Partition do Editor) nos pontos de extremo da rota. |
| Ausência de tela de loading | ❌ Não | Mesma razão — verificação manual, visual, no roteiro |
| HLOD reduzindo custo (P2) | ❌ Não, de forma barata | `stat unit`/`stat gpu` comparando frame time com e sem HLOD habilitado — roteiro manual |

**Precedente já existente no projeto:** `docs/verification/apresentacao-combate-visual.md` já assumiu exatamente essa divisão (lógica testável headless, estética/performance manual) — este design segue o mesmo padrão, não inventa um novo.

---

## Arquitetura

- **Nível novo:** `Content/Maps/WorldStreamingTest` (nome de nível dedicado a esta feature — não reaproveita nenhum nível de teste de batalha existente).
- **World Partition:** habilitado na criação do nível (opção do Editor), tamanho de célula/raio configurados via `WorldPartitionEditorSettings`/`WorldPartitionRuntimeHashSettings` conforme DP-streaming-02.
- **`UDebugRouteMoverComponent`** (novo, `BattleSquare` — não é lógica de batalha, mas fica no mesmo módulo de gameplay por não haver módulo de "mundo" ainda; revisar se um módulo `BattleWorld` nasce quando a próxima feature de M5 chegar):
  - `TArray<FVector> Waypoints`, `float SpeedUnitsPerSecond`
  - `TickComponent` avança a posição do `Owner` em linha reta até o waypoint atual, avança o índice ao chegar perto o bastante (tolerância nomeada), para no último
  - Zero dependência de `BattleSim` — este componente nunca participa de resolução de combate
- **HLOD:** camada de HLOD única (`HLODLayer` padrão da engine) atribuída às malhas de placeholder, build via `WorldPartitionHLODsBuilder` (commandlet/Editor) — não runtime dinâmico.

## O que NÃO muda

- `BattleSim`/`BattleSquare` (M1–M4): nenhuma linha tocada. `ABattleArena` continua sendo spawnada sob demanda, sem relação com este nível — a integração "batalha nasce de um encontro no mundo" é a PRÓXIMA feature de M5, não esta.
- Nenhuma sonda de arquitetura (`audit_determinism.sh`, `audit_no_recalculation.sh`, `probe_isolation.sh`) precisa mudar — nenhuma delas varre `Content/` ou toca em World Partition; todas continuam válidas e devem continuar passando.

---

## Tarefas de alto nível (detalhe em tasks.md)

1. `UDebugRouteMoverComponent` — C++ puro, testável headless (determinismo de rota).
2. Nível `WorldStreamingTest` com World Partition habilitado, célula/raio conforme DP-streaming-02 — trabalho de Editor (via `unreal-mcp`).
3. Conteúdo de placeholder (grid de cubos, material por célula) — trabalho de Editor.
4. Pawn de debug com `UDebugRouteMoverComponent` anexado, waypoints definidos cobrindo a rota de referência — trabalho de Editor + dados.
5. HLOD layer atribuída e build rodado — trabalho de Editor.
6. Roteiro de verificação manual (`docs/verification/streaming-de-mundo.md`) cobrindo os itens não-automatizáveis de DP-streaming-05.
7. Regressão: `Automation RunTests BattleSquare+BattleSim` continua 127/127; as três sondas continuam limpas.
