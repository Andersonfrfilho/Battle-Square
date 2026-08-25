# Combate Núcleo — Tarefas

**Design:** `.specs/features/combate-nucleo/design.md`
**Status:** ✅ Completo e verificado — 44/44 testes automatizados Success, 0 falhas (2026-08-25)
**Escopo:** apenas o módulo `BattleSim`. Apresentação e camada de dados viram feature própria, porque exigem editor e assets. Tudo aqui compila e é testável **sem abrir a Unreal**.

---

## Comandos de referência

```bash
# Compilar
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
  BattleSquareEditor Mac Development \
  -Project="$PWD/BattleSquare.uproject" -waitmutex

# Rodar os testes do núcleo, headless
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "$PWD/BattleSquare.uproject" \
  -ExecCmds="Automation RunTests BattleSim; Quit" \
  -unattended -nopause -nosplash -log
```

---

## Plano de Execução

### Fase 1 — Fundação (sequencial)
> 🤖 Modelo: `sonnet` — estruturas bem especificadas, sem decisão de arquitetura em aberto

```
T1 → T2 → T3 → T4
```

### Fase 2 — Fases da resolução (paralelo após a Fase 1)
> 🤖 Modelo: `sonnet` — **T7 é 🧠**, validar com `opus` antes

```
        ┌→ T5 [P] ─┐
T4 ─────┼→ T6 [P] ─┼──→ T9
        ├→ T7 [P]🧠┤
        └→ T8 [P] ─┘
```

### Fase 3 — Orquestração (sequencial)
> 🤖 Modelo: `sonnet` — **T9 é 🧠**

```
T9 → T10
```

### Fase 4 — Verificação (paralelo)
> 🤖 Modelo: `sonnet` para T11 e T12; `haiku` para T13 e T14 (mecânicos)

```
T10 ──┬→ T11 [P]
      ├→ T12 [P]
      ├→ T13 [P]
      └→ T14 [P]
```

---

## Tarefas

### T1: Tipos base da ação e da grade
**O quê:** enums `EActionType` e `EBattleDirection`, structs `FBattleAction` e `FTurnCommit`, helpers de empacotamento de célula.
**Onde:** `Source/BattleSim/Public/Battle/BattleTypes.h`
**Depende de:** nada
**Requisito:** BTL-01, e AD-009
**Ferramentas:** MCP `context7` (convenções de `USTRUCT`/`UENUM` na 5.8) · Skill `unreal-best-practices`

**Pronto quando:**
- [ ] `EActionType` tem os 6 tipos e `EBattleDirection` as 8 direções mais `Nenhuma`
- [ ] `FTurnCommit` fixa `ActionsPerTurn = 3` como `constexpr`
- [ ] `FBattleAction` ocupa **2 bytes**, verificado por `static_assert(sizeof(FBattleAction) == 2)`
- [ ] Compila

**Verificar:** build verde; o `static_assert` é a prova do custo de rede do design.
**Commit:** `feat(battlesim): tipos base de ação e direção`

---

### T2: Gerador pseudoaleatório determinístico
**O quê:** `FBattleRandom` com estado explícito de 64 bits, `NextUInt32()` e `NextRange()` sem viés de módulo.
**Onde:** `Source/BattleSim/Public/Battle/BattleRandom.h` + `Private/BattleRandom.cpp`
**Depende de:** T1
**Requisito:** BTL-16, BTL-18
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Nenhuma chamada a `FMath::Rand`, `FRandomStream` ou qualquer estado global
- [ ] `NextRange` é inclusivo nos dois extremos e **sem viés de módulo** (rejeição, não `%`)
- [ ] Teste: mesma seed, 1.000 chamadas, sequência idêntica entre execuções
- [ ] Teste: `NextRange(0,5)` em 100.000 amostras não desvia mais de 2% do uniforme

