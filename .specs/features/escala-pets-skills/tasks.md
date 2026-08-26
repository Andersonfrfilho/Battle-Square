# Escala de Pets e Skills — Tarefas

**Design:** `.specs/features/escala-pets-skills/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`. `BattleSim` não é tocado em nenhuma tarefa.

---

## Plano de Execução

### Fase 1 — Tabela de efetividade (sequencial)
> 🤖 Modelo: `sonnet`

```
T1 → T2
```

### Fase 2 — Tradução com efetividade (depende da Fase 1)
> 🤖 Modelo: `sonnet` — **T3 é 🧠**

```
T3
```

### Fase 3 — Ferramenta de simulação em lote (depende da Fase 2)
> 🤖 Modelo: `sonnet`

```
T4 → T5
```

### Fase 4 — Verificação (depende da Fase 3)
> 🤖 Modelo: `sonnet` para T6 · `haiku` para T7

```
T6 → T7
```

---

## Tarefas

### T1: `FTypeEffectivenessTable` — estrutura e consulta
**O quê:** `GetPercent(AttackerType, DefenderType)` — 100 (neutro) se o par não existir na tabela.
**Onde:** `Source/BattleSquare/Public/Balance/TypeEffectivenessTable.h` + `.cpp`
**Depende de:** nada
**Requisito:** ESCALA-01, ESCALA-02, edge case "par ausente é neutro"

**Pronto quando:**
- [ ] Par presente retorna o percentual cadastrado
- [ ] Par ausente retorna 100 (neutro), nunca erro/crash
- [ ] Mesmo tipo contra si mesmo, sem entrada explícita, retorna 100 (neutro por padrão — edge case da spec)
- [ ] Função pura, sem I/O, testável construindo a tabela em memória direto no teste

**Verificar:** `Automation RunTests BattleSquare.Balance.TypeEffectivenessTable.GetPercent`
**Commit:** `feat(battlesquare): tabela de efetividade de tipo, consulta pura`

---

### T2: `FTypeEffectivenessTable::LoadFromJson`
**O quê:** carrega a tabela de `Config/TypeEffectiveness.json` via `FJsonSerializer`. Falha explícita (retorna `false`) se arquivo ausente ou malformado — nunca tabela vazia silenciosa.
**Onde:** mesmo arquivo de T1
**Depende de:** T1
**Requisito:** ESCALA-06, design.md (Riscos — JSON malformado)

**Pronto quando:**
- [ ] Arquivo válido carrega todos os pares corretamente
- [ ] Arquivo ausente retorna `false` (mesmo padrão de `FPetDataLoader::LoadVerifiedPets`)
- [ ] JSON malformado (sintaxe inválida) retorna `false`, não crasha
- [ ] Tipo novo adicionado ao JSON é reconhecido na próxima carga, sem qualquer mudança de código (prova viva de ESCALA-06)

**Verificar:** `Automation RunTests BattleSquare.Balance.TypeEffectivenessTable.LoadFromJson` — usa arquivos de fixture (válido, ausente, malformado), mesmo padrão de fixtures de `PetDataLoaderTest`
**Commit:** `feat(battlesquare): carregamento da tabela de efetividade a partir de JSON`

---

### T3: `FBattleDataTranslator::TranslateMatchup` 🧠
**O quê:** traduz os dois lados de uma partida simultaneamente, pré-multiplicando `Attack` de cada lado pela efetividade contra o tipo do oponente.
**Onde:** `Source/BattleSquare/Public/Data/BattleDataTranslator.h` + `.cpp` (extensão)
**Depende de:** T2
**Requisito:** ESCALA-01, ESCALA-02, ESCALA-03, ESCALA-05

**Por que 🧠:** fácil inverter atacante/defensor por engano (aplicar a efetividade do tipo do Left CONTRA o tipo do Left, em vez de contra o Right), ou aplicar o percentual no lado errado — o tipo de bug que passa em um teste simétrico (dois pets do mesmo tipo) e só aparece com tipos assimétricos.

**Pronto quando:**
- [ ] Left super efetivo contra Right → `Attack` efetivo do Left é MAIOR que o base; `Attack` do Right não muda por causa do tipo do Left (só pela efetividade do Right contra o Left, independente)
- [ ] Left resistido por Right → `Attack` efetivo do Left é MENOR que o base, nunca zero (arredondamento para baixo respeita `MinDamage` só na hora do dano, não aqui — `Attack` pode ficar baixo, o dano mínimo é regra do núcleo, inalterada)
- [ ] Dois tipos neutros entre si → `Attack` de ambos os lados idêntico ao comportamento de `TranslatePet` hoje (ESCALA-03, zero regressão)
- [ ] `Defense`/`Speed`/`MaxHealth`/`Health` NUNCA são alterados por tipo — só `Attack`
- [ ] `FPetState` resultante alimenta `FBattleResolver::ResolveTurn` real sem erro (ponta a ponta)

