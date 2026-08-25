# Backend de Dados de Pet — Especificação

**Status:** ✅ Aprovada — seguindo para Design (2026-08-25)
**Depende de:** Combate Núcleo (concluído) — este backend preenche `FPetState`, que o resolvedor já consome.

---

## Problem Statement

Os 250 pets do jogo precisam ser administráveis (balanceamento, criação de conteúdo) sem exigir reimportação de CSV nem recompilação do projeto Unreal a cada ajuste. O usuário decidiu (AD-014) que os pets vivem num banco de dados externo, num sistema **separado** do servidor de combate — não uma extensão dele.

Ao mesmo tempo, o servidor de combate não pode pagar latência de rede nem ficar refém da disponibilidade do backend no meio de uma partida — isso violaria AD-004 (determinismo) e criaria um ponto único de falha no caminho crítico do jogo.

## Goals

- [ ] Um pet pode ser criado, lido, atualizado e removido via API, sem tocar em código do jogo
- [ ] O servidor de combate nunca faz uma chamada de rede durante a resolução de um turno
- [ ] Queda do backend não impede partidas em andamento nem o início de novas, dentro de uma janela de tolerância
- [ ] Schema do pet no backend e `FPetState` no núcleo nunca divergem silenciosamente

## Out of Scope

| Item | Razão |
|---|---|
| Painel administrativo (UI web) | v1 usa a API diretamente (Postman/curl/script). UI é М4+ |
| Autenticação de jogador final | Este backend serve dados de pet, não sessão de jogador — isso é do servidor de combate (M2) |
| As 250 fichas de pet em si (conteúdo) | Schema e API primeiro; popular com 250 pets é trabalho de conteúdo, não de engenharia |
| Skills e efeitos | Esta spec cobre só o pet base (atributos). Skill é feature própria, decidida em M3 |
| Versionamento de schema entre backend e espelho | Registrado como risco (ver Edge Cases), solução adiada até o primeiro caso real de mudança de schema |

---

## Arquitetura (visão geral, detalha em design.md)

```
┌─────────────────────┐         sync           ┌──────────────────────┐         lê         ┌───────────────┐
│  Backend de Pets     │  (polling ou push)     │  Espelho Local        │  (sem rede)        │  Servidor de   │
│  Bun + Postgres      │ ─────────────────────► │  (cache/réplica no    │ ─────────────────► │  Combate       │
│  fonte da verdade    │                        │  processo do servidor │                    │  (BattleSim)   │
│  API REST /v1/pets   │                        │  de combate)          │                    │                │
└─────────────────────┘                        └──────────────────────┘                    └───────────────┘
```

- **Backend de Pets**: serviço HTTP próprio, PostgreSQL via Drizzle, API REST versionada. Fonte da verdade.
- **Espelho local**: mantido pelo servidor de combate (não pelo backend). Atualizado por sincronização periódica. É o único lugar que a montagem de batalha lê.
- **Servidor de Combate**: nunca fala com o Backend de Pets diretamente no caminho de uma partida — só com o espelho.

---

## User Stories

### P1: Criar um pet via API ⭐ MVP

**User Story:** Como administrador de conteúdo, quero criar um pet com seus atributos base via API, para popular o jogo sem editar arquivo nenhum do projeto Unreal.

**Acceptance Criteria:**
1. WHEN um POST válido é enviado para `/v1/pets` THEN o sistema SHALL criar o pet e retornar `201` com o registro criado
2. WHEN o payload falha a validação THEN o sistema SHALL retornar `400` com todos os erros de campo de uma vez, não só o primeiro
3. WHEN um campo obrigatório está ausente THEN o sistema SHALL identificar exatamente qual campo, com código estável (ex.: `PET_NAME_REQUIRED`)

**Independent Test:** `curl -X POST /v1/pets` com payload válido e inválido, checando os dois casos.

---

### P1: Ler pets via API ⭐ MVP

**User Story:** Como servidor de sincronização, quero listar e buscar pets por id, para popular o espelho local.

**Acceptance Criteria:**
1. WHEN um GET é feito em `/v1/pets` THEN o sistema SHALL retornar a lista paginada, envelope `{data, pagination}`
2. WHEN um GET é feito em `/v1/pets/:id` com id existente THEN o sistema SHALL retornar `200` com o pet
3. WHEN o id não existe THEN o sistema SHALL retornar `404` com código `PET_NOT_FOUND`
4. WHEN o cliente de sincronização pede só o que mudou desde X THEN o sistema SHALL suportar filtro por `updatedAfter` (evita full-sync a cada ciclo)

---

### P1: Atualizar e remover pet ⭐ MVP

**Acceptance Criteria:**
1. WHEN um PUT válido é enviado para `/v1/pets/:id` THEN o sistema SHALL atualizar e retornar `200` com o registro atualizado
2. WHEN um DELETE é enviado para `/v1/pets/:id` existente THEN o sistema SHALL remover e retornar `204`
3. WHEN qualquer operação de escrita altera um pet THEN o sistema SHALL atualizar `updatedAt`, para o espelho detectar a mudança

---

### P1: Sincronização do espelho local ⭐ MVP

**User Story:** Como servidor de combate, quero manter uma cópia local dos pets sempre razoavelmente atualizada, para montar batalhas sem depender de rede no caminho crítico.

**Acceptance Criteria:**
1. WHEN o servidor de combate inicia THEN o sistema SHALL fazer uma sincronização completa antes de aceitar a primeira partida
2. WHEN o intervalo de sincronização se cumpre THEN o sistema SHALL buscar apenas pets com `updatedAt` mais recente que a última sincronização
3. WHEN o backend está inacessível durante uma sincronização periódica THEN o sistema SHALL manter o espelho existente e tentar de novo no próximo ciclo, sem interromper partidas em andamento
4. WHEN uma batalha monta `FBattleState` THEN o sistema SHALL ler exclusivamente do espelho local — nunca do backend diretamente
5. WHEN o backend está inacessível na primeira inicialização (espelho ainda vazio) THEN o sistema SHALL recusar iniciar partidas com pets ausentes, com erro claro — não é um caso silencioso

