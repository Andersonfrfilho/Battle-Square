# Interface de Batalha — Design

**Spec:** `.specs/features/interface-de-batalha/spec.md`
**Status:** Draft — aguarda aprovação

---

## DP-ui-01: A tela não decide nada

**Decisão:** todo botão chama um método de `UBattleActionSelectorWidget`, que encaminha para `UBattleActionQueueComponent`. A tela **nunca** avalia se uma ação é válida, se cabe mais uma, ou se o tipo pede direção.

**Razão:** essa regra já existe, está no componente e tem teste. Duplicá-la na tela criaria uma segunda fonte de verdade que diverge no primeiro ajuste de balanceamento — é a mesma armadilha de **L-032** (duas camadas validando a mesma regra, a de fora escondendo a de baixo), que este projeto já pagou uma vez.

**Como a tela sabe o que mostrar:** pelo estado espelhado que o widget já expõe (`CurrentStep`, `ConfirmedActionCount`, `bIsCommitted`), atualizado por delegate. Ler, nunca recalcular.

## DP-ui-02: D-pad 3x3 em vez de roseta

**Decisão:** as 8 direções viram uma grade 3x3 com o centro vazio (ou ocupado por "cancelar").

**Razão:** DP-08 deixou as duas opções abertas e registrou que a grade é mais simples de autorar. Autorando por código a diferença é maior ainda: uma grade é posição calculada, uma roseta é geometria. E a spec de M1 já garantiu que **a interface do componente não muda entre as duas** — trocar depois é trabalho de layout, não de fiação.

**Custo aceito e nomeado:** a grade é menos elegante e pior para arrastar com o dedo. Quando M6 destravar e o toque importar de verdade, a roseta volta à mesa.

## DP-ui-03: Autoria por código, com o layout em C++ e o visual no Blueprint

**Decisão:** o Widget Blueprint (`WBP_BattleActionSelector`) é criado programaticamente, com uma árvore de widgets simples: um painel vertical com (a) o contador de ações, (b) a linha dos 6 tipos, (c) a grade 3x3 de direções, (d) a linha de commit/desfazer.

**Razão honesta:** o projeto nunca teve autoria visual, e esperar por ela foi o que manteve o jogo injogável por quatro marcos. Um layout funcional agora vale mais que um layout bonito depois — e ele é substituível sem tocar em código, que é justamente o que DP-ui-01 garante.

## DP-ui-04: A tela de batalha é um nível próprio, e ele é descartável

**Decisão:** nível `BattleScreen`, com um GameMode que monta uma partida e mostra a interface. Sem World Partition, sem streaming, sem mundo.

**Razão:** é o que permite jogar o combate **hoje**, sem depender do mundo aberto — que é a prioridade que o usuário declarou. E por ser um nível separado, ele não atrapalha `WorldStreamingTest`: os dois caminhos de entrada convivem, e o de mundo já funciona (M5 provou: arena spawnada, dois pets, `BeginBattle`).

**O que se reusa, e é a maior parte:** a montagem de partida (`FEncounterMatchAssembler`), `ABattleArena`, `UBattleTurnCoordinator`, `UBattleTracePlayer`, `FPetDataLoader`. Nada disso é reescrito.

## DP-ui-05: O que é testável, e o que não é

| Verificação | Automatizável? | Como |
|---|---|---|
| A tela encaminha cada botão ao método certo do componente | ✅ Sim | Teste headless sobre `UBattleActionSelectorWidget`, sem UMG |
| O estado espelhado acompanha o componente (passo, contagem, commit) | ✅ Sim | Idem |
| Não aceitar 4ª ação, bloquear depois do commit | ✅ Sim | Já é regra do componente, com teste próprio; aqui se testa o espelho |
| O layout aparece, os botões são clicáveis, o texto cabe | ❌ Não | Roteiro manual — é julgamento visual, como PRES-06/07 |
| "Jogar é gostoso" | ❌ Não | Só jogando. É o ponto do B-001 que nunca foi respondido |

---

## O que muda

- `Content/UI/WBP_BattleActionSelector` (novo) — layout, derivado da classe C++ existente
- `Content/Maps/BattleScreen` (novo) — nível de batalha sem mundo
- `ABattleScreenGameMode` (novo, `BattleSquare/UI/` ou `Net/`) — monta a partida e mostra a interface
- Testes headless novos para o widget

## O que NÃO muda

- **`BattleSim`:** nenhuma linha. A tela não conhece o núcleo.
- **`UBattleActionQueueComponent`:** nenhuma regra. Se a tela pedir uma mudança nele, o design está errado.
- **`WorldStreamingTest` e a corrente de M5:** intocados. Os dois caminhos de entrada para a batalha coexistem.