**Verificar:** `Automation RunTests BattleSim.Random`
**Commit:** `feat(battlesim): PRNG determinístico com estado explícito`

---

### T3: Estado da batalha e do pet
**O quê:** `FPetState`, `FBattleState` e `uint64 ComputeHash()` para detecção de dessincronia.
**Onde:** `Source/BattleSim/Public/Battle/BattleState.h` + `Private/BattleState.cpp`
**Depende de:** T2
**Requisito:** BTL-03, BTL-15, BTL-17
**Ferramentas:** Skill `unreal-best-practices`

**Pronto quando:**
- [ ] `FPetState` tem `MaxHealth` separado de `Health`
- [ ] `PostureFlags` é bitmask, não bool solto
- [ ] `FBattleState` carrega o `FBattleRandom` **dentro** de si
- [ ] `ComputeHash()` é estável e não depende de ordem de contêiner
- [ ] Varredura confirma **zero** `float`/`double` no arquivo

**Verificar:** `grep -nE '\b(float|double)\b' Source/BattleSim/Public/Battle/BattleState.h` → sem resultado
**Commit:** `feat(battlesim): estado de batalha com hash de dessincronia`

---

### T4: Evento de trace
**O quê:** `EBattleEventType` (14 valores) e o struct POD `FBattleEvent`.
**Onde:** `Source/BattleSim/Public/Battle/BattleEvent.h`
**Depende de:** T3
**Requisito:** BTL-08, BTL-22
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Struct plano: sem ponteiro, sem virtual, sem `FString`, sem `TArray` interno
- [ ] `static_assert` fixando o tamanho do struct
- [ ] `TargetId == 0xFF` documentado como "não se aplica"

**Verificar:** build verde
**Commit:** `feat(battlesim): formato do trace de eventos`

---

### T5: Fase F2 — Postura [P]
**O quê:** função livre que aplica `Defender` e `Esquivar` no `PostureFlags`.
**Onde:** `Source/BattleSim/Private/Battle/PhasePosture.cpp`
**Depende de:** T4
**Requisito:** BTL-09, BTL-11, BTL-12
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Emite `PosturaAssumida` para cada pet que assumiu postura
- [ ] Não toca em posição nem em vida
- [ ] Teste: postura de um slot **não vaza** para o seguinte

**Verificar:** `Automation RunTests BattleSim.Phase.Posture`
**Commit:** `feat(battlesim): fase de postura`

---

### T6: Fase F3 — Movimento [P]
**O quê:** resolução simultânea de movimento, com limites da grade e colisão entre aliados.
**Onde:** `Source/BattleSim/Private/Battle/PhaseMovement.cpp`
**Depende de:** T4
**Requisito:** BTL-04, BTL-05, BTL-06
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Movimento para fora da grade é anulado e emite `MovimentoBloqueado`
- [ ] Dois aliados para a mesma casa: **ambos** anulados
- [ ] Lados opostos coabitam (DP-02, comportamento padrão)
- [ ] Troca de casas entre dois pets é permitida, sem colisão
- [ ] Nenhuma intenção é resolvida antes de todas serem lidas

**Verificar:** `Automation RunTests BattleSim.Phase.Movement`
**Commit:** `feat(battlesim): fase de movimento simultâneo`

---

### T7: Fase F4 — Combate 🧠 [P]
**O quê:** ataque e magia direcionais sobre as posições pós-movimento, com dano **acumulado, não aplicado**.
**Onde:** `Source/BattleSim/Private/Battle/PhaseCombat.cpp`
**Depende de:** T4
**Requisito:** BTL-07, BTL-09, BTL-10, BTL-13
**Ferramentas:** nenhuma

**Por que 🧠:** é a tarefa com mais sutileza do lote. Direção, alcance, o triângulo ataque/defesa/esquiva e o acúmulo de dano se cruzam aqui, e é onde um erro produz combate injusto em vez de crash.

