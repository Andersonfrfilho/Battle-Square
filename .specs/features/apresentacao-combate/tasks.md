# Apresentação do Combate — Tarefas

**Design:** `.specs/features/apresentacao-combate/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`. Componentes de lógica (T1–T10) são verificáveis headless, mesmo padrão das 53 tarefas anteriores. Componentes visuais (T11–T12) entregam a classe C++ base; o layout em si (UMG Designer) fica fora do que este lote verifica automaticamente — registrado explicitamente, não escondido.

---

## Plano de Execução

### Fase 1 — Fila de ação (sequencial)
> 🤖 Modelo: `sonnet`

```
T1 → T2 → T3 → T4
```

### Fase 2 — Oponente placeholder (paralelo à Fase 1, depende só de tipos do núcleo)
> 🤖 Modelo: `sonnet`

```
T5
```

### Fase 3 — Tocador de trace (sequencial)
> 🤖 Modelo: `sonnet` — **T6 é 🧠** (agrupar por fase erra fácil em caso de borda)

```
T6 → T7
```

### Fase 4 — Componentes de cena (sequencial, depende de T1–T7)
> 🤖 Modelo: `sonnet`

```
T8 → T9 → T10
```

### Fase 5 — Widgets, classe base C++ (paralelo após Fase 4)
> 🤖 Modelo: `sonnet`

```
T10 ──┬→ T11 [P]
      └→ T12 [P]
```

### Fase 6 — Verificação
> 🤖 Modelo: `sonnet` para T13 (script) · `haiku` para T14 (roteiro, mecânico)

```
T12 ──┬→ T13 [P]
      └→ T14 [P]
```

---

## Tarefas

### T1: Tipos da seleção pendente
**O quê:** `EActionSelectionStep`, `FPendingActionSelection`.
**Onde:** `Source/BattleSquare/Public/Battle/BattleActionSelection.h`
**Depende de:** nada
**Requisito:** PRES-01
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] `EActionSelectionStep` tem `ChoosingType` e `ChoosingDirection`
- [ ] `FPendingActionSelection` guarda passo atual e tipo escolhido
- [ ] Compila

**Verificar:** build verde
**Commit:** `feat(battlesquare): tipos de seleção de ação pendente`

---

### T2: `UBattleActionQueueComponent` — passo 1 (tipo)
**O quê:** `BeginSelectingType` — confirma direto se o tipo não precisa de direção, senão avança para o passo 2.
**Onde:** `Source/BattleSquare/Public/Battle/BattleActionQueueComponent.h` + `.cpp`
**Depende de:** T1
**Requisito:** PRES-01, PRES-03

**Pronto quando:**
- [ ] `Defender`/`Esquivar`/`Aguardar` confirmam a ação imediatamente, sem passar por `ChoosingDirection`
- [ ] `Mover`/`Atacar`/`Magia` avançam para `ChoosingDirection`, sem adicionar à fila ainda
- [ ] Com fila em 3/3, `BeginSelectingType` recusa e não muda o estado

**Verificar:** `Automation RunTests BattleSquare.ActionQueue.BeginSelectingType`
**Commit:** `feat(battlesquare): passo 1 da seleção de ação`

---

### T3: `UBattleActionQueueComponent` — passo 2 (direção) e cancelamento
**O quê:** `ConfirmDirection`, `CancelPendingSelection`.
**Onde:** mesmo arquivo de T2
**Depende de:** T2
**Requisito:** PRES-01, PRES-02

**Pronto quando:**
- [ ] `ConfirmDirection` fora do passo `ChoosingDirection` é ignorado (sem crash, sem efeito)
- [ ] `ConfirmDirection` válido adiciona `(Tipo, Direção)` à fila e volta para `ChoosingType`
- [ ] `CancelPendingSelection` no passo 2 volta para o passo 1 **sem alterar as ações já confirmadas**

**Verificar:** `Automation RunTests BattleSquare.ActionQueue.ConfirmAndCancel`
**Commit:** `feat(battlesquare): passo 2 e cancelamento da seleção de ação`

---

### T4: `UBattleActionQueueComponent` — commit e remoção
**O quê:** `RemoveLastAction`, `Commit`, `BuildCommit`.
**Onde:** mesmo arquivo
**Depende de:** T3
**Requisito:** BTL-01 (critério 3), PRES-05, edge case de preenchimento automático