---

## Segurança do Espelho Local

O espelho vive **exclusivamente no processo do servidor de combate** — nunca é enviado ou lido pelo cliente do jogador. Como o servidor é autoritativo (AD-005), essa já é a defesa primária: o jogador nunca tem acesso de escrita.

**Duas camadas, propósitos diferentes:**

| Mecanismo | Protege contra | Não protege contra |
|---|---|---|
| Criptografia em repouso (SQLCipher) | Disco ou backup roubado e lido offline, fora do processo do servidor | Alguém com acesso equivalente ao processo do servidor em tempo de execução — a chave precisa estar acessível ali para o servidor funcionar |
| Verificação de integridade (assinatura por registro) | Adulteração do arquivo local por quem tem acesso ao disco mas não ao backend — a assinatura não bate e o registro é rejeitado | Comprometimento total do backend em si (fonte da verdade) |

**Refinado no Design:** o mecanismo de assinatura usa **Ed25519 (par de chaves assimétrico)**, não HMAC simétrico. Só o backend guarda a chave privada; a chave pública fica embarcada em qualquer verificador (sidecar de sync, servidor de combate) e serve só para conferir, nunca para assinar — se um verificador for comprometido, ele não ganha a capacidade de forjar assinaturas novas.

**Decisão adotada:** as duas camadas, para propósitos distintos — criptografia como defesa em profundidade barata; assinatura como o mecanismo que de fato detecta e rejeita adulteração.

## Anticheat — Consequência da Adulteração Detectada

**Princípio estrutural, já garantido por AD-005:** adulterar o cache local de pet no cliente NÃO consegue mudar o resultado de uma luta — o servidor resolve com seus próprios dados, nunca com o que o cliente reporta. A detecção de adulteração não é uma correção de resultado; é um **sinal de comportamento hostil** com consequência própria.

**Nível 1 — dentro da sessão atual (implementável agora, sem infraestrutura nova):**
1. Assinatura do pet no cliente não bate (mesmo mecanismo de PETDB-10) → evidência registrada (quem, quando, o que divergiu)
2. Sessão do cliente adulterado é encerrada imediatamente
3. Partida NÃO é anulada — o oponente é declarado vencedor por W.O., reconhecendo que ele jogou de boa-fé e teve a partida interrompida por causa alheia

**Nível 2 — bloqueio persistente entre partidas (FORA DE ESCOPO desta spec, depende de conta de jogador):**
"Banir" alguém de forma duradoura exige identidade persistente entre sessões, que este projeto ainda não tem — v1 usa código de sala, sem conta de usuário (ver PROJECT.md, Out of Scope). Fica registrado como dependência: quando um sistema de conta existir, esta trilha de auditoria (evidências do Nível 1) é a base para banimento — não precisa ser reconstruída, só consumida por um sistema de moderação futuro.

## Edge Cases

- WHEN dois pets têm o mesmo nome THEN o sistema SHALL permitir (nome não é chave única — id é)
- WHEN um pet é removido no backend enquanto está referenciado numa partida em andamento no espelho THEN o sistema SHALL deixar a partida em andamento terminar normalmente com os dados que já tinha — a remoção só afeta partidas futuras
- WHEN o schema do backend ganha um campo novo que o espelho/`FPetState` não conhece THEN o sistema SHALL ignorar o campo desconhecido no espelho, sem quebrar a sincronização (tolerância a campo adicional, não a campo removido)
- WHEN um atributo chega do backend fora da faixa esperada (ex.: `attack` negativo) THEN o sistema SHALL rejeitar o registro na fronteira de validação (Zod), nunca deixar entrar no espelho
- WHEN um registro no espelho local falha a verificação de assinatura (adulterado ou corrompido) THEN o sistema SHALL rejeitar o registro, logar o evento como possível violação, e tratar como se o pet estivesse ausente (mesmo caminho de PETDB-06/08) — nunca usar um dado não verificado numa batalha

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| PETDB-01 | P1: Criar pet | Specify | Pending |
| PETDB-02 | P1: Validação de payload com todos os erros | Specify | Pending |
| PETDB-03 | P1: Listar e buscar pet | Specify | Pending |
| PETDB-04 | P1: Filtro incremental (updatedAfter) | Specify | Pending |
| PETDB-05 | P1: Atualizar e remover pet | Specify | Pending |
| PETDB-06 | P1: Sync completo na inicialização | Specify | Pending |
| PETDB-07 | P1: Sync incremental periódico | Specify | Pending |
| PETDB-08 | P1: Resiliência a backend fora do ar | Specify | Pending |
| PETDB-09 | P1: Montagem de batalha lê só o espelho | Specify | Pending |

| PETDB-10 | P1: Assinatura por registro, verificada antes do uso | Specify | Pending |
| PETDB-11 | P1: Criptografia em repouso do espelho local | Specify | Pending |
| PETDB-12 | P2: Sessão encerrada e vitória por W.O. ao detectar adulteração no cliente | Specify | Pending |

**Cobertura:** 12 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Criar, ler, atualizar e remover um pet funciona de ponta a ponta via API
- [ ] Derrubar o backend não impede uma partida já em andamento
- [ ] Uma sincronização incremental não rebaixa nenhum pet que não mudou (idempotente)
- [ ] Nenhuma chamada de rede acontece dentro de `FBattleResolver::ResolveTurn` — auditável pelo mesmo tipo de sonda do AD-012
