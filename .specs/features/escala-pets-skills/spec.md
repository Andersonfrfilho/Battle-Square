# Escala de Pets e Skills — Especificação

**Status:** Concluída e verificada por teste (status corrigido em 29/08/2026 — a linha dizia "Draft" enquanto a feature já rodava)
**Depende de:** Combate Núcleo (concluído), Backend de Dados de Pet (concluído). Primeira feature de M3 — ver `.specs/project/OBJECTIVE.md` para onde este marco se encaixa no roadmap inteiro.

---

## Problem Statement

O núcleo resolve combate com 6 tipos de ação e uma fórmula de dano fixa (`ComputeDamage`, `BattlePhaseCombat.cpp`) — nenhuma noção de "tipo de pet" existe dentro de `BattleSim`. Isso foi decisão deliberada (AD-012): `FGameplayTag`/tipo nunca atravessa a fronteira do núcleo, fica só na camada de apresentação (`FPetPresentationInfo::TypeTag`, `FBattleDataTranslator`). Com 1 pet de teste de cada lado e uma fórmula única, isso nunca foi testado sob pressão.

**O problema real que esta feature ataca:** o backend já suporta 6–10 pets cadastrados (Backend de Dados de Pet), mas o combate trata todos como estatisticamente iguais — só `Attack`/`Defense`/`Speed`/`MaxHealth` importam, nunca **o que o pet é**. Isso significa que o sistema hoje não prova a alegação central do roadmap (`ROADMAP.md`, M3): que "N pets aguentam escala de conteúdo sem código novo por pet". Até um segundo pet ter uma fraqueza que o primeiro não tem, essa alegação é uma hipótese, não um fato provado.

**A pergunta que esta spec não responde, e que fica para o design:** um "tipo" que afeta dano precisa ser **dado**, e ele precisa entrar em algum lugar de `FBattleState`/`FPetState` para o resolvedor consultá-lo durante `ComputeDamage`. Isso é dado diferente de `FGameplayTag` (que é metadado de apresentação, nunca usado em cálculo) — mas ainda assim é uma mudança na fronteira de `BattleSim`, que até hoje nunca precisou saber "o que" um pet é, só "quanto" ele tem de cada atributo. Esta é a decisão de arquitetura mais pesada da feature, e fica explicitamente para `design.md` (ver Decision Points).

## Goals

- [ ] Um pet tem um tipo (elemento/categoria), e esse tipo produz efetividade concreta em combate (super efetivo / neutro / resistido) contra o tipo do alvo
- [ ] Adicionar um pet novo com um tipo já existente exige **zero mudança de código** — só um registro de dados (mesmo padrão que adicionar um pet novo já é hoje, via API do backend)
- [ ] Existe uma ferramenta que roda N combates completos, headless, sem GUI, e produz um relatório agregado (taxa de vitória por tipo, dano médio, turnos médios) — a base de qualquer decisão de balanceamento futura
- [ ] O determinismo do núcleo (AD-004) continua intacto — a tabela de efetividade de tipo é dado consultado, nunca introduz float, RNG não semeado, ou qualquer fonte de não-determinismo nova

## Out of Scope

| Item | Razão |
|---|---|
| N pets por lado (mais de 1 pet simultâneo por jogador) | Estrutural, maior que esta feature — `FTurnCommit` é por lado, não por pet (B-003, `STATE.md`). Fica registrado como expansão futura dentro de M3, não implícito aqui |
| Skills customizadas por pet (habilidades além dos 6 tipos de ação já existentes) | O ROADMAP fala em "tipos, fraquezas e resistências" — isso é o sistema de tipo elemental. Skill única por pet (ex.: um ataque especial só do Pet X) é uma camada acima, não bloqueada por esta feature mas não incluída nela |
| UI de visualização da tabela de tipo/efetividade para o jogador | Apresentação — feature futura, esta spec cobre o núcleo + ferramenta de balanceamento, não tela nova |
| Balanceamento automático (ajuste de números por algoritmo) | A ferramenta desta feature **mede**, não decide. Ajustar os números é trabalho humano informado pela medição |
| Persistir resultado de simulação em lote em banco/histórico | A ferramenta roda e reporta; guardar histórico de balanceamento é infraestrutura de produto, fora do escopo de uma spec de gameplay |

---

## User Stories

### P1: Tipo de pet afeta o resultado do combate de forma auditável ⭐ MVP

**User Story:** Como jogador, quero que o tipo do meu pet importe taticamente — que atacar um pet fraco ao meu tipo doa mais, e um resistente doa menos — para que escolher pet vire uma decisão estratégica, não só uma escolha de número maior.

