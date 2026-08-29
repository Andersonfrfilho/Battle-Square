# Arenas Variadas — Especificação

**Status:** Concluída e verificada por teste (status corrigido em 29/08/2026 — a linha dizia "Draft" enquanto a feature já rodava)
**Depende de:** Combate Núcleo (concluído), Escala de Pets e Skills (concluído). Segunda e última feature planejada de M3 — ver `.specs/project/OBJECTIVE.md`.

---

## Problem Statement

A grade 3x3 hoje é puramente geométrica — `Column`/`Row` em `[0,2]`, sem nenhuma propriedade própria de casa. Toda variação tática vem só do pet (ataque, tipo, posição relativa). Isso significa que trocar de "arena" hoje não muda a experiência tática nenhum pouco — é só um cenário visual diferente por trás do mesmo tabuleiro neutro.

**Isto é diferente da feature anterior de um jeito que importa para o design:** Escala de Pets e Skills conseguiu ficar inteiramente fora de `BattleSim` porque efetividade de tipo é uma propriedade FIXA da dupla de pets, resolvível uma vez, antes da batalha começar. Propriedade de casa é o oposto: ela participa da resolução de **cada turno** — bloqueio impede movimento (F3), dano-ao-entrar acontece durante a resolução (F3/F4), buff altera o resultado de combate (F4). Não tem como pré-computar isso fora do núcleo sem duplicar a lógica de resolução inteira do lado de fora — o que violaria BTL-22 e o próprio motivo de `BattleSim` existir isolado. **Esta feature toca `BattleSim`, diferente da anterior, e isso precisa ficar dito com todas as letras antes de qualquer linha de design.**

## Goals

- [ ] Uma casa da grade pode ter uma propriedade: bloqueada (movimento para ela nunca é aceito), dano (quem entra ou termina o turno nela sofre dano fixo), ou buff (quem está nela recebe um bônus a um atributo enquanto permanece)
- [ ] O layout de uma arena (quais casas têm qual propriedade) é dado de partida — parte do `FBattleState`, decidido na montagem, nunca hardcoded como "a casa do meio sempre bloqueia"
- [ ] Toda propriedade de casa produz evento no trace — a apresentação nunca infere "por que" um movimento falhou ou um dano apareceu sem vir de um pet
- [ ] O determinismo do núcleo continua intacto — layout de arena é dado estático consultado durante a resolução, nunca introduz float, RNG não semeado, ou I/O

## Out of Scope

| Item | Razão |
|---|---|
| Editor visual de arena (desenhar o layout numa UI) | Autoria de conteúdo, não mecânica — layout é dado (JSON/struct), a ferramenta de edição é feature futura |
| Propriedades dinâmicas (uma casa que muda de propriedade DURANTE a partida, ex.: lava que liga/desliga) | Layout desta spec é fixo desde a montagem até o fim da partida — propriedade que muda ao longo do tempo é uma dimensão nova, não incluída aqui |
| Mais de uma propriedade por casa simultaneamente (ex.: bloqueada E buff) | Uma casa tem NO MÁXIMO uma propriedade nesta v1 — simplifica a ordem de resolução (não precisa decidir "bloqueio ou buff primeiro" se só um existe por casa) |
| Arenas com tamanho diferente de 3x3 | `GridSize` já é parametrizável em `IsInsideGrid` (`BattleTypes.h`), mas mudar o tamanho de verdade é uma reformulação de UI/câmera (Apresentação do Combate) fora do escopo desta spec |
| Balanceamento automático de quais arenas são "justas" | A ferramenta de simulação em lote (Escala de Pets e Skills) já existe e pode ser reaproveitada para medir isso — não é uma tarefa nova desta feature, é um uso da ferramenta existente |

---

## User Stories

### P1: Casa bloqueada nunca aceita movimento ⭐ MVP

**User Story:** Como jogador, quero que algumas casas da arena sejam intransitáveis, para que a posição na grade vire uma decisão tática real (rota, não só distância).

**Acceptance Criteria:**
1. WHEN um pet tenta mover para uma casa bloqueada THEN o sistema SHALL tratar como movimento inválido — mesmo caminho de "fora da grade" ou "casa ocupada" já existente (`MovimentoBloqueado`, evento já no vocabulário do núcleo)
2. WHEN a casa de origem de um pet é bloqueada (não deveria acontecer por construção, mas por defesa) THEN o sistema SHALL nunca permitir que a montagem de partida posicione um pet numa casa bloqueada
3. WHEN o evento de movimento bloqueado por propriedade de casa é emitido THEN o sistema SHALL ser indistinguível, no tipo de evento, de um bloqueio por ocupação — a AUSÊNCIA de necessidade de um evento novo é o critério de sucesso (reaproveita o vocabulário existente)

**Independent Test:** um pet tenta mover repetidamente para uma casa marcada como bloqueada, num teste que usa o resolvedor real, e o pet nunca sai da posição original.

---

### P1: Casa de dano fere quem está nela ao fim do turno ⭐ MVP

**User Story:** Como jogador, quero que algumas casas sejam perigosas de ocupar, para que ficar parado num lugar "seguro" seja uma escolha, não um padrão sem custo.

**Acceptance Criteria:**
1. WHEN um pet termina um turno (ou um slot, a decidir em design.md) numa casa de dano THEN o sistema SHALL aplicar uma quantidade fixa de dano a ele, usando o mesmo mecanismo de acumulação (`PendingDamage`) e o mesmo evento (`DanoAplicado`) já existentes — nunca um caminho de dano paralelo
2. WHEN o dano de casa mata um pet THEN o sistema SHALL emitir `PetMorreu` normalmente — a causa da morte (casa vs. ataque) não muda o vocabulário do evento

