# Interface de Batalha — Especificação

**Status:** Concluída e verificada por teste (status corrigido em 29/08/2026 — a linha dizia "Draft" enquanto a feature já rodava)
**Depende de:** M1–M4 (o combate inteiro já funciona por código). Paga a dívida que `apresentacao-combate` deixou explícita em **DP-08**.

---

## Problem Statement

O combate está inteiro, testado e funcionando — e **ninguém consegue jogá-lo**.

`UBattleActionQueueComponent` aceita 3 ações por turno, em dois passos (tipo → direção), com cancelamento e commit às cegas. `FBattleResolver` resolve os dois lados simultaneamente. XP e captura acontecem no fim. São 52 testes no núcleo e mais de 120 no jogo. Tudo isso roda **sem uma única tela**.

`UBattleActionSelectorWidget` existe como classe C++ e expõe a API certa para Blueprint (`BeginSelectingType`, `ConfirmDirection`, `Commit`, com o estado espelhado). Mas **DP-08 adiou o layout** para "a autoria visual", e essa autoria nunca aconteceu:

> *"Decisão adiada para a autoria visual (fora do que C++ resolve)."*

O resultado é o que se vê hoje ao jogar: a batalha começa, os pets nascem na arena, o `BattleSim` está pronto para receber o commit — e o jogador olha para uma tela sem nenhum botão.

**Decisão de prioridade do usuário (2026-08-26):** a batalha vem primeiro, e por enquanto se abre **por tela**, sem depender do mundo aberto. O mundo continua existindo e evoluindo em paralelo, mas não é pré-requisito para jogar.

## Goals

- [ ] O jogador escolhe as 3 ações de um turno pela tela, em dois passos (tipo, e direção quando o tipo pedir)
- [ ] O jogador vê quantas ações já confirmou e consegue desfazer a última antes de commitar
- [ ] O commit fecha o turno às cegas, e o resultado da rodada aparece
- [ ] Existe uma forma de **abrir uma batalha direto**, sem atravessar o mundo aberto
- [ ] Nenhuma regra de combate nova nasce aqui: a tela só chama o que `UBattleActionQueueComponent` já decide

## Out of Scope

| Item | Razão |
|---|---|
| Arte final (ícones, cores, tipografia de marca) | Layout funcional primeiro; identidade visual é trabalho de design, e trocar depois não muda a fiação |
| Animação de ataque, efeito de dano, câmera cinematográfica | `apresentacao-combate` já entregou o `UBattleTracePlayer` que anima o trace; polimento é outro escopo |
| Interface de toque adaptada a celular | M6 e bloqueado por B-006/B-007. O layout nasce clicável, e toque reusa os mesmos botões |
| Menu de jogo, seleção de time, tela de coleção | Cada um é uma tela própria. Esta feature entrega a tela de **batalha** e um jeito mínimo de chegar nela |
| Mudar qualquer regra de combate | O núcleo está fechado e testado. Se a tela precisar de uma regra nova, é sinal de que a tela está errada |

---

## User Stories

### P1: Escolher as três ações do turno ⭐ MVP

**User Story:** Como treinador, quero escolher as ações do meu pet a cada rodada clicando na tela, para poder de fato jogar o combate que já existe.

**Acceptance Criteria:**
1. WHEN o turno começa THEN a tela SHALL mostrar os 6 tipos de ação e quantas ações ainda cabem no turno
2. WHEN o jogador escolhe um tipo que exige direção (Mover, Atacar, Magia) THEN a tela SHALL pedir a direção antes de confirmar a ação
3. WHEN o jogador escolhe um tipo que NÃO exige direção (Aguardar, Defender, Esquivar) THEN a tela SHALL confirmar a ação direto, sem pedir direção
4. WHEN 3 ações estão confirmadas THEN a tela SHALL permitir commitar, e não aceitar uma quarta

**Independent Test:** a regra de quando pedir direção já é do `UBattleActionQueueComponent` e já tem teste. Aqui se verifica que a tela **reflete** o passo atual e a contagem, sem decidir nada por conta própria.

---

### P1: Desfazer antes de commitar ⭐ MVP

**User Story:** Como treinador, quero poder voltar atrás numa ação antes de fechar o turno, porque escolher às cegas já é difícil o bastante sem eu ficar preso a um clique errado.

**Acceptance Criteria:**
1. WHEN há ao menos uma ação confirmada e o turno não foi commitado THEN a tela SHALL permitir remover a última
2. WHEN o jogador está no passo de direção THEN a tela SHALL permitir cancelar e voltar à escolha de tipo
3. WHEN o turno foi commitado THEN a tela SHALL bloquear qualquer alteração

---

### P1: Abrir uma batalha por tela ⭐ MVP

**User Story:** Como quem está desenvolvendo o jogo, quero abrir uma batalha direto, sem atravessar o mundo aberto, para poder jogar e avaliar o combate sem depender de um mundo que ainda é placeholder.

**Acceptance Criteria:**
1. WHEN o jogo abre no nível de batalha THEN o sistema SHALL montar uma partida com dois pets do catálogo e mostrar a tela de ações
2. WHEN a batalha termina THEN o sistema SHALL mostrar o resultado e permitir jogar de novo

**Independent Test:** a montagem reusa `FEncounterMatchAssembler` (M5) ou o caminho de `ABattleSquareGameMode` — nenhum caminho de montagem novo.

---

### P2: Ver o que aconteceu na rodada

**User Story:** Como treinador, quero ver o resultado da rodada depois do commit, para entender por que perdi vida antes de escolher as próximas ações.

**Acceptance Criteria:**
1. WHEN o turno resolve THEN a tela SHALL exibir os eventos da rodada, usando o `UBattleTracePlayer` que já existe
2. WHEN a batalha termina THEN a tela SHALL dizer quem venceu

---

## Decisões Pendentes

**PROPOSTA** — mudam se você discordar.

1. **D-pad 3x3 para a direção, não roseta radial.** DP-08 deixou as duas em aberto e observou que a grade é mais simples de autorar. Como estou autorando por código, a grade é a que nasce correta e testável; a roseta pode substituí-la depois sem mudar nenhuma chamada.
2. **A tela de batalha é um nível próprio.** Um nível `BattleScreen` com a arena e a interface, aberto direto. É o que permite jogar sem o mundo, e é descartável quando o mundo assumir.
3. **Layout funcional, sem identidade visual.** Botões com rótulo de texto e posição clara. Trocar por arte depois não toca na fiação.
