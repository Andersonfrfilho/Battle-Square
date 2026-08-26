# Arenas Variadas — Tarefas

**Design:** `.specs/features/arenas-variadas/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** `BattleSim` (T1–T5) **e** `BattleSquare` (T6–T7). Primeira vez desde Combate Núcleo que uma feature de M3 muda o núcleo — auditar com `Tools/probe_isolation.sh`/`audit_determinism.sh` continua obrigatório em toda tarefa de `BattleSim`.

---

## Plano de Execução

### Fase 1 — Layout no estado (sequencial)
> 🤖 Modelo: `sonnet`

```
T1 → T2
```

### Fase 2 — Propriedades na resolução (depende da Fase 1)
> 🤖 Modelo: `sonnet` — **T3 e T4 são 🧠**

```
T3 → T4 → T5
```

### Fase 3 — Autoria de arena, camada BattleSquare (depende da Fase 2)
> 🤖 Modelo: `sonnet`

```
T6 → T7
```

### Fase 4 — Verificação (depende da Fase 3)
> 🤖 Modelo: `sonnet` para T8 · `haiku` para T9

```
T8 → T9
```

---

## Tarefas

### T1: `ECellProperty` e `CellLayout` em `FBattleState`
**O quê:** enum `None`/`Blocked`/`Damage`/`Buff`; `TArray<uint8> CellLayout` (9 posições, índice `Row*3+Column`), default todas `None` (arena neutra = comportamento de hoje).
**Onde:** `Source/BattleSim/Public/Battle/BattleTypes.h`, `Source/BattleSim/Public/Battle/BattleState.h`
**Depende de:** nada
**Requisito:** DP-arena-01

**Pronto quando:**
- [ ] `FBattleState` recém-construído tem `CellLayout` com 9 entradas, todas `None`
- [ ] Compila, `BATTLESIM_API` onde necessário (lição de L-016/L-019 — checar ANTES de outro módulo precisar)

**Verificar:** build verde
**Commit:** `feat(battlesim): propriedade de casa e layout no estado da batalha`

---

### T2: `ComputeHash()` inclui o layout
**O quê:** o hash de estado passa a mudar quando o layout muda, mesmo com os mesmos pets.
**Onde:** `Source/BattleSim/Private/Battle/BattleState.cpp`
**Depende de:** T1
**Requisito:** DP-arena-01, design.md ("Riscos" — dessincronia)

**Pronto quando:**
- [ ] Dois `FBattleState` idênticos exceto pelo `CellLayout` produzem hashes diferentes
- [ ] Dois `FBattleState` com o MESMO layout continuam produzindo o mesmo hash (zero regressão do hash existente)

**Verificar:** `Automation RunTests BattleSim.State.ComputeHashIncludesCellLayout`
**Commit:** `feat(battlesim): hash de estado inclui o layout de casas`

---

### T3: Bloqueio de movimento por propriedade de casa 🧠
**O quê:** `ApplyMovement` trata casa `Blocked` como destino inválido, mesmo caminho de "fora da grade" — reaproveita `MovimentoBloqueado`.
**Onde:** `Source/BattleSim/Private/Battle/BattlePhaseMovement.cpp`
**Depende de:** T1
**Requisito:** ARENA-01, ARENA-03

**Por que 🧠:** fácil checar o layout na posição ERRADA (origem em vez de destino), ou esquecer que colisão entre aliados (passo 3 do algoritmo já existente) precisa continuar funcionando quando um dos destinos concorrentes é bloqueado.

**Pronto quando:**
- [ ] Pet tentando mover para casa bloqueada nunca sai da posição original, em N tentativas repetidas
- [ ] Casa bloqueada some do cálculo de colisão entre aliados (não é um "destino válido" disputável)
- [ ] Nenhum evento novo — só `MovimentoBloqueado`, já existente
- [ ] Arena com `CellLayout` todo `None` produz resultado IDÊNTICO ao comportamento de hoje (zero regressão)

**Verificar:** `Automation RunTests BattleSim.Movement.BlockedCellRejectsMovement`
**Commit:** `feat(battlesim): casa bloqueada rejeita movimento`

---

### T4: Dano de casa acumula em `PendingDamage` 🧠
**O quê:** ao fim de cada slot (dentro de `ApplyMovement`, depois dos movimentos aplicados), pet vivo numa casa `Damage` soma `CellDamageAmount` ao `PendingDamage` — sem evento próprio, F5 já cobre.
**Onde:** `Source/BattleSim/Private/Battle/BattlePhaseMovement.cpp`, nova constante em `BattleSim` (mesmo padrão de `MinDamage`)
**Depende de:** T3
**Requisito:** ARENA-04, ARENA-05, edge case "morte simultânea"

**Por que 🧠:** morte simultânea (dano de casa + combate no mesmo slot) é fácil de testar incompleto — um teste que só cobre dano de casa sozinho não prova que a soma com combate funciona.

**Pronto quando:**
- [ ] Pet parado numa casa de dano, sem nenhum ataque do oponente, perde vida ao longo de N turnos
- [ ] Dano de casa mata um pet → `PetMorreu` emitido pelo caminho já existente, zero mudança em F5
- [ ] Pet que leva dano de casa E de combate no mesmo slot morre pela SOMA dos dois, aplicados juntos (prova da garantia de morte simultânea estendida)
- [ ] Arena sem casa de dano produz resultado idêntico ao de hoje

**Verificar:** `Automation RunTests BattleSim.Movement.DamageCellAccumulatesPendingDamage` + `BattleSim.Movement.DamageCellCombinesWithCombatDamageForSimultaneousDeath`
**Commit:** `feat(battlesim): casa de dano fere quem permanece nela`

---

### T5: Buff contextual em `ComputeDamage` 🧠
**O quê:** casa `Buff` fortalece `Attack` de quem ataca a partir dela, `Defense` de quem é alvo estando nela — nunca persiste em `FPetState`, nunca soma com o slot seguinte se o pet sair da casa.
**Onde:** `Source/BattleSim/Private/Battle/BattlePhaseCombat.cpp`
**Depende de:** T1
**Requisito:** ARENA-06, ARENA-07

**Por que 🧠:** combinar o multiplicador de buff com o de "Defendendo" na mesma fórmula é fácil de inverter a ordem dos percentuais ou aplicar duas vezes.

**Pronto quando:**
- [ ] Mesmo ataque, atacante numa casa de buff, produz dano MAIOR que o mesmo ataque fora dela (atributos idênticos)
- [ ] Mesmo ataque, alvo numa casa de buff, produz dano MENOR que o mesmo ataque contra o alvo fora dela
- [ ] Buff de Defense + postura "Defendendo" combinados não dobram nem se anulam — fórmula testada com os dois juntos
- [ ] Pet que sai da casa de buff no slot seguinte não carrega nenhum resíduo do bônus
- [ ] Arena sem casa de buff produz resultado idêntico ao de hoje

**Verificar:** `Automation RunTests BattleSim.Combat.BuffCellStrengthensAttackAndDefenseContextually`
**Commit:** `feat(battlesim): casa de buff fortalece contextualmente`

---

### T6: `FArenaLayoutCatalog` — nomeação e carga de arena
**O quê:** `LoadFromJson`/`GetLayoutByName` — mesmo padrão de `FTypeEffectivenessTable`. `BattleSim` nunca importa `Json`.
**Onde:** `Source/BattleSquare/Public/Balance/ArenaLayoutCatalog.h` + `.cpp`
**Depende de:** T5
**Requisito:** ARENA-08, DP-arena-04

**Pronto quando:**
- [ ] Arquivo JSON válido carrega layouts nomeados, cada um um `TArray<uint8>` de 9 posições
- [ ] Arquivo ausente/malformado retorna `false`, mesmo padrão de `FPetDataLoader`/`FTypeEffectivenessTable`
- [ ] `GetLayoutByName` de um nome inexistente retorna `false`, sem crash

**Verificar:** `Automation RunTests BattleSquare.Balance.ArenaLayoutCatalog.LoadAndLookup`
**Commit:** `feat(battlesquare): catálogo de layouts de arena nomeados`

---

### T7: Fiação com montagem de partida — validação de posição inicial
**O quê:** `ABattleArena::BeginBattle`/caminho de montagem recebe o layout; validação explícita rejeita montagem que posicionaria um pet numa casa bloqueada (edge case da spec).
**Onde:** `Source/BattleSquare/Private/Battle/BattleArena.cpp` (extensão), ponto de montagem em `ABattleSquareGameMode::HandleRoomReady`
**Depende de:** T6
**Requisito:** ARENA-02, edge case "montagem em casa bloqueada"

**Pronto quando:**
- [ ] `FBattleState` montado com um layout carrega `CellLayout` corretamente
- [ ] Tentativa de montar partida com um pet posicionado numa casa bloqueada falha explicitamente (log alto e claro), nunca reposiciona silenciosamente
- [ ] `ABattleArena.FullTurnEndToEnd` (já existente) continua passando sem modificação com layout neutro (zero regressão)

**Verificar:** `Automation RunTests BattleSquare.BattleArena.RejectsInitialPositionOnBlockedCell`
**Commit:** `feat(battlesquare): validação de posição inicial contra casa bloqueada`

---

### T8: Regressão completa [P]
**O quê:** `BattleSim` + `BattleSquare` inteiros, `audit_determinism.sh`, `probe_isolation.sh`.
**Onde:** n/a — verificação
**Depende de:** T7
**Requisito:** Success Criteria da spec

**Pronto quando:**
- [ ] `Automation RunTests BattleSim` — Success == total (44 + novos), Fail == 0
- [ ] `Automation RunTests BattleSquare` — idem
- [ ] `./Tools/audit_determinism.sh` e `./Tools/probe_isolation.sh` — `exit 0`

**Verificar:** rodar tudo, ler o log
**Commit:** (nenhum — verificação; correção vira commit próprio se algo falhar)

---

### T9: Roteiro de verificação — arena "parece justa" jogando de verdade [P]
**O quê:** documento curto, mesmo padrão de `escala-pets-skills-balanceamento.md`.
**Onde:** `docs/verification/arenas-variadas-jogabilidade.md`
**Depende de:** T8
> 🤖 `haiku`

**Pronto quando:**
- [ ] Lista o item ❌ (percepção humana de arena legível/justa)
- [ ] Passo concreto: jogar partidas manuais em cada tipo de arena, comparar com `RunBatchSimulation` (Escala de Pets e Skills, reaproveitada) rodando pets idênticos em arenas diferentes
- [ ] "não verificado ainda" até alguém rodar

**Verificar:** revisão humana
**Commit:** `docs: roteiro de verificação de jogabilidade de arenas variadas`

---

## Cobertura de Requisitos

8 requisitos na spec · **8 mapeados**.

| ID | Tarefa(s) |
|---|---|
| ARENA-01 | T3 |
| ARENA-02 | T7 |
| ARENA-03 | T3 (satisfeito por construção — reaproveita `MovimentoBloqueado`) |
| ARENA-04 | T4 |
| ARENA-05 | T4 |
| ARENA-06 | T5 |
| ARENA-07 | T5 |
| ARENA-08 | T6 |
