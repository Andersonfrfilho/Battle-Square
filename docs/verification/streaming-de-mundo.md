# Roteiro de Verificação Manual — Streaming de Mundo

**Feature:** `.specs/features/streaming-de-mundo/`
**Nível:** `Content/Maps/WorldStreamingTest` (`/Game/Maps/WorldStreamingTest`)
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado no editor.

Este documento cobre exatamente os itens que `design.md` (DP-streaming-05)
marca como **não automatizáveis de forma confiável**: streaming real de
World Partition, ausência de tela de loading e ganho de HLOD. A lógica que
*é* automatizável — `UDebugRouteMoverComponent` percorrendo waypoints de
forma determinística — já está coberta por
`Automation RunTests BattleSquare.DebugRouteMover` e **não** se repete aqui.

Cada item tem um passo concreto e um número a ler — nunca "olhar e ver se
está bom". Marcar `[x]` só depois de executar o passo, anotando data e quem
verificou.

---

## Preparação (comum a todos os itens)

1. Abrir o Editor no projeto `BattleSquare.uproject`.
2. Abrir o nível `/Game/Maps/WorldStreamingTest`.
3. Confirmar no painel **World Partition** que a partição `MainGrid` está com
   **Cell Size = 1600** e **Loading Range = 3200** (ver DP-streaming-02a — o
   valor original de 512uu do design é inalcançável: a engine força um piso
   de 1600uu em `URuntimePartitionLHGrid::PostEditChangeProperty`).
4. Confirmar no Outliner que existem:
   - a pasta `StreamingPlaceholder/` com 25 subpastas `R{0..4}C{0..4}`,
     9 cubos em cada (225 no total, uma cor por célula);
   - o ator `DebugRoutePawn` na pasta `StreamingDebug/`, com
     `Auto Possess Player = Player 0` e um `DebugRouteMover` cujos
     `Waypoints` são `(3600,-3600,300)`, `(3600,3600,300)`, `(-3600,3600,300)`.

A rota completa mede 21.600uu a 400 uu/s → **54 segundos** ponta a ponta.

---

## STREAM-01 — Células carregam e descarregam ao longo da rota

- [ ] **Não verificado**

**Passo:** dar Play in Editor. O `DebugRoutePawn` é possuído automaticamente
e vira a fonte de streaming; o `DebugRouteMover` o leva pela rota sem input
humano. No console, rodar `stat streaming` e anotar o número de **níveis /
células carregadas** em três momentos:

| Momento | Posição aproximada | Células carregadas |
|---|---|---|
| t = 0s (início) | `(-3600, -3600)` | ___ |
| t ≈ 27s (canto oposto) | `(3600, 3600)` | ___ |
| t ≈ 54s (fim da rota) | `(-3600, 3600)` | ___ |

**Critério:** o conjunto de células carregadas **muda** entre os três momentos
— não basta o número ser igual, as células precisam ser outras. A leitura
direta disso é o comando `wp.Runtime.ToggleDrawRuntimeHash2D`, que desenha a
grade e destaca as células carregadas: confirmar visualmente que a mancha
carregada acompanha o pawn e que as células deixadas para trás apagam.

**Critério quantitativo:** com raio de 3200uu e célula de 1600uu, o esperado é
uma vizinhança de 5x5 células ao redor do pawn quando ele está no meio da
área, e menos nas bordas (não há mundo além delas).

---

## STREAM-02 — Nenhuma tela de loading durante a rota

- [ ] **Não verificado**

**Passo:** na mesma sessão de PIE de STREAM-01, assistir aos 54 segundos
inteiros sem tocar em nada. Confirmar que **em nenhum momento** aparece tela
de carregamento, fade preto ou congelamento de tela — o mundo entra e sai de
memória com o jogo rodando.

**Critério:** zero interrupções visuais. Se houver engasgo (hitch), rodar
`stat unit` junto e anotar o pico de `Frame` em ms no momento do engasgo:
______ ms. Hitch acima de ~100ms é um achado a registrar em `STATE.md`, não
um sucesso.

---

## STREAM-03 — HLOD representa regiões distantes

- [ ] **Não verificado**

**Passo:** com o build de HLOD já rodado (T5), no viewport do Editor ativar
o modo de visualização de HLOD (`Show → Advanced → HLOD Clusters`, ou o
console `wp.Runtime.ToggleDrawRuntimeHash2D` em PIE para ver quais células
estão em HLOD). Posicionar a câmera num canto da área e confirmar que as
células além do raio de 3200uu aparecem como representação de HLOD
(`HLOD_StreamingPlaceholder`, tipo *Instancing*) em vez da geometria completa.

**Critério:** existe ao menos uma região visível servida por ator
`WorldPartitionHLOD` enquanto o pawn está no canto oposto.

---

## STREAM-04 — Custo com e sem HLOD

- [ ] **Não verificado**

**Passo:** rodar a rota duas vezes em PIE, anotando `stat unit` e `stat gpu`
no ponto médio (t ≈ 27s, pawn em `(3600, 3600)`):

| Cenário | `Frame` (ms) | `GPU` (ms) | Draw calls (`stat rhi`) |
|---|---|---|---|
| HLOD habilitado | ___ | ___ | ___ |
| HLOD desabilitado (`wp.Runtime.HLOD 0`) | ___ | ___ | ___ |

**Critério:** este é um **teste de medição, não de aprovação**. O objetivo é
ter o número registrado. Com apenas 225 cubos a diferença pode ser
desprezível ou até negativa — se for, isso é um dado honesto sobre o tamanho
do cenário de teste, não uma falha do HLOD, e deve ser anotado como tal.

---

## Orçamento de memória (DP-streaming-01)

- [ ] **Não verificado**

**Passo:** em PIE, no ponto médio da rota, rodar `stat memory` e anotar o
uso total do processo: ______ MB.

**Critério:** o alvo de partida de DP-streaming-01 é 512 MB. Como aquele
número é declaradamente uma decisão de partida e não uma medição (não há
hardware de referência definido — ver `STATE.md`), o resultado aqui serve
para *calibrar* o alvo, não para reprovar a feature.