**Pronto quando:**
- [ ] `RemoveLastAction` desfaz só a última, preserva as anteriores
- [ ] `RemoveLastAction` numa fila vazia não crasha
- [ ] `Commit` com fila incompleta preenche o resto com `Aguardar` antes de travar
- [ ] Após `Commit`, `BeginSelectingType`/`RemoveLastAction` não têm mais efeito (fila travada)
- [ ] `BuildCommit()` produz um `FTurnCommit` válido, consumível direto por `FBattleResolver::ResolveTurn`

**Verificar:** `Automation RunTests BattleSquare.ActionQueue.CommitAndRemove` — inclui um teste que alimenta o `FTurnCommit` construído no resolvedor real e confirma que ele resolve sem erro
**Commit:** `feat(battlesquare): commit e remoção da fila de ação`

---

### T5: `UDumbOpponentAI`
**O quê:** gera um `FTurnCommit` válido usando o RNG do próprio `FBattleState`.
**Onde:** `Source/BattleSquare/Public/Battle/DumbOpponentAI.h` + `.cpp`
**Depende de:** nada (paralelo à Fase 1)
**Requisito:** design.md — infraestrutura de demonstração

**Pronto quando:**
- [ ] Nunca gera `Mover` para fora da grade (verifica antes de escolher a direção)
- [ ] Usa exclusivamente `FBattleRandom` — auditável pela sonda anti-float/anti-rand já existente (`Tools/audit_determinism.sh`, estendida para cobrir este arquivo)
- [ ] Duas chamadas com o mesmo estado e mesma seed produzem o mesmo commit (determinismo herdado do RNG)

**Verificar:** `Automation RunTests BattleSquare.DumbOpponentAI` + `./Tools/audit_determinism.sh` continua limpo
**Commit:** `feat(battlesquare): oponente placeholder com ação válida aleatória`

---

### T6: Agrupamento de eventos por fase 🧠
**O quê:** `GroupEventsByPhase` — função pura que agrupa o trace linear por `SlotIndex`+`Phase`.
**Onde:** `Source/BattleSquare/Public/Battle/BattleTracePlayer.h` (função) + `.cpp`
**Depende de:** nada (paralelo às Fases 1–2)
**Requisito:** PRES-09 (critério 2 do P7 do núcleo)

**Por que 🧠:** casos de borda de agrupamento (evento de fronteira de turno sem slot, grupo de 1 evento só, ordem estável dentro do grupo) são fáceis de errar sutilmente.

**Pronto quando:**
- [ ] Eventos do mesmo `SlotIndex`+`Phase` caem no mesmo grupo
- [ ] Eventos de fases diferentes do mesmo slot ficam em grupos diferentes, na ordem em que apareceram no trace
- [ ] `TurnoIniciado`/`TurnoEncerrado`/`BatalhaEncerrada` (sem `SlotIndex` significativo) formam grupo próprio, sem quebrar o agrupamento dos demais
- [ ] Ordem relativa dentro de um grupo é preservada (estável, não reordena)
- [ ] Não modifica o `FBattleEvent` nem o `TArray` de entrada

**Verificar:** `Automation RunTests BattleSquare.TracePlayer.GroupEventsByPhase` — usa um trace real gerado por `FBattleResolver::ResolveTurn` (não um trace inventado à mão)
**Commit:** `feat(battlesquare): agrupamento de eventos do trace por fase`

---

### T7: `UBattleTracePlayer` — orquestração
**O quê:** `PlayTrace`, `SkipToEnd` — consome os grupos de T6, despacha para `APetView` (via delegate, já que `APetView` ainda não existe neste ponto — usar interface mínima).
**Onde:** mesmo arquivo de T6
**Depende de:** T6
**Requisito:** PRES-09, PRES-10, BTL-22

**Pronto quando:**
- [ ] `PlayTrace` processa os grupos em ordem
- [ ] `SkipToEnd` aplica o estado final sem exigir que grupos anteriores tenham "tocado"
- [ ] Nenhum cálculo de dano/alcance/resultado no arquivo — só leitura de campos do evento

**Verificar:** `Automation RunTests BattleSquare.TracePlayer.PlayAndSkip`; grep manual confirmando ausência de operador aritmético sobre `Attack`/`Defense` no arquivo
**Commit:** `feat(battlesquare): orquestração do tocador de trace`

---

### T8: `APetView` — lógica de estado (sem UMG)
**O quê:** `SetInitialState`, `ApplyEvent` — reage a eventos do trace, expõe `HealthRatio` e posição para consumo visual.
**Onde:** `Source/BattleSquare/Public/Battle/PetView.h` + `.cpp`
**Depende de:** nada (paralelo às Fases 1–3)
**Requisito:** PRES-11, PRES-12, PRES-13