**Independent Test:** um pet posicionado numa casa de dano, sem nenhum ataque do oponente, perde vida ao longo dos turnos só por permanecer lá — provado com o resolvedor real, sem depender de combate.

---

### P1: Casa de buff favorece quem está nela ⭐ MVP

**User Story:** Como jogador, quero que algumas casas deem vantagem tática a quem as ocupa, para que valha a pena arriscar uma posição pior defendida em troca de um bônus.

**Acceptance Criteria:**
1. WHEN um pet está numa casa de buff durante a fase de combate THEN o sistema SHALL aplicar o bônus ao atributo relevante (Attack ou Defense, a decidir em design.md) só enquanto ele estiver lá — nunca permanente, nunca acumulável turno a turno
2. WHEN o pet sai da casa de buff THEN o sistema SHALL remover o bônus imediatamente, sem resíduo no turno seguinte

**Independent Test:** dois combates idênticos, um com o atacante numa casa de buff e outro sem, produzem dano mensuravelmente diferente no mesmo ataque, provado com o resolvedor real.

---

### P2: Layout de arena é auditável e reproduzível

**User Story:** Como quem monta o conteúdo do jogo, quero poder registrar o layout de uma arena como dado versionável, para que arenas sejam conteúdo administrável, não código.

**Why P2:** o MVP (P1) já exige que o layout seja dado, não hardcoded — esta story é sobre ONDE e COMO esse dado é versionado/nomeado, refinamento sobre o que já é estrutural.

**Acceptance Criteria:**
1. WHEN uma arena é nomeada (ex.: "Arena de Lava") THEN o sistema SHALL associar esse nome a um layout específico, reproduzível entre partidas

---

## Edge Cases

- WHEN uma casa de dano teria dano suficiente para matar um pet no MESMO turno em que ele já tomou dano de combate THEN o sistema SHALL somar os dois no mesmo acumulador (`PendingDamage`) e aplicar tudo de uma vez em F5 — mesma garantia de "morte simultânea" que já existe para combate normal (BTL-07)
- WHEN a montagem de partida tenta posicionar um pet numa casa bloqueada (erro de dado/configuração) THEN o sistema SHALL falhar explicitamente na montagem, nunca silenciosamente mover o pet para outro lugar
- WHEN um layout de arena não define propriedade para uma casa THEN o sistema SHALL tratá-la como neutra (sem propriedade) — mesma filosofia de "ausência não é erro" já usada em `GetDirectionDelta` e na tabela de efetividade de tipo
- WHEN um pet está numa casa de buff e é alvo de um ataque (buff é de Defense) THEN o sistema SHALL aplicar o bônus de Defense na hora exata do cálculo de dano do turno em que ele foi atingido, consistente com quando `Defendendo` já é avaliado hoje

---

## Decision Points (para o Design)

- **DP-arena-01 (a mais pesada):** onde o layout vive dentro de `FBattleState` — um array fixo de 9 `ECellProperty` (paralelo à ideia de `Pets`, mas para casas) é o candidato mais simples; precisa decidir se participa de `ComputeHash()` (provavelmente sim, já que afeta o resultado da partida e detecção de dessincronia deveria cobrir isso).
- **DP-arena-02:** dano/buff aplicado "ao fim do turno" vs. "ao fim de cada slot" — afeta se um pet que passa por uma casa de dano SEM PARAR nela (movimento intermediário dentro do mesmo turno, se isso um dia existir) toma dano ou não. Com 1 movimento por turno (v1), a diferença é sutil mas precisa ser uma decisão explícita, não implícita.
- **DP-arena-03:** valor do dano fixo e do percentual de buff — constantes nomeadas, mesmo padrão de `MinDamage`/`AttackDamageMultiplierPercent`.
- **DP-arena-04:** formato de dado do layout — struct C++ simples vs. JSON (mesmo padrão de `TypeEffectivenessTable`, mas agora DENTRO de `BattleSim`, que não tem dependência de `Json` — precisa decidir se o parsing fica em `BattleSquare` e só o resultado (array de enums) atravessa para `BattleSim`, preservando a fronteira de módulo mesmo com o dado sendo consultado durante a resolução).

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| ARENA-01 | P1: Casa bloqueada nunca aceita movimento | Specify | Pending |
| ARENA-02 | P1: Montagem nunca posiciona pet em casa bloqueada | Specify | Pending |
| ARENA-03 | P1: Bloqueio por propriedade reaproveita evento existente | Specify | Pending |
| ARENA-04 | P1: Casa de dano fere ao fim do turno, via PendingDamage | Specify | Pending |
| ARENA-05 | P1: Dano de casa pode matar, evento PetMorreu normal | Specify | Pending |
| ARENA-06 | P1: Casa de buff aplica bônus só enquanto ocupada | Specify | Pending |
| ARENA-07 | P1: Buff removido imediatamente ao sair da casa | Specify | Pending |
| ARENA-08 | P2: Layout de arena é nomeado e reproduzível | Specify | Pending |

**Cobertura:** 8 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Um pet nunca atravessa uma casa bloqueada, em nenhum teste automatizado que tente forçar isso
- [ ] Um pet parado numa casa de dano perde vida de forma mensurável e determinística ao longo de N turnos, sem nenhum ataque do oponente
- [ ] O mesmo ataque produz dano diferente dependendo de o atacante estar ou não numa casa de buff, no mesmo teste, com o resolvedor real
- [ ] `Tools/audit_determinism.sh` continua limpo — nenhuma linha nova introduz float ou RNG não semeado
- [ ] Regressão zero: os 44 testes de `BattleSim` e a bateria de `BattleSquare` continuam passando; uma arena SEM nenhuma propriedade de casa produz resultado idêntico ao comportamento de hoje
