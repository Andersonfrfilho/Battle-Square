# Níveis, Experiência e Evolução — Tarefas

**Design:** `.specs/features/niveis-experiencia-evolucao/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`. `BattleSim` não é tocado.

---

## Plano de Execução

### Fase 1 — Serviço de progressão (sequencial)
> 🤖 Modelo: `sonnet` — **T2 é 🧠**

```
T1 → T2 → T3
```

### Fase 2 — Fiação (depende da Fase 1)
> 🤖 Modelo: `sonnet`

```
T4 → T5
```

### Fase 3 — Verificação (depende da Fase 2)
> 🤖 Modelo: `sonnet`

```
T6
```

---

## Tarefas

### T1: `FPetProgressionService::ExperienceRequiredForLevel`/`GetLevel`
**O quê:** derivação de nível a partir de XP acumulado — nunca um campo `Level` separado.
**Onde:** `Source/BattleSquare/Public/Meta/PetProgressionService.h` + `.cpp`
**Depende de:** nada
**Requisito:** DP-nivel-02, DP-nivel-03

**Pronto quando:**
- [ ] Nível 1 corresponde a 0 XP acumulado
- [ ] `GetLevel` deriva corretamente para XP exatamente no limiar de um nível, e para XP entre dois limiares
- [ ] `GetLevel` nunca retorna acima de `MaxLevel`, mesmo com XP muito além do necessário

**Verificar:** `Automation RunTests BattleSquare.Meta.PetProgressionService.LevelDerivedFromExperience`
**Commit:** `feat(battlesquare): nível derivado de XP acumulado`

---

### T2: `FPetProgressionService::GrantExperience` 🧠
**O quê:** credita XP a uma instância; processa múltiplos níveis de uma vez; nunca ultrapassa o teto.
**Onde:** mesmo arquivo de T1
**Depende de:** T1
**Requisito:** NIVEL-05, NIVEL-06, NIVEL-07

**Por que 🧠:** off-by-one é fácil aqui — conceder XP suficiente para pular exatamente no limiar, ou muito além do teto, são os casos que costumam quebrar implementações ingênuas de "subir de nível".

**Pronto quando:**
- [ ] XP suficiente para 1 nível sobe exatamente 1 nível
- [ ] XP suficiente para 3 níveis de uma vez sobe exatamente 3, não 1
- [ ] XP muito além do necessário para o teto nunca resulta em nível > `MaxLevel`
- [ ] `Experience` no `FOwnedPetInstance` continua acumulando mesmo depois do teto (spec.md, NIVEL-07 — "continua aceitando XP")

**Verificar:** `Automation RunTests BattleSquare.Meta.PetProgressionService.GrantExperienceProcessesMultipleLevels`
**Commit:** `feat(battlesquare): concessão de XP processa múltiplos níveis sem passar do teto`

---

### T3: `FPetProgressionService::ApplyLevelBonus`
**O quê:** bônus percentual de atributo por nível, aplicado a um `FPetState` já montado.
**Onde:** mesmo arquivo de T1
**Depende de:** T1
**Requisito:** NIVEL-08, NIVEL-09

**Pronto quando:**
- [ ] Nível 1 não altera nenhum atributo (zero regressão)
- [ ] Nível > 1 aumenta `Attack`/`Defense`/`Speed`/`MaxHealth` proporcionalmente, aritmética inteira
- [ ] Nível máximo (`MaxLevel`) produz o bônus máximo esperado, sem overflow nem arredondamento incorreto

**Verificar:** `Automation RunTests BattleSquare.Meta.PetProgressionService.ApplyLevelBonusScalesAttributes`
**Commit:** `feat(battlesquare): bônus de atributo por nível, aplicado na montagem`

---

### T4: `ABattleArena::GrantExperienceIfOwned`
**O quê:** ao fim de batalha, credita XP à instância do jogador local (se já capturada) — quantidade diferente por vitória/derrota/empate.
**Onde:** `Source/BattleSquare/Private/Battle/BattleArena.cpp` (extensão, ao lado de `CheckForCapture`)
**Depende de:** T2
**Requisito:** NIVEL-01, NIVEL-02, NIVEL-03, NIVEL-04, edge case "os dois lados são do mesmo jogador"

**Pronto quando:**
- [ ] Vitória do jogador local credita `ExperienceForWin` à instância do PRÓPRIO lado (Side == LocalPlayerSide), se `CatalogId` já capturado
- [ ] Derrota credita `ExperienceForLoss`; empate credita `ExperienceForDraw` — sempre à instância do jogador local, nunca do oponente
- [ ] Pet do jogador local ainda não capturado não gera XP fantasma — nenhuma instância nova criada por este caminho (só `CheckForCapture` cria instâncias, e só para o pet do OPONENTE)
- [ ] Numa mesma vitória, captura do oponente (se novo) e XP do próprio pet (se já capturado) acontecem juntas, sem um bloquear o outro

**Verificar:** `Automation RunTests BattleSquare.BattleArena.VictoryGrantsExperienceToOwnPet`
**Commit:** `feat(battlesquare): batalha credita XP ao pet do jogador local`

---

### T5: Fiação em `ABattleSquareGameMode::HandleRoomReady`
**O quê:** depois de `TranslateMatchup`, cada pet cujo `CatalogId` já está na coleção local recebe `ApplyLevelBonus` conforme o nível dele, antes de `AssembleMatchForRoom`.
**Onde:** `Source/BattleSquare/Private/Net/BattleSquareGameMode.cpp`
**Depende de:** T3
**Requisito:** NIVEL-08 (fiação real)

**Pronto quando:**
- [ ] Pet de catálogo já capturado e nível > 1 entra na partida com atributos efetivos maiores
- [ ] Pet não capturado (ou capturado, nível 1) entra sem bônus — comportamento idêntico a antes desta feature
- [ ] `BattleSquare.Net.GameMode.AssembleMatchForRoomBuildsRealArena` (já existente) continua passando sem modificação

**Verificar:** `Automation RunTests BattleSquare.Net.GameMode.LevelBonusAppliedAtAssembly`
**Commit:** `feat(battlesquare): montagem de partida aplica bônus de nível do pet já capturado`

---

### T6: Regressão completa
**O quê:** `BattleSquare` + `BattleSim` inteiros, sondas.
**Onde:** n/a — verificação
**Depende de:** T5
**Requisito:** Success Criteria da spec

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` — Success == total, Fail == 0
- [ ] `Automation RunTests BattleSim` (52 testes) — continua limpo
- [ ] `./Tools/audit_determinism.sh`, `./Tools/audit_no_recalculation.sh`, `./Tools/probe_isolation.sh` — todos `exit 0`
- [ ] **L-020 aplicada:** rebuild de verdade depois de `probe_isolation.sh`, antes de rodar testes

**Verificar:** rodar tudo, ler o log
**Commit:** (nenhum — verificação)

---

## Cobertura de Requisitos

10 requisitos na spec · **9 mapeados** (NIVEL-10, P2, "nível/XP visível na consulta", já é satisfeito pelo `LoadCollection` existente retornar `FOwnedPetInstance` inteiro, incluindo `Experience` — `GetLevel` é uma função pública de `FPetProgressionService`, consultável a qualquer momento sobre o resultado; não precisa de tarefa própria).
