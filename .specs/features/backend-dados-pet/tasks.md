# Backend de Dados de Pet — Tarefas

**Design:** `.specs/features/backend-dados-pet/design.md`
**Status:** ✅ Completo e verificado — 22/22 tarefas, 9/9 testes BattleSquare Success (2026-08-25)
**Escopo:** três serviços/módulos — `apps/api-battle-pets` (Bun+Postgres), `apps/worker-pet-sync` (Bun), e a integração C++ na Unreal (`BattleSquare`, nunca `BattleSim`).

---

## Plano de Execução

### Fase 1 — Fundação do backend (sequencial)
> 🤖 Modelo: `sonnet`

```
T1 → T2 → T3 → T4
```

### Fase 2 — Endpoints CRUD (paralelo após Fase 1)
> 🤖 Modelo: `sonnet`

```
        ┌→ T5 [P]
T4 ─────┼→ T6 [P]
        ├→ T7 [P]
        └→ T8 [P]
```

### Fase 3 — Assinatura e exportação (sequencial)
> 🤖 Modelo: `sonnet` — **T9 é 🧠** (correção criptográfica importa mais que estilo)

```
T9 → T10
```

### Fase 4 — Sidecar de sincronização (sequencial)
> 🤖 Modelo: `sonnet`

```
T11 → T12 → T13 → T14
```

### Fase 5 — Integração Unreal (sequencial)
> 🤖 Modelo: `sonnet` — **T15 e T16 são 🧠** (linkagem de biblioteca terceira em C++ é a parte de maior risco do lote; usar a skill `unreal-thirdparty`)

```
T15 → T16 → T17 → T18 → T19
```

### Fase 6 — Verificação de ponta a ponta (paralelo)
> 🤖 Modelo: `sonnet`

```
T19 ──┬→ T20 [P]
      ├→ T21 [P]
      └→ T22 [P]
```

---

## Tarefas

### T1: Scaffold do backend
**O quê:** projeto Bun em `apps/api-battle-pets` — `package.json`, `tsconfig.json`, config do Drizzle, schema de env validado (nodejs.md: nunca `process.env.X` solto).
**Onde:** `apps/api-battle-pets/`
**Depende de:** nada
**Requisito:** AD-015
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] `bun run` sobe um servidor vazio respondendo em `/health`
- [ ] `environment.ts` valida `DATABASE_URL`, `ADMIN_API_TOKEN`, `SYNC_API_TOKEN`, `ED25519_PRIVATE_KEY` via schema (zod)
- [ ] Falha alto e claro se uma env var obrigatória faltar — nunca segue com default silencioso

**Verificar:** `bun run apps/api-battle-pets/src/index.ts` sobe sem erro com `.env` completo; falha com mensagem clara com `.env` incompleto
**Commit:** `feat(api-battle-pets): scaffold do serviço`

---

### T2: Schema `pets` e migration
**O quê:** tabela `pets` via Drizzle, migration versionada.
**Onde:** `apps/api-battle-pets/src/pet/pet.schema.ts` (schema), `apps/api-battle-pets/drizzle/`
**Depende de:** T1
**Requisito:** PETDB-01 a 05
**Ferramentas:** MCP `context7` (API atual do Drizzle) · Skill nenhuma

**Pronto quando:**
- [ ] `id uuid` PK, `type varchar` (sem enum nativo, code-standart.md §8)
- [ ] `createdAt`/`updatedAt` com default e atualização automática
- [ ] Migration gerada e aplicável (`drizzle-kit generate` + apply local)

**Verificar:** migration roda contra um Postgres local; `\d pets` mostra as colunas esperadas
**Commit:** `feat(api-battle-pets): schema e migration da tabela pets`

---

### T3: Validação Zod (create/update/query)
**O quê:** schemas de validação para os payloads de entrada.
**Onde:** `apps/api-battle-pets/src/pet/pet.schema.ts` (validação, arquivo compartilhado com T2 ou separado por `*.schema.ts`)
**Depende de:** T2
**Requisito:** PETDB-02
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Campo obrigatório ausente → erro com código estável (`PET_NAME_REQUIRED`, etc.)
- [ ] Vários campos inválidos ao mesmo tempo → todos os erros voltam juntos, não só o primeiro (apis.md, Validação)
- [ ] `attack`/`defense`/`speed`/`maxHealth` rejeitam negativo

**Verificar:** teste unitário com payload com 3 erros simultâneos → 3 erros na resposta
**Commit:** `feat(api-battle-pets): validação de payload de pet`