**Verificar:** `Automation RunTests BattleSquare.Data.TranslateMatchupAppliesTypeEffectiveness` — inclui um caso assimétrico (Left super efetivo, Right neutro contra Left) que pegaria o bug de inversão
**Commit:** `feat(battlesquare): tradução de partida com efetividade de tipo pré-aplicada`

---

### T4: `RunBatchSimulation`
**O quê:** roda N combates completos (`FDumbOpponentAI` dos dois lados, `FBattleResolver::ResolveTurn` real turno a turno até `bBattleEnded`), agrega vitórias/turnos/dano médio.
**Onde:** `Source/BattleSquare/Public/Balance/BattleBalanceSimulator.h` + `.cpp`
**Depende de:** T3
**Requisito:** ESCALA-07, ESCALA-08

**Pronto quando:**
- [ ] `LeftWins + RightWins + Draws == NumSimulations`, sempre
- [ ] Duas execuções com a mesma `BaseSeed` produzem `FBatchSimulationResult` byte a byte idêntico
- [ ] Duas execuções com seeds diferentes produzem resultados que variam (não travados no mesmo valor sempre)
- [ ] Cada simulação individual usa `FBattleRandom` semeado com `BaseSeed + Index` — nunca `FMath::Rand`, auditável pelo mesmo espírito de `audit_determinism.sh`

**Verificar:** `Automation RunTests BattleSquare.Balance.RunBatchSimulation.DeterministicBySeed`
**Commit:** `feat(battlesquare): simulação de combate em lote, determinística por seed`

---

### T5: Teste de Automation como ferramenta de medição
**O quê:** `BattleSquare.Balance.RunBatchSimulation.ReportsAggregateStats` — roda um lote real (ex.: 100 simulações) entre uma composição super-efetiva e uma neutra, loga o relatório agregado via `AddInfo`.
**Onde:** `Source/BattleSquare/Private/Tests/BattleBalanceSimulatorTest.cpp`
**Depende de:** T4
**Requisito:** ESCALA-07, Success Criteria ("taxas de vitória visivelmente diferentes")

**Pronto quando:**
- [ ] Composição com `Attack` super efetivo vence visivelmente mais que a composição neutra (não precisa ser 100%, precisa ser assimetricamente maior — limiar definido no teste, ex.: >60%)
- [ ] Relatório inclui taxa de vitória de cada lado, turnos médios, dano médio por turno
- [ ] Teste sempre "passa" no sentido de rodar sem crash — é instrumentação, e o teste falha explicitamente SE a composição super-efetiva não vencer mais (isso é o que prova que a feature funciona, não é ruído)

**Verificar:** `Automation RunTests BattleSquare.Balance.RunBatchSimulation.ReportsAggregateStats`
**Commit:** `test(battlesquare): prova de que tipo super efetivo vence mais em lote`

---

### T6: Regressão completa
**O quê:** rodar `BattleSquare` + `BattleSim` inteiros, e as duas sondas (`audit_determinism.sh`, `audit_no_recalculation.sh`).
**Onde:** n/a — verificação
**Depende de:** T5
**Requisito:** Success Criteria da spec

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` — Success == total, Fail == 0
- [ ] `Automation RunTests BattleSim` (44 testes) — continua limpo, zero mudança
- [ ] `./Tools/audit_determinism.sh` e `./Tools/audit_no_recalculation.sh` — ambos `exit 0`

**Verificar:** rodar os quatro, ler o log
**Commit:** (nenhum — task de verificação)

---

### T7: Roteiro de verificação — "sensação" de tipo jogando de verdade [P]
**O quê:** documento curto listando o item ❌ do design (efetividade "parecer certa" jogando, não só nos números agregados).
**Onde:** `docs/verification/escala-pets-skills-balanceamento.md`
**Depende de:** T6
> 🤖 `haiku` — checklist, não lógica

**Pronto quando:**
- [ ] Lista o item não coberto por teste automatizado (percepção humana de "essa vitória fez sentido tacticamente")
- [ ] Passo concreto: jogar N partidas manuais com composições conhecidas, comparar com o relatório de `RunBatchSimulation`
- [ ] Documento diz "não verificado ainda" até alguém rodar

**Verificar:** revisão humana do próprio roteiro
**Commit:** `docs: roteiro de verificação de sensação de balanceamento`

---

## Cobertura de Requisitos

10 requisitos na spec · **10 mapeados** (ESCALA-04 é propriedade estrutural do design — auditabilidade — verificada pelas sondas existentes em T6, não uma tarefa própria; ESCALA-09 é satisfeita pela T5/T6 rodarem via `Automation RunTests`, mesmo mecanismo headless de sempre; ESCALA-10, P2, fica fora deste primeiro lote — relatório com destaque automático de desbalanceamento é melhoria sobre o relatório cru que T5 já produz).
