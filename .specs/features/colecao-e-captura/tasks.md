# Coleção e Captura — Tarefas

**Design:** `.specs/features/colecao-e-captura/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`. `BattleSim` não é tocado.

---

## Plano de Execução

### Fase 1 — Tipos e serviço de coleção (sequencial)
> 🤖 Modelo: `sonnet`

```
T1 → T2 → T3
```

### Fase 2 — Fiação com o fim de batalha (depende da Fase 1)
> 🤖 Modelo: `sonnet` — **T5 é 🧠**

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

### T1: `FOwnedPetInstance` e `UPetCollectionSaveGame`
**O quê:** tipos de dado do save — instância possuída (`CatalogId`, `Name`, `Type`, `Experience`) e o `USaveGame` que os guarda.
**Onde:** `Source/BattleSquare/Public/Meta/PetCollectionSaveGame.h`
**Depende de:** nada
**Requisito:** DP-colecao-01, DP-colecao-03

**Pronto quando:**
- [ ] `FOwnedPetInstance` tem os 4 campos, `Experience` default 0
- [ ] `UPetCollectionSaveGame::OwnedPets` default vazio
- [ ] Compila

**Verificar:** build verde
**Commit:** `feat(battlesquare): tipos de save da coleção de pets`

---

### T2: `FPetCollectionService::CaptureIfNewInMemory`
**O quê:** lógica pura de captura — pet novo (`CatalogId` ausente) é adicionado, pet repetido não duplica.
**Onde:** `Source/BattleSquare/Public/Meta/PetCollectionService.h` + `.cpp`
**Depende de:** T1
**Requisito:** COLECAO-01, COLECAO-03

**Pronto quando:**
- [ ] `CatalogId` novo → adiciona, retorna `true`
- [ ] `CatalogId` já presente → não duplica, retorna `false`
- [ ] Dois pets com o mesmo `Type` mas `CatalogId` diferente são capturas independentes (edge case da spec)
- [ ] Função pura, sem tocar `UGameplayStatics`/disco

**Verificar:** `Automation RunTests BattleSquare.Meta.PetCollectionService.CaptureIfNewInMemory`
**Commit:** `feat(battlesquare): lógica pura de captura, sem duplicar`

---

### T3: `FPetCollectionService::CaptureIfNew`/`LoadCollection` — persistência real
**O quê:** carrega/salva via `UGameplayStatics::SaveGameToSlot`/`LoadGameFromSlot`. Save ausente/corrompido vira coleção vazia, nunca crash.
**Onde:** mesmo arquivo de T2
**Depende de:** T2
**Requisito:** COLECAO-02, COLECAO-05, COLECAO-06, edge case "save corrompido"

**Pronto quando:**
- [ ] Capturar, depois `LoadCollection` do mesmo slot, retorna a instância persistida
- [ ] Slot nunca usado antes → `LoadCollection` retorna lista vazia, sem erro
- [ ] Slot de teste dedicado, nunca o de produção — limpo ao fim de cada teste

**Verificar:** `Automation RunTests BattleSquare.Meta.PetCollectionService.PersistsAcrossLoadCycle`
**Commit:** `feat(battlesquare): persistência real da coleção em SaveGame`

---

### T4: `FPetPresentationInfo::CatalogId` + retenção em `ABattleArena`
**O quê:** `CatalogId` novo em `FPetPresentationInfo` (preenchido em `TranslatePet`/`TranslateMatchup` a partir de `FLoadedPetRecord::Id`); `ABattleArena` retém as `Presentations` recebidas em `BeginBattle`, indexadas por `PetId`.
**Onde:** `Source/BattleSquare/Public/Data/BattleDataTranslator.h`/`.cpp`, `Source/BattleSquare/Public/Battle/BattleArena.h`/`.cpp`
**Depende de:** nada (paralelo à Fase 1)
**Requisito:** DP-colecao-03

**Pronto quando:**
- [ ] `TranslatePet`/`TranslateMatchup` preenchem `CatalogId` corretamente a partir do `Source.Id`
- [ ] `ABattleArena::BeginBattle` guarda as `Presentations` recebidas, consultáveis por `PetId` depois
- [ ] `BattleArenaTest`/`BattleDataTranslatorMatchupTest` existentes continuam passando sem modificação

**Verificar:** `Automation RunTests BattleSquare.Data.TranslatePetPreservesCatalogId`
**Commit:** `feat(battlesquare): CatalogId preservado da tradução até a arena`

---

### T5: `ABattleArena::CheckForCapture` 🧠
**O quê:** ao detectar `BatalhaEncerrada` com `Value == LocalPlayerSide`, identifica o pet do lado OPOSTO e chama `FPetCollectionService::CaptureIfNew`.
**Onde:** `Source/BattleSquare/Private/Battle/BattleArena.cpp` (extensão de `HandlePlayerCommitted`/`HandleCoordinatorTurnResolved`, logo após `EvaluateOutcome`)
**Depende de:** T3, T4
**Requisito:** COLECAO-01, COLECAO-04, edge cases (abandono conta como vitória, Standalone captura normalmente)

**Por que 🧠:** identificar o lado OPOSTO ao vencedor é fácil de inverter (capturar o PRÓPRIO pet do jogador por engano, em vez do oponente) — sem teste específico, um erro assim passaria despercebido porque "algo foi capturado" já pareceria sucesso.

**Pronto quando:**
- [ ] Vitória do jogador local captura o pet do OPONENTE, nunca o próprio pet do jogador — teste com pets de `CatalogId` claramente distintos confirma qual foi capturado
- [ ] Derrota do jogador local não captura nada
- [ ] Empate não captura nada
- [ ] Vitória por abandono (`DeclareAbandonment`, mesmo evento `BatalhaEncerrada`) captura normalmente
- [ ] Vitória contra pet já capturado não duplica (reaproveita T2)

**Verificar:** `Automation RunTests BattleSquare.BattleArena.VictoryCapturesOpponentPet`
**Commit:** `feat(battlesquare): vitória captura o pet oponente, nunca o próprio`

---

### T6: Regressão completa
**O quê:** `BattleSquare` + `BattleSim` inteiros.
**Onde:** n/a — verificação
**Depende de:** T5
**Requisito:** Success Criteria da spec

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` — Success == total, Fail == 0
- [ ] `Automation RunTests BattleSim` (52 testes) — continua limpo
- [ ] **Lição L-020 aplicada:** se qualquer sonda de plantar-código (`probe_isolation.sh`) rodar nesta verificação, rebuildar de verdade antes de rodar os testes

**Verificar:** rodar tudo, ler o log
**Commit:** (nenhum — verificação)

---

## Cobertura de Requisitos

8 requisitos na spec · **7 mapeados** (COLECAO-07, "jogador consulta a coleção", é satisfeito por `LoadCollection` de T3 — não precisa de tarefa própria, é o mesmo método; COLECAO-08, P2, XP em vitória redundante, fica registrado como gancho em `FOwnedPetInstance::Experience` mas a lógica de conceder XP em si é da próxima feature, Níveis/Experiência/Evolução — não implementada aqui, para não inflar esta feature além do que ela promete).