---

### T4: Middleware de autenticação (dois escopos)
**O quê:** Bearer token, dois tokens distintos (`admin`: leitura+escrita, `sync`: só leitura), comparação `timingSafeEqual` (security.md §2).
**Onde:** `apps/api-battle-pets/src/auth/auth.middleware.ts`
**Depende de:** T1
**Requisito:** design.md, fronteiras de confiança
**Ferramentas:** nenhuma

**Pronto quando:**
- [ ] Token de escopo `sync` tentando `POST`/`PUT`/`DELETE` → `403`
- [ ] Token ausente ou inválido → `401`
- [ ] Comparação de token nunca usa `===`/`==` direto em string (timing attack) — usa `crypto.timingSafeEqual`

**Verificar:** teste com os 4 casos (admin ok, sync ok em GET, sync bloqueado em POST, token inválido)
**Commit:** `feat(api-battle-pets): autenticação com dois escopos`

---

### T5: `POST /v1/pets` [P]
**O quê:** cria pet.
**Onde:** `apps/api-battle-pets/src/pet/pet.controller.ts`
**Depende de:** T3, T4
**Requisito:** PETDB-01, PETDB-02

**Pronto quando:**
- [ ] `201` com o registro criado, envelope `{data}`
- [ ] `400` com todos os erros de validação de uma vez
- [ ] Requer escopo `admin`

**Verificar:** `curl -X POST` válido e inválido, mais tentativa com token `sync`
**Commit:** `feat(api-battle-pets): endpoint de criação de pet`

---

### T6: `GET /v1/pets` e `GET /v1/pets/:id` [P]
**O quê:** lista paginada (com `updatedAfter`) e busca por id.
**Onde:** `apps/api-battle-pets/src/pet/pet.controller.ts`
**Depende de:** T3, T4
**Requisito:** PETDB-03, PETDB-04

**Pronto quando:**
- [ ] Lista retorna `{data: [...], pagination: {total, page, perPage}}`
- [ ] `?updatedAfter=<ISO8601>` filtra corretamente
- [ ] Id inexistente → `404`, código `PET_NOT_FOUND`
- [ ] Os dois escopos (`admin`, `sync`) podem ler

**Verificar:** popular 3 pets, atualizar 1, checar que `updatedAfter` retorna só o atualizado
**Commit:** `feat(api-battle-pets): endpoints de leitura de pet`

---

### T7: `PUT /v1/pets/:id` [P]
**O quê:** atualiza pet, atualiza `updatedAt`.
**Onde:** `apps/api-battle-pets/src/pet/pet.controller.ts`
**Depende de:** T3, T4
**Requisito:** PETDB-05

**Pronto quando:**
- [ ] `200` com o registro atualizado
- [ ] `updatedAt` muda mesmo que o valor de um campo seja o mesmo de antes (toque = atualização)
- [ ] Requer escopo `admin`

**Verificar:** atualizar, checar `updatedAt` novo é maior que o antigo
**Commit:** `feat(api-battle-pets): endpoint de atualização de pet`

---

### T8: `DELETE /v1/pets/:id` [P]
**O quê:** remove pet.
**Onde:** `apps/api-battle-pets/src/pet/pet.controller.ts`
**Depende de:** T3, T4
**Requisito:** PETDB-05

**Pronto quando:**
- [ ] `204` sem corpo
- [ ] Segunda tentativa no mesmo id → `404`
- [ ] Requer escopo `admin`

**Verificar:** deletar, tentar de novo, checar `404`
**Commit:** `feat(api-battle-pets): endpoint de remoção de pet`

---

### T9: Assinatura Ed25519 🧠
**O quê:** função que assina um registro de pet com a chave privada; gera o par de chaves (documentar o processo, chave privada nunca versionada).
**Onde:** `apps/api-battle-pets/src/pet/pet-signing.ts`
**Depende de:** T2
**Requisito:** PETDB-10 (refinado no design: Ed25519, não HMAC)
**Ferramentas:** MCP `context7` (biblioteca Ed25519 atual para Bun/Node — `node:crypto` tem suporte nativo desde Node 12, confirmar API)

**Por que 🧠:** é criptografia — um erro sutil (canonicalização inconsistente do JSON antes de assinar, por exemplo) quebra a verificação silenciosamente em todo o resto do pipeline.

