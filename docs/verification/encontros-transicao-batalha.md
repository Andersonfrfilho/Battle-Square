# Roteiro de Verificação Manual — Encontros e Transição para Batalha

**Feature:** `.specs/features/encontros-transicao-batalha/`
**Nível:** `Content/Maps/WorldStreamingTest` (`/Game/Maps/WorldStreamingTest`)
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado no editor.

Este documento cobre só o que `design.md` (DP-enc-05) marca como **não
automatizável de forma confiável**: a ausência de tela de carregamento na
transição, e a permanência do mundo carregado enquanto a batalha corre. Tudo o
que é regra — disparo do encontro, montagem com os `CatalogId` certos,
restauração exata da posição, não disparar um segundo encontro na volta — já
está coberto por `Automation RunTests BattleSquare.World` e **não** se repete
aqui.

Cada item tem um passo concreto e um número a ler. Marcar `[x]` só depois de
executar, anotando data e quem verificou.

---

## Preparação

1. Abrir o Editor no projeto `BattleSquare.uproject` e carregar
   `/Game/Maps/WorldStreamingTest`.
2. Confirmar no Outliner a pasta `StreamingEncounters/` com os
   `AWorldEncounterActor` posicionados sobre a rota do `DebugRoutePawn`
   (a rota vai de `(-3600,-3600)` a `(3600,-3600)`, daí a `(3600,3600)` e
   termina em `(-3600,3600)`).
3. Confirmar que o `DebugRoutePawn` tem, além do `DebugRouteMover`, um
   `UEncounterDetectionComponent`.
4. **Pré-requisito de dados:** o espelho de pets precisa estar configurado com
   `CatalogId` que batam com os dos atores de encontro do nível. Sem isso a
   montagem é recusada de propósito (é o comportamento testado em
   `EncounterMatchAssembler.RejectsUnknownCatalogId`), a detecção religa e
   **nenhuma batalha começa** — o que parece "não funcionou" e na verdade é a
   recusa explícita fazendo o trabalho dela. Conferir antes de reportar bug.

---

## ENC-01 — A transição mundo → batalha não tem tela de carregamento

- [ ] **Não verificado**

**Passo:** dar Play in Editor e deixar o `DebugRoutePawn` percorrer a rota até
alcançar o primeiro encontro. No instante em que a batalha começa, observar a
tela.

**Critério:** nenhuma tela de carregamento, fade preto ou congelamento. A arena
aparece no mesmo frame ou no seguinte, porque ela nasce no **mesmo `UWorld`**,
deslocada de 1.000.000uu (DP-enc-03) — não há troca de nível.

**Se houver engasgo:** rodar `stat unit` junto e anotar o pico de `Frame` no
momento da transição: ______ ms.

---

## ENC-02 — O mundo continua carregado durante a batalha

- [ ] **Não verificado**

**Passo:** com a batalha ativa (depois de ENC-01), rodar `stat streaming` no
console e anotar o número de células carregadas: ______.

**Critério:** o número é **o mesmo** que estava carregado no mundo um instante
antes do encontro. É isto que torna a volta instantânea: nada foi descarregado,
então nada precisa recarregar. Se o número cair para perto de zero, a arena
provavelmente está dentro do raio de streaming e puxou a fonte de streaming
junto — o deslocamento precisaria ser revisto.

---

## ENC-03 — A volta é instantânea e no lugar certo

- [ ] **Não verificado**

**Passo:** deixar a batalha terminar (qualquer resultado). Observar a volta ao
mundo.

**Critério, em três partes:**
1. Nenhuma tela de carregamento na volta.
2. O pawn está **exatamente** onde estava — a mesma posição e a mesma direção.
   O teste headless já garante isso numericamente; o que se confere aqui é que
   a câmera não "salta" de um jeito desconfortável.
3. O pawn está parado em cima (ou ao lado) do pet que acabou de derrotar e
   **nenhuma segunda batalha começa**. Se começar, o bug é a ordem de
   DP-enc-03, e há um teste dedicado que deveria ter pegado — reportar como
   regressão, não como comportamento esperado.

---

## ENC-04 — O resultado da batalha valeu

- [ ] **Não verificado**

**Passo:** antes de encostar no encontro, anotar o tamanho da coleção e o nível
do pet do jogador. Depois de vencer a batalha e voltar ao mundo, conferir de
novo.

| Momento | Pets na coleção | Nível do pet do jogador |
|---|---|---|
| Antes do encontro | ___ | ___ |
| Depois da batalha | ___ | ___ |

**Critério:** captura e XP já valem **na volta ao mundo**, sem nenhuma ação
extra. Isto não é código novo desta feature — é `FPetCollectionService` e
`FPetProgressionService` rodando por dentro de `ABattleArena` (DP-enc-04). O
que este item verifica é que a transição não os atropelou ao destruir a arena.