**Pronto quando:**
- [ ] Ataque atinge a primeira casa na direção, mais a própria casa se houver oponente coabitando
- [ ] `Esquivar` anula ataque físico, **mas não magia**
- [ ] `Defender` reduz todo dano, magia inclusive
- [ ] Dano é escrito num acumulador — **nenhuma vida muda nesta fase**
- [ ] Dano mínimo garantido; nunca zero nem negativo
- [ ] Ataque em direção sem alvo emite `AtaqueErrou`
- [ ] Fórmula usa só inteiros, multiplicador em percentual

**Verificar:** `Automation RunTests BattleSim.Phase.Combat`
**Commit:** `feat(battlesim): fase de combate direcional`

---

### T8: Fase F5 — Encerramento [P]
**O quê:** aplica o dano acumulado de uma vez, verifica mortes, expira posturas.
**Onde:** `Source/BattleSim/Private/Battle/PhaseResolution.cpp`
**Depende de:** T4
**Requisito:** BTL-07, BTL-12
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Todo o dano acumulado é aplicado **antes** de qualquer checagem de morte
- [ ] Dois pets que se matam no mesmo slot **morrem os dois**
- [ ] `PostureFlags` zerado ao fim
- [ ] Emite `DanoAplicado`, `PetMorreu` e `SlotEncerrado` nessa ordem

**Verificar:** `Automation RunTests BattleSim.Phase.Resolution`
**Commit:** `feat(battlesim): fase de encerramento de slot`

---

### T9: Orquestração do turno 🧠
**O quê:** `FBattleResolver::ResolveTurn` — laço de 3 slots chamando as cinco fases em ordem.
**Onde:** `Source/BattleSim/Public/Battle/BattleResolver.h` + `Private/BattleResolver.cpp`
**Depende de:** T5, T6, T7, T8
**Requisito:** BTL-03, BTL-08, BTL-17
**Ferramentas:** nenhuma

**Por que 🧠:** é onde o determinismo se ganha ou se perde. O desempate e a ordem de iteração vivem aqui.

**Pronto quando:**
- [ ] Função **estática e pura** — sem membro, sem singleton, sem estado interno
- [ ] Desempate por velocidade desc, depois `PetId` asc — **nunca** ordem de contêiner
- [ ] Pet morto tem as ações restantes descartadas
- [ ] Commit com menos de 3 ações é completado com `Aguardar`
- [ ] Devolve estado final **e** trace completo

**Verificar:** `Automation RunTests BattleSim.Resolver`
**Commit:** `feat(battlesim): orquestração do turno em 3 slots`

---

### T10: Condição de fim de batalha
**O quê:** vitória, derrota, empate e limite de turnos com desempate por percentual de vida.
**Onde:** `Source/BattleSim/Private/Battle/BattleOutcome.cpp`
**Depende de:** T9
**Requisito:** BTL-14, BTL-15
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Os dois lados zerando no mesmo F5 → empate
- [ ] Três slots com ambos vivos → novo turno, **não** fim de batalha
- [ ] Limite de turnos → vence o maior percentual de vida; igual → empate
- [ ] Emite `BatalhaEncerrada` **uma única vez**

**Verificar:** `Automation RunTests BattleSim.Outcome`
**Commit:** `feat(battlesim): condições de fim de batalha`

---

### T11: Teste de snapshot de trace [P]
**O quê:** entradas fixas → trace gravado → comparação byte a byte.
**Onde:** `Source/BattleSim/Private/Tests/TraceSnapshotTest.cpp`
**Depende de:** T10
**Requisito:** BTL-16
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Ao menos 5 cenários gravados, incluindo morte mútua e commit por timeout
- [ ] Rodar duas vezes produz hashes idênticos
- [ ] Falha com mensagem que aponta o **primeiro evento divergente**, não só "diferente"

**Verificar:** `Automation RunTests BattleSim.Snapshot` duas vezes seguidas
**Commit:** `test(battlesim): snapshot de trace determinístico`