**Pronto quando:**
- [ ] Assina sobre uma serialização **canônica** dos campos (ordem de chave fixa — sem isso, a mesma informação pode gerar assinaturas diferentes e quebrar verificação por engano)
- [ ] Mesmo registro assinado duas vezes produz assinaturas verificáveis (Ed25519 não é determinístico por padrão em todas libs — confirmar que a verificação funciona independente disso)
- [ ] Chave privada só existe via env var, nunca em código ou log

**Verificar:** assinar um registro, verificar com a chave pública correspondente → sucesso; alterar 1 byte do payload → verificação falha
**Commit:** `feat(api-battle-pets): assinatura Ed25519 de registros de pet`

---

### T10: `GET /v1/pets/export`
**O quê:** endpoint só para o sync sidecar — retorna pets (com filtro `updatedAfter`) já assinados.
**Onde:** `apps/api-battle-pets/src/pet/pet.controller.ts`
**Depende de:** T6, T9
**Requisito:** PETDB-04, PETDB-10

**Pronto quando:**
- [ ] Cada registro no payload inclui `signature`
- [ ] Requer escopo `sync` (ou `admin`, mas nunca menos que `sync`)
- [ ] `updatedAfter` funciona igual ao de T6

**Verificar:** exportar, verificar assinatura de cada registro com a chave pública
**Commit:** `feat(api-battle-pets): endpoint de exportação assinada`

---

### T11: Scaffold do sidecar de sincronização
**O quê:** projeto Bun em `apps/worker-pet-sync`.
**Onde:** `apps/worker-pet-sync/`
**Depende de:** nada (paralelo à Fase 1-3, mas sequenciado aqui por simplicidade de execução)
**Requisito:** design.md, componente Sync

**Pronto quando:**
- [ ] `environment.ts` valida `API_BATTLE_PETS_URL`, `SYNC_API_TOKEN`, `ED25519_PUBLIC_KEY`, `LOCAL_MIRROR_PATH`, `SQLCIPHER_KEY`
- [ ] Falha alto e claro se env var obrigatória faltar

**Verificar:** roda com `.env` completo, falha com claro com incompleto
**Commit:** `feat(worker-pet-sync): scaffold do serviço`

---

### T12: Schema do espelho local (SQLite + SQLCipher)
**O quê:** cria o arquivo local criptografado, mesma estrutura do payload assinado (design.md: "o espelho É o payload persistido").
**Onde:** `apps/worker-pet-sync/src/mirror/mirror.schema.ts`
**Depende de:** T11
**Requisito:** PETDB-11
**Ferramentas:** MCP `context7` (binding SQLCipher para Bun/Node atual)

**Pronto quando:**
- [ ] Arquivo criado é ilegível sem a chave (`sqlite3` puro no arquivo falha; só abre com a chave correta)
- [ ] Colunas espelham exatamente `SignedPetExport` (id, name, type, attack, defense, speed, maxHealth, updatedAt, signature)

**Verificar:** tentar abrir o arquivo gerado com `sqlite3` sem `PRAGMA key` → erro; com a chave → abre
**Commit:** `feat(worker-pet-sync): espelho local criptografado`

---

### T13: Verificação de assinatura (chave pública)
**O quê:** função espelho da T9 — verifica, não assina.
**Onde:** `apps/worker-pet-sync/src/mirror/signature-verifier.ts`
**Depende de:** T11
**Requisito:** PETDB-10

**Pronto quando:**
- [ ] Registro genuíno (assinado por T9) → verificação passa
- [ ] Registro com 1 campo alterado após assinado → verificação falha
- [ ] Nunca precisa da chave privada — só a pública

**Verificar:** reusar os pares assinatura/registro do teste de T9, confirmar verificação bate
**Commit:** `feat(worker-pet-sync): verificação de assinatura Ed25519`

---

### T14: Loop de sincronização
**O quê:** poll periódico, grava só registros verificados, resiliente a backend fora do ar.
**Onde:** `apps/worker-pet-sync/src/mirror/sync-loop.ts`
**Depende de:** T12, T13
**Requisito:** PETDB-06, PETDB-07, PETDB-08

**Pronto quando:**
- [ ] Ao iniciar, faz sync completo antes de sinalizar "pronto"
- [ ] Ciclos seguintes usam `updatedAfter` (incremental)
- [ ] Registro que falha verificação é descartado e logado, não trava o ciclo
- [ ] Backend inacessível → mantém o espelho existente, tenta de novo no próximo intervalo, não derruba o processo