**Pronto quando:**
- [ ] `SetInitialState` posiciona o pet e define vida cheia a partir de `FPetState`+`FPetPresentationInfo`
- [ ] `ApplyEvent(DanoAplicado)` reduz `HealthRatio` proporcionalmente ao `Value` do evento — nunca recalcula o dano
- [ ] `ApplyEvent(PetMorreu)` marca o pet como derrotado (flag booleano exposto, não decide visual)
- [ ] `ApplyEvent(Moveu)` atualiza a posição a partir de `ToCell` (desempacotado), não de um cálculo próprio

**Verificar:** `Automation RunTests BattleSquare.PetView.AppliesEventsFromRealTrace` — usa trace real de uma resolução completa
**Commit:** `feat(battlesquare): lógica de estado visual do pet`

---

### T9: `ABattleArena` — scaffold
**O quê:** câmera fixa (FOV estreito, post-process tilt-shift só nesta cena — DP-09), grade visual 3x3, spawn de `APetView` a partir de `FBattleState` inicial.
**Onde:** `Source/BattleSquare/Public/Battle/BattleArena.h` + `.cpp`
**Depende de:** T8
**Requisito:** PRES-06, PRES-07, PRES-08

**Pronto quando:**
- [ ] Compila e um `ABattleArena` instanciado em nível de teste spawna sem crash
- [ ] Câmera posicionada de forma que as 9 casas fiquem dentro do frustum (checável programaticamente: projetar os 9 centros de casa e confirmar que caem dentro da viewport)
- [ ] Grade usa material próprio (não default), respeitando token de cor (nenhum hex hardcoded — mesma regra do `web.md` §8, aplicada aqui)

**Verificar:** teste automatizado para a checagem de frustum; **layout/estética final exigem inspeção visual no editor — não coberto por este teste, registrado no roteiro de T14**
**Commit:** `feat(battlesquare): scaffold da arena com câmera e grade`

---

### T10: Fiação completa — jogador, IA, resolvedor, apresentação
**O quê:** `ABattleArena` orquestra `UBattleActionQueueComponent` (jogador) + `UDumbOpponentAI` (oponente) + `FBattleResolver::ResolveTurn` + `UBattleTracePlayer`, de ponta a ponta.
**Onde:** `Source/BattleSquare/Private/Battle/BattleArena.cpp`
**Depende de:** T4, T5, T7, T9
**Requisito:** todos os P1 da spec, de ponta a ponta

**Pronto quando:**
- [ ] Jogador monta e commita 3 ações
- [ ] IA gera commit automaticamente ao detectar o commit do jogador
- [ ] `ResolveTurn` roda com os dois commits reais
- [ ] Trace resultante é passado para `UBattleTracePlayer`, que atualiza os `APetView`
- [ ] Um turno completo, de ponta a ponta, sem intervenção manual além da seleção inicial de ações

**Verificar:** `Automation RunTests BattleSquare.BattleArena.FullTurnEndToEnd` — simula seleção de ações via `UBattleActionQueueComponent` diretamente (sem UI visual), confirma que o ciclo completo roda e produz um `FBattleState` consistente
**Commit:** `feat(battlesquare): fiação completa de um turno de combate`

---

### T11: `UBattleActionSelectorWidget` — classe base C++ [P]
**O quê:** `UUserWidget` com `UFUNCTION(BlueprintCallable)` para cada ação de `UBattleActionQueueComponent`, `UPROPERTY(BlueprintReadOnly)` para o estado atual (passo, tipo escolhido, contagem 3/3).
**Onde:** `Source/BattleSquare/Public/UI/BattleActionSelectorWidget.h` + `.cpp`
**Depende de:** T10
**Requisito:** PRES-01 a PRES-04 (camada de exposição — o layout em si é T14)

**Pronto quando:**
- [ ] Toda operação de `UBattleActionQueueComponent` tem uma função `BlueprintCallable` correspondente
- [ ] Estado da fila (passo, 3/3) exposto como `BlueprintReadOnly`, atualizado via os delegates de T2–T4
- [ ] Compila; nenhum layout UMG é criado por esta tarefa — fica para autoria visual (DP-08)

**Verificar:** build verde; **não há teste automatizado de UI aqui, por design (ver Limite de Ferramenta no design.md)**
**Commit:** `feat(battlesquare): classe base C++ do seletor de ação`

---

### T12: `UBattleResultWidget` — classe base C++ [P]
**O quê:** `UUserWidget` que expõe o resultado final (`WinningSide`, ou empate) lido do evento `BatalhaEncerrada`.
**Onde:** `Source/BattleSquare/Public/UI/BattleResultWidget.h` + `.cpp`
**Depende de:** T10
**Requisito:** PRES-14