---

### T12: Matriz 5×5 de tipos de ação [P]
**O quê:** todo tipo contra todo tipo, resultado tabelado.
**Onde:** `Source/BattleSim/Private/Tests/ActionMatrixTest.cpp`
**Depende de:** T10
**Requisito:** BTL-09 a BTL-13
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] 25 combinações cobertas
- [ ] O triângulo se confirma: Ataque fura Defesa por atrito, Magia fura Esquiva, Esquiva anula Ataque
- [ ] Tabela de esperados no topo do arquivo, legível sem rodar

**Verificar:** `Automation RunTests BattleSim.ActionMatrix`
**Commit:** `test(battlesim): matriz de interação entre ações`

---

### T13: Auditoria anti-ponto-flutuante [P]
**O quê:** script que falha o build se aparecer `float`, `double`, `FMath::Rand` ou `FRandomStream` no `BattleSim`.
**Onde:** `Tools/audit_determinism.sh`
**Depende de:** T10
**Requisito:** BTL-18
**Ferramentas:** nenhuma
> 🤖 `haiku` — mecânico

**Pronto quando:**
- [ ] Sai com código diferente de zero ao achar qualquer ocorrência
- [ ] Ignora comentário e string literal
- [ ] Passa no código atual

**Verificar:** `./Tools/audit_determinism.sh; echo $?` → `0`; plantar um `float`, rodar, esperar `1`
**Commit:** `chore(battlesim): auditoria de determinismo`

---

### T14: Sonda de isolação do núcleo [P]
**O quê:** automatizar o experimento do AD-012 — um build que precisa **falhar**.
**Onde:** `Tools/probe_isolation.sh`
**Depende de:** T10
**Requisito:** AD-011, AD-012
**Ferramentas:** nenhuma
> 🤖 `haiku` — mecânico

**Pronto quando:**
- [ ] Planta um `.cpp` referenciando `AActor::StaticClass()` no `BattleSim`
- [ ] Roda o build e espera **falha**
- [ ] Remove a sonda **mesmo se o script for interrompido** (`trap`)
- [ ] Sai `0` se o build falhou (correto) e `1` se compilou (fronteira furada)

**Verificar:** `./Tools/probe_isolation.sh; echo $?` → `0`
**Commit:** `chore(battlesim): sonda automatizada de isolação do núcleo`

---

## Checagem de Granularidade

| Tarefa | Escopo | Situação |
|---|---|---|
| T1–T4 | 1 header cada | ✅ |
| T5–T8 | 1 fase, 1 arquivo cada | ✅ |
| T9 | 1 função pública | ✅ |
| T10 | 1 arquivo | ✅ |
| T11–T14 | 1 teste ou 1 script cada | ✅ |

---

## Cobertura de Requisitos

22 requisitos na spec · **19 mapeados** para tarefas.

**Não mapeados, e por quê:**

| ID | Requisito | Motivo |
|---|---|---|
| BTL-02 | Commit trava a fila e esconde do oponente | Precisa de UI e rede — feature de apresentação |
| BTL-19 a BTL-21 | Contrato de rede, timeout, reconexão | Marco M2 |
| BTL-22 | Apresentação anima o trace | Feature de apresentação |

Nenhum requisito ficou órfão por esquecimento — os quatro de fora são de features que ainda não foram especificadas.

---

## Pergunta antes de executar

**Ferramentas por tarefa:** a maioria não precisa de nada — é C++ puro num projeto novo, sem código a reusar. As exceções estão marcadas: `context7` no T1 (convenções de `USTRUCT` na 5.8, onde meu conhecimento para em 5.7) e a skill `unreal-best-practices` no T1 e T3.

**MCP da Unreal não é usado em nenhuma tarefa deste lote** — nada aqui cria asset. O editor pode ficar fechado do T1 ao T14.
