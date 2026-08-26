# Roteiro de Verificação Manual — Traversal e Câmera de Mundo

**Feature:** `.specs/features/traversal-camera-mundo/`
**Nível:** `Content/Maps/WorldStreamingTest` (`/Game/Maps/WorldStreamingTest`)
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado no editor.

Cobre só o que `design.md` (DP-trav-07) marca como não automatizável: colisão e
gravidade reais, câmera não entrando na parede, streaming acompanhando o
personagem, e o julgamento humano de conforto. A parte que é decisão nossa —
converter input + orientação de câmera em direção de movimento — já está
coberta por `Automation RunTests BattleSquare.World.WorldTraversalMotion` e
**não** se repete aqui.

---

## Preparação

1. Abrir o Editor e carregar `/Game/Maps/WorldStreamingTest`.
2. Confirmar no Outliner:
   - `StreamingFloor/` com 25 pisos (um por célula, topo em `z = 0`);
   - `StreamingDebug/` com `DebugRoutePawn` **e** `WorldExplorer`;
   - `StreamingEncounters/` com os 3 atores de encontro.
3. **Assets de input — já existem.** `/Game/Input/IMC_Traversal`, `IA_Move`
   (WASD, com swizzle/negate para bater com `FWorldTraversalMotion`) e
   `IA_Look` (Mouse2D) foram criados e **já estão atribuídos** ao
   `WorldExplorer`. Se o personagem não andar, o problema não é asset faltando
   — é outra coisa, e vale investigar em vez de recriar os assets.

4. **Posse:** o `DebugRoutePawn` está com `Auto Possess Player = Player 0` (é o
   que os roteiros anteriores usam). Para verificar traversal, trocar a posse
   para o `WorldExplorer` — e **devolver ao `DebugRoutePawn` no fim**, senão os
   roteiros de `streaming-de-mundo.md` e `encontros-transicao-batalha.md`
   param de reproduzir.

---

## TRT-01 — Andar com colisão e gravidade

- [ ] **Não verificado**

**Passo:** dar Play in Editor com o `WorldExplorer` possuído. Andar em todas as
direções e caminhar contra um dos cubos de placeholder.

**Critério, em quatro partes:**
1. O personagem **fica apoiado no piso** — não afunda e não flutua.
2. Ele **não atravessa** o cubo: para ao encostar.
3. Soltar as teclas **para** o personagem, sem deriva.
4. Andar na diagonal **não é mais rápido** que andar reto. O teste headless já
   garante isso na matemática; aqui é só confirmar que nada no meio do caminho
   desfez.

---

## TRT-02 — Movimento relativo à câmera

- [ ] **Não verificado**

**Passo:** girar a câmera 90° e andar para frente.

**Critério:** o personagem vai **para onde a câmera olha**, não para um eixo
fixo do mundo. Repetir com a câmera olhando bem para baixo: o personagem
continua andando na horizontal, **sem afundar no chão** — é o descarte de pitch
de DP-trav-02 valendo em runtime.

---

## TRT-03 — A câmera não entra na parede

- [ ] **Não verificado**

**Passo:** encostar o personagem de costas num cubo de placeholder, de modo que
o cubo fique entre a câmera e ele.

**Critério:** a câmera **aproxima** (o braço encolhe) e o personagem continua
visível. Se a câmera entrar no cubo e a tela mostrar o interior da geometria,
`bDoCollisionTest` foi desligado em algum lugar — é regressão, e há teste
headless que deveria ter pego.

Girar também para cima e para baixo até o limite: o enquadramento **nunca**
inverte nem chega a olhar o topo da própria cabeça (limites em
`CameraPitchMinDegrees`/`CameraPitchMaxDegrees`).

---

## TRT-04 — Streaming acompanha o personagem

- [ ] **Não verificado**

**Passo:** atravessar a área de ponta a ponta andando, rodando `stat streaming`
e anotando as células carregadas em dois extremos:

| Posição | Células carregadas |
|---|---|
| Canto inicial, perto de `(-3400, -3400)` | ___ |
| Canto oposto, perto de `(3400, 3400)` | ___ |

**Critério:** o conjunto de células **muda** — o personagem de jogador é fonte
de streaming igual ao pawn de debug era. Se o número não mudar, a posse
provavelmente ficou no `DebugRoutePawn` (ver Preparação, item 4).

---

## TRT-05 — Encontro dispara andando

- [ ] **Não verificado**

**Passo:** andar até um dos `Encounter_Leg*`.

**Critério:** o encontro dispara pelo mesmo caminho da feature anterior.
**Ressalva herdada:** os `CatalogId` dos encontros são placeholders e não batem
com o espelho de pets — enquanto não forem trocados por ids reais, a montagem é
recusada de propósito e **nenhuma batalha começa**. Isso é o comportamento
testado, não um bug deste roteiro. Ver a preparação de
`encontros-transicao-batalha.md`.

---

## TRT-06 — Conforto (julgamento humano)

- [ ] **Não verificado**

**Passo:** andar por uns dois minutos sem tarefa específica.

**Critério:** é leitura humana, no mesmo espírito de PRES-06/07. Três perguntas
concretas, cada uma com resposta escrita:
1. A distância da câmera (`CameraArmLengthUnits`, hoje 450uu) deixa ver o
   suficiente à frente para decidir abordar um pet a tempo? ____________
2. A velocidade de giro do corpo (`540°/s`) parece responsiva ou brusca? ______
3. A câmera "chacoalha" ao encolher perto de geometria? ____________

Cada "não" aqui vira um número a ajustar nas constantes de
`WorldExplorerCharacter.h` — que é a razão de elas serem constantes nomeadas.