**Acceptance Criteria:**
1. WHEN um pet de tipo A ataca um pet de tipo B, e A é super efetivo contra B THEN o sistema SHALL aplicar um multiplicador de dano maior que o neutro (valor exato decidido em design.md, seguindo o padrão de percentual inteiro já usado em `AttackDamageMultiplierPercent`/`MagicDamageMultiplierPercent`)
2. WHEN A é resistido por B THEN o sistema SHALL aplicar um multiplicador menor que o neutro, nunca dano zero (mesma regra de `MinDamage` já existente — todo ataque que acerta causa pelo menos 1 de dano)
3. WHEN A e B não têm relação de efetividade definida (neutro) THEN o sistema SHALL aplicar o multiplicador atual, sem mudança de comportamento — combate hoje (1 pet, sem tipo relevante) continua idêntico
4. WHEN a efetividade de tipo é auditada THEN o sistema SHALL expor de onde veio o número (auditável pelo mesmo espírito de `Tools/audit_no_recalculation.sh` — a apresentação continua só lendo o `Value` do evento, nunca recalculando)

**Independent Test:** dois pets de tipos com efetividade conhecida (um super efetivo contra o outro) produzem dano mensuravelmente diferente de dois pets neutros com os mesmos atributos base, num teste automatizado que roda o resolvedor real.

---

### P1: Adicionar um pet/tipo novo não exige recompilar nada ⭐ MVP

**User Story:** Como quem administra o conteúdo do jogo (via API do backend), quero cadastrar um pet de um tipo já suportado, ou até um tipo novo, sem que alguém precise tocar em C++, para que o sistema realmente escale como o roadmap promete.

**Acceptance Criteria:**
1. WHEN um pet novo é cadastrado no backend com um `type` já presente na tabela de efetividade THEN o sistema SHALL aplicar as regras de efetividade dele automaticamente, sem mudança de código
2. WHEN um tipo completamente novo é adicionado à tabela de efetividade (dado, não código) THEN o sistema SHALL reconhecê-lo na próxima simulação, sem rebuild do módulo `BattleSquare`/`BattleSim`

**Independent Test:** adicionar uma linha na tabela de efetividade (arquivo de dados) e rodar um combate entre dois pets desse tipo novo, sem qualquer alteração em `.cpp`/`.h`.

---

### P1: Ferramenta de simulação em lote produz números confiáveis ⭐ MVP

**User Story:** Como quem está balanceando o jogo, quero rodar centenas de combates automaticamente entre composições de pets diferentes e ver quem vence mais, para decidir se um tipo está desbalanceado antes de qualquer jogador sentir isso.

**Acceptance Criteria:**
1. WHEN a ferramenta roda N combates entre duas composições de pets THEN o sistema SHALL reportar taxa de vitória de cada lado, número médio de turnos até o fim, e dano médio por turno
2. WHEN a ferramenta roda THEN o sistema SHALL usar seeds diferentes e determinísticas por execução (AD-004 — o RNG do núcleo continua sendo o único usado, nunca `FMath::Rand`), de forma que rodar de novo com a mesma seed reproduza o mesmo resultado exato
3. WHEN a ferramenta é invocada headless (linha de comando/Automation) THEN o sistema SHALL rodar sem GUI, sem `-nullrhi` sendo um problema (mesma disciplina de `Tools/audit_*.sh` — roda em CI eventualmente)

**Independent Test:** rodar a ferramenta duas vezes com a mesma seed e confirmar que os agregados batem exatamente; rodar com seeds diferentes e confirmar que os números variam de forma plausível (não travados no mesmo valor).

---

### P2: Relatório de balanceamento aponta o tipo mais forte/mais fraco

**User Story:** Como quem está balanceando, quero que o relatório já aponte quais tipos ganham desproporcionalmente mais, para não precisar ler uma tabela de números crus toda vez.

**Why P2:** melhora a experiência de quem balanceia, mas os números crus (P1) já são suficientes para uma decisão manual informada — não é bloqueio.

**Acceptance Criteria:**
1. WHEN o relatório é gerado THEN o sistema SHALL destacar o(s) tipo(s) com taxa de vitória fora de uma faixa configurável (ex.: >60% ou <40% num confronto que deveria ser equilibrado)

---

## Edge Cases