**Verificar:** simular backend fora do ar (parar o serviço) durante um ciclo — processo do sidecar continua vivo, espelho intacto
**Commit:** `feat(worker-pet-sync): loop de sincronização resiliente`

---

### T15: Linkagem de SQLite/SQLCipher na Unreal 🧠
**O quê:** integrar a biblioteca de leitura SQLite/SQLCipher como third-party no módulo `BattleSquare`.
**Onde:** `Source/BattleSquare/ThirdParty/SQLCipher/` (novo módulo `ModuleType.External`), `BattleSquare.Build.cs` atualizado
**Depende de:** nada (paralelo às Fases 1-4, sequenciado aqui)
**Requisito:** design.md, `UPetDataLoader`
**Ferramentas:** Skill `unreal-thirdparty` (obrigatória — é exatamente o caso de uso dela)

**Por que 🧠:** é a integração de maior risco do lote — build de biblioteca terceira em C++/Unreal, ABI, RTTI/exceptions, ligação estática vs. dinâmica.

**Pronto quando:**
- [ ] `BattleSquareEditor` compila com a dependência linkada
- [ ] Um teste simples abre um arquivo SQLCipher de amostra (gerado por T12) e lê uma linha
- [ ] **`BattleSim` continua sem essa dependência** — só `BattleSquare` a recebe

**Verificar:** build verde; sonda de isolação (variante de `probe_isolation.sh`) confirma que `BattleSim` não linka SQLite
**Commit:** `feat(battlesquare): integra SQLCipher como third-party`

---

### T16: Linkagem de Ed25519 na Unreal 🧠
**O quê:** integrar biblioteca de verificação Ed25519 (ex.: libsodium) no módulo `BattleSquare`.
**Onde:** `Source/BattleSquare/ThirdParty/Ed25519/`, `BattleSquare.Build.cs`
**Depende de:** nada (paralelo)
**Requisito:** design.md, `UPetDataLoader`
**Ferramentas:** Skill `unreal-thirdparty`

**Pronto quando:**
- [ ] `BattleSquareEditor` compila com a dependência linkada
- [ ] Verifica uma assinatura de amostra gerada pelo T9/T13 e confirma o mesmo resultado (mesma chave pública, mesmo par assinatura/registro)
- [ ] `BattleSim` continua sem essa dependência

**Verificar:** build verde; resultado de verificação bate com o do sidecar TypeScript para o MESMO par (prova de compatibilidade entre as duas implementações)
**Commit:** `feat(battlesquare): integra verificação Ed25519 como third-party`

---

### T17: `UPetDataLoader`
**O quê:** lê o espelho local, reverifica assinatura de cada registro, descarta o que falhar.
**Onde:** `Source/BattleSquare/Public/Data/PetDataLoader.h` + `Private/Data/PetDataLoader.cpp`
**Depende de:** T15, T16
**Requisito:** PETDB-09, PETDB-10 (defesa em profundidade)

**Pronto quando:**
- [ ] Lê todos os registros válidos do espelho
- [ ] Registro com assinatura adulterada é descartado e logado como possível violação
- [ ] Espelho vazio ou ausente → erro explícito, não lista vazia silenciosa (PETDB-06)

**Verificar:** rodar contra um espelho gerado por T14; corromper 1 registro manualmente e confirmar que só ele é descartado
**Commit:** `feat(battlesquare): leitor do espelho local de pets`

---

### T18: `UBattleDataTranslator`
**O quê:** traduz o pet carregado (com `type` string, `uuid`) para `FPetState` (núcleo, todos inteiros) — mapeamento `uuid → PetId` local à partida (DP-06).
**Onde:** `Source/BattleSquare/Public/Battle/BattleDataTranslator.h` + `.cpp`
**Depende de:** T17
**Requisito:** design.md, Modelos de Dados

**Pronto quando:**
- [ ] `attack`/`defense`/`speed`/`maxHealth` copiados sem conversão
- [ ] `Health = MaxHealth` no início da batalha
- [ ] `type` (string) vira `FGameplayTag` **nesta camada**, nunca chega como string ao `BattleSim`
- [ ] Dois pets na mesma partida recebem `PetId` distintos, estável durante toda a batalha

**Verificar:** traduzir 2 pets de amostra, montar um `FBattleState`, confirmar que `FBattleResolver::ResolveTurn` aceita e resolve normalmente
**Commit:** `feat(battlesquare): tradutor de dados de pet para o núcleo`