**Pronto quando:**
- [ ] `UPROPERTY(BlueprintReadOnly)` para vitória/derrota/empate, preenchido a partir do `Value` do evento `BatalhaEncerrada` — nunca de um recálculo
- [ ] Compila

**Verificar:** build verde; teste automatizado confirma que o widget lê corretamente os 3 casos (vitória, derrota, empate) de um `FBattleEvent` construído à mão com cada `Value`
**Commit:** `feat(battlesquare): classe base C++ da tela de resultado`

---

### T13: Sonda "nenhum recálculo" [P]
**O quê:** script que falha se aparecer fórmula de dano/alcance fora de `BattleSim` (mesmo espírito do `audit_determinism.sh`).
**Onde:** `Tools/audit_no_recalculation.sh`
**Depende de:** T12
**Requisito:** BTL-22, critério de sucesso da spec

**Pronto quando:**
- [ ] Detecta padrões como `Attack *`, `- Defense`, `* 1.5` fora de `Source/BattleSim`
- [ ] Passa no código atual de `BattleSquare`
- [ ] Falha se alguém plantar uma fórmula de dano dentro de um widget (testado plantando uma, igual ao padrão de `probe_isolation.sh`)

**Verificar:** `./Tools/audit_no_recalculation.sh; echo $?` → `0`; plantar fórmula, rodar, esperar `1`; remover, confirmar `0` de novo
**Commit:** `chore(battlesquare): sonda anti-recálculo na apresentação`

---

### T14: Roteiro de verificação visual [P]
**O quê:** documento listando o que só se prova olhando no editor — câmera enquadrando as 9 casas de verdade, material fosco aplicado, layout dos 6 botões de tipo + roseta/D-pad de direção, alvo de toque ≥44pt em mobile, tilt-shift aplicado só na arena.
**Onde:** `docs/verification/apresentacao-combate-visual.md`
**Depende de:** T9, T11
> 🤖 `haiku` — é um checklist, não lógica

**Pronto quando:**
- [ ] Lista os itens da spec que são P1 e não têm teste automatizado (PRES-04, PRES-06 estética, PRES-07 estética, PRES-08, layout de DP-08)
- [ ] Cada item tem um passo concreto de como verificar no editor (não "olhar e ver se está bom")
- [ ] Documento explicitamente diz "não verificado ainda" até alguém (você ou eu via MCP) rodar o roteiro

**Verificar:** revisão humana do próprio roteiro — é um documento, não código
**Commit:** `docs: roteiro de verificação visual da apresentação de combate`

---

## Checagem de Granularidade

| Tarefa | Escopo | Situação |
|---|---|---|
| T1 | 1 header | ✅ |
| T2–T4 | 1 componente, 3 fatias de comportamento | ✅ |
| T5 | 1 classe | ✅ |
| T6–T7 | 1 arquivo, 2 responsabilidades relacionadas | ✅ |
| T8 | 1 classe | ✅ |
| T9–T10 | 1 ator, scaffold depois fiação | ✅ |
| T11–T12 | 1 widget cada | ✅ |
| T13–T14 | 1 script / 1 documento | ✅ |

---

## Cobertura de Requisitos

15 requisitos na spec · **13 mapeados** para tarefas de lógica testável.

**Parcialmente fora do que este lote verifica automaticamente:**

| ID | Requisito | Motivo |
|---|---|---|
| PRES-04 | Alvo de toque ≥44pt em mobile | Propriedade de layout UMG — T14, verificação manual |
| PRES-06/07/08 (estética) | Câmera "bonita", grade "legível", material fosco de verdade | O código (frustum, uso de token) é testável (T9); a leitura estética final é humana — T14 |

Nenhum requisito foi silenciosamente ignorado — os que não têm teste automatizado têm passo explícito no roteiro de T14.

---

## Pergunta antes de executar

**MCP da Unreal:** T9–T14 tocam em conteúdo visual. As tarefas em si (C++ + testes headless) não precisam do editor aberto. A **verificação** de T14 precisa — e ali é onde a skill `unreal-mcp` entra, se você quiser que eu tente a autoria/inspeção visual assistida em vez de fazer manualmente.

**Ferramentas por tarefa:** nenhuma tarefa de lógica exige MCP ou skill externa — é C++ novo sobre uma base já estável (núcleo + backend). `context7` fica disponível se alguma API de UMG/Slate específica precisar de confirmação em vez de memória.