- WHEN um pet ataca um alvo do mesmo tipo dele THEN o sistema SHALL tratar como neutro por padrão, a menos que a tabela de efetividade declare explicitamente uma relação (alguns jogos têm "mesmo tipo bate menos", isso é decisão de design.md, não desta spec)
- WHEN a tabela de efetividade não tem entrada para um par de tipos THEN o sistema SHALL assumir neutro (nunca crashar, nunca dano zero por ausência de dado) — mesma filosofia de "ausência não é erro" já usada em `GetDirectionDelta`
- WHEN a ferramenta de simulação em lote roda com um número de combates alto (milhares) THEN o sistema SHALL continuar determinístico e não vazar memória de forma perceptível — não precisa ser rápido, precisa ser correto
- WHEN dois pets do mesmo tipo se enfrentam (times espelhados) THEN a ferramenta de simulação SHALL ainda produzir um resultado válido (não é um caso de erro, é o caso mais simples de teste A/B)

---

## Decision Points (para o Design)

- **DP-escala-01 (a mais pesada):** como o tipo do pet entra em `FBattleState`/`FPetState` para o resolvedor consultar, sem violar AD-012 (GameplayTags nunca entra em `BattleSim`). Candidato natural: um campo `uint8 TypeId` em `FPetState` — inteiro puro, sem `FGameplayTag`, resolvendo o **símbolo** (tipo) sem resolver a **string**/metadado. A tabela de efetividade (`TypeId x TypeId -> Multiplicador`) precisa viver em algum lugar consultável pelo núcleo sem quebrar AD-004 (dado estático, sem I/O durante `ResolveTurn`).
- **DP-escala-02:** formato da tabela de efetividade — `UDataTable`/CSV (rápido, edita sem código, mas reintroduz o padrão que AD-008 já superou para pets) vs. um `TArray`/`TMap` estático compilado em C++ (mais rápido, mas "sem código" da User Story 2 fica mais fraco) vs. um arquivo JSON/dado lido do mesmo espelho local que já existe para pets. A spec exige "sem recompilar" (US2) — a decisão aqui precisa honrar isso de verdade, não só no papel.
- **DP-escala-03:** onde a ferramenta de simulação em lote roda — `Automation RunTests` (reaproveita a infraestrutura já usada em todo o projeto) vs. um executável/commandlet dedicado. `Automation` já provou ser confiável e headless-friendly (`Tools/*.sh` inteiros dependem disso); a decisão é se um teste de Automation "aberto" (roda N vezes, não é pass/fail binário) é o encaixe certo ou se merece um caminho próprio.
- **DP-escala-04:** valor do multiplicador super efetivo/resistido — parâmetro de balanceamento, constante nomeada, mesmo padrão de `AttackDamageMultiplierPercent`.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| ESCALA-01 | P1: Multiplicador de tipo super efetivo | Specify | Pending |
| ESCALA-02 | P1: Multiplicador de tipo resistido, nunca dano zero | Specify | Pending |
| ESCALA-03 | P1: Neutro é o comportamento atual, sem regressão | Specify | Pending |
| ESCALA-04 | P1: Efetividade auditável, nunca recalculada na apresentação | Specify | Pending |
| ESCALA-05 | P1: Pet novo de tipo existente funciona sem mudança de código | Specify | Pending |
| ESCALA-06 | P1: Tipo novo reconhecido sem rebuild | Specify | Pending |
| ESCALA-07 | P1: Ferramenta reporta taxa de vitória, turnos médios, dano médio | Specify | Pending |
| ESCALA-08 | P1: Ferramenta determinística por seed | Specify | Pending |
| ESCALA-09 | P1: Ferramenta roda headless | Specify | Pending |
| ESCALA-10 | P2: Relatório destaca tipos desbalanceados | Specify | Pending |

**Cobertura:** 10 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Um combate entre um pet super efetivo e um resistido, com atributos base IDÊNTICOS, produz taxas de vitória visivelmente diferentes ao rodar a ferramenta de simulação em lote (prova que o tipo importa, não só o número)
- [ ] Um pet e um tipo novos são adicionados só por dado (arquivo/registro), e uma simulação os reconhece, sem tocar em `.cpp`/`.h` nem recompilar
- [ ] `Tools/audit_determinism.sh` e `Tools/audit_no_recalculation.sh` continuam limpos — nenhuma linha nova de código introduz float, RNG não semeado, ou recálculo fora de `BattleSim`
- [ ] Regressão zero nos 44 testes de `BattleSim` e nos testes de `BattleSquare` já existentes — combate com tipos neutros (o caso de hoje) continua produzindo exatamente os mesmos resultados de antes