---

### T19: Sonda de isolação estendida
**O quê:** confirmar que toda essa feature nova não furou a fronteira do `BattleSim` — variante do `probe_isolation.sh` (AD-012), agora também testando SQLite/Ed25519.
**Onde:** `Tools/probe_isolation.sh` (estendido) ou novo `Tools/probe_isolation_pet_backend.sh`
**Depende de:** T15, T16, T17, T18
**Requisito:** AD-011, AD-012, PETDB-09

**Pronto quando:**
- [ ] Sonda planta referência a um tipo do SQLCipher/Ed25519 dentro do `BattleSim` → build precisa **falhar**
- [ ] Sonda é removida mesmo se o script for interrompido (`trap`, mesmo padrão do original)

**Verificar:** `./Tools/probe_isolation_pet_backend.sh; echo $?` → `0`
**Commit:** `chore(battlesim): estende sonda de isolação para o backend de pets`

---

### T20: Teste de integração de ponta a ponta [P]
**O quê:** backend assina → sidecar verifica e grava → loader lê e reverifica → tradutor produz `FPetState` válido → resolvedor aceita.
**Onde:** documentado como roteiro manual (não é teste automatizado único, é a cadeia inteira) — `docs/verification/pet-backend-e2e.md` ou similar
**Depende de:** T10, T14, T19
**Requisito:** todos os PETDB

**Pronto quando:**
- [ ] Um pet criado via API aparece no espelho local em até 1 ciclo de sync
- [ ] Esse pet é lido pelo `UPetDataLoader`, traduzido, e usado numa resolução de turno real
- [ ] Toda a cadeia registrada com evidência (logs/output de cada etapa)

**Verificar:** roteiro executado manualmente uma vez, evidência anexada
**Commit:** `test: verificação de ponta a ponta do backend de dados de pet`

---

### T21: Teste de resiliência [P]
**O quê:** backend fora do ar durante sync não afeta partida em andamento nem trava o sidecar.
**Onde:** mesmo roteiro de verificação
**Depende de:** T14
**Requisito:** PETDB-08

**Pronto quando:**
- [ ] Sidecar rodando, backend derrubado no meio de um ciclo → processo continua vivo
- [ ] Espelho local não é apagado nem corrompido
- [ ] Próximo ciclo, com backend de volta, sincroniza normalmente

**Verificar:** roteiro executado, evidência de que o processo sobreviveu
**Commit:** `test: resiliência do sync a backend indisponível`

---

### T22: Teste de adulteração [P]
**O quê:** corromper o espelho local depois de sincronizado, confirmar que o `UPetDataLoader` rejeita e loga.
**Onde:** mesmo roteiro de verificação
**Depende de:** T17
**Requisito:** PETDB-10

**Pronto quando:**
- [ ] Editar 1 campo de 1 registro direto no arquivo SQLite local (fora do fluxo de sync)
- [ ] `UPetDataLoader` descarta só aquele registro, mantém os outros
- [ ] Log registra o evento como possível violação

**Verificar:** roteiro executado, log conferido
**Commit:** `test: rejeição de registro adulterado no espelho local`

---

## Checagem de Granularidade

| Tarefa | Escopo | Situação |
|---|---|---|
| T1–T4 | 1 arquivo/preocupação cada | ✅ |
| T5–T8 | 1 endpoint cada | ✅ |
| T9–T10 | 1 função + 1 endpoint | ✅ |
| T11–T14 | 1 componente do sidecar cada | ✅ |
| T15–T19 | 1 integração/componente cada | ✅ |
| T20–T22 | 1 roteiro de verificação cada | ✅ |

---

## Cobertura de Requisitos

12 requisitos na spec · **12 mapeados** para tarefas — nenhum órfão.

---

## Pergunta antes de executar

**Pré-requisitos de ambiente, antes de escrever a primeira linha:** esta feature precisa de **PostgreSQL** e **Bun** disponíveis localmente (backend e sidecar), e a Fase 5 precisa das bibliotecas SQLCipher e Ed25519/libsodium para linkar na Unreal — nenhum desses foi verificado ainda nesta sessão. Vou checar antes de começar T1.

**Ferramentas por tarefa:** `context7` nas tarefas que dependem de API externa que meu conhecimento pode estar desatualizado (Drizzle, SQLCipher binding, Ed25519 nativo). Skill `unreal-thirdparty` obrigatória em T15/T16 — é literalmente o caso de uso dela.
