# Níveis, Experiência e Evolução — Especificação

**Status:** Draft — aguarda aprovação
**Depende de:** Coleção e Captura (concluída). Segunda e última feature planejada de M4 — ver `.specs/project/OBJECTIVE.md`.

---

## Problem Statement

`FOwnedPetInstance` já tem um campo `Experience`, criado como gancho deliberado em Coleção e Captura (P2, "vitória redundante concede XP") — mas nada o incrementa, nada o consome, e não existe conceito de nível. Um pet capturado hoje é uma entrada estática: nunca fica mais forte, nunca muda.

**A tensão que esta spec precisa resolver de forma explícita:** progressão de pet pressupõe que o pet que luta é o pet do jogador. Mas "Seleção de time" continua fora de escopo (registrado assim desde Apresentação do Combate, reafirmado em Coleção e Captura) — a montagem de partida hoje usa os 2 primeiros pets do catálogo (`HandleRoomReady`, Escala de Pets e Skills), não a coleção do jogador. Esta spec **não resolve seleção de time**. O que ela faz: quando o pet que lutou (de qualquer lado da partida, vencendo ou perdendo) corresponde a um `CatalogId` que o jogador **já possui** na coleção, aquela instância ganha XP pelo resultado. Se o pet que lutou ainda não foi capturado, não há instância para dar XP — nada acontece, sem erro. Isso é consistente com o mecanismo de captura já existente (mesmo gancho, mesmo evento `BatalhaEncerrada`) e não inventa seleção de time por trás das costas da decisão já tomada de adiar isso.

**Segunda decisão que precisa ficar explícita:** "evolução" nesta spec significa **crescimento de atributos por nível**, não troca de espécie/forma. O catálogo do backend não modela cadeias de evolução (não existe campo "evolui para"), e criar isso é uma mudança de schema fora do escopo de uma feature de progressão local. Evoluir, aqui, é o mesmo `CatalogId` ficando mais forte — `Attack`/`Defense`/`Speed`/`MaxHealth` crescem com o nível, aplicados como bônus sobre a base do catálogo no momento da montagem de partida (mesmo padrão arquitetural de `TranslateMatchup` pré-multiplicar `Attack` por efetividade de tipo — outra vez, sem tocar `BattleSim`).

## Goals

- [ ] Toda batalha que envolve um pet já capturado concede XP à instância correspondente — vitória concede mais que derrota/empate (mantém pessoa jogando engajada mesmo perdendo)
- [ ] XP acumulado sobe de nível segundo uma tabela de limiares determinística, sem depender de float
- [ ] Nível mais alto se traduz em atributos de batalha mais fortes, aplicados na montagem de partida — nunca dentro de `BattleSim`
- [ ] Existe um teto de nível (não cresce para sempre) — número finito, decidido em design.md
- [ ] A progressão persiste no mesmo save local da coleção (Coleção e Captura) — não um sistema de save paralelo

## Out of Scope

| Item | Razão |
|---|---|
| Seleção de time / escolher qual pet capturado leva pra batalha | Decisão já tomada e reafirmada em specs anteriores — continua fora daqui |
| Evolução como troca de espécie/CatalogId | Backend não modela cadeia de evolução; "evoluir" nesta spec é crescimento de atributo, não troca de forma |
| Curva de XP ajustável por conteúdo/dados (sem recompilar) | A tabela de limiares de nível é uma constante nomeada nesta v1, mesmo padrão de `MaxTurns`/`AttackDamageMultiplierPercent` — não é dado externo como `TypeEffectiveness.json`; motivo: poucos números, mudam raramente, e não há requisito registrado pedindo isso |
| Perda de XP/nível (penalidade por derrota) | Derrota concede XP menor, nunca negativo — jogo não pune o jogador por perder, só recompensa menos |
| UI de barra de XP / animação de "subiu de nível" | Mesma separação de sempre — esta spec cobre a lógica, layout fica para autoria posterior |

---

## User Stories

### P1: Vitória concede XP ao pet do jogador que lutou ⭐ MVP

**User Story:** Como jogador, quero que meu pet fique mais forte por lutar, para que jogar tenha uma recompensa contínua além de capturar coisas novas.

**Acceptance Criteria:**
1. WHEN uma batalha termina (`BatalhaEncerrada`) e o pet do lado do jogador local corresponde a um `CatalogId` já na coleção THEN o sistema SHALL conceder XP à instância correspondente
2. WHEN o jogador local vence THEN o sistema SHALL conceder mais XP que quando ele perde ou empata
3. WHEN o pet que lutou do lado do jogador NÃO está na coleção (ainda não capturado) THEN o sistema SHALL não conceder XP a ninguém — nenhuma instância fantasma é criada
4. WHEN a mesma vitória já capturou um pet NOVO (do oponente, feature anterior) THEN o sistema SHALL, adicionalmente, conceder XP ao pet do jogador se ele também já era possuído — os dois mecanismos (captura do oponente, XP do próprio pet) não se excluem

**Independent Test:** simular uma batalha com o pet do jogador já na coleção, terminar em vitória, confirmar que a instância correspondente ganhou XP; repetir terminando em derrota, confirmar XP menor mas ainda positivo.

---

### P1: XP acumulado sobe de nível por uma tabela determinística ⭐ MVP

**User Story:** Como jogador, quero ver meu pet subir de nível conforme luto, com uma progressão previsível.

**Acceptance Criteria:**
1. WHEN o XP acumulado de uma instância atinge o limiar do próximo nível THEN o sistema SHALL incrementar o nível dela
2. WHEN XP suficiente for concedido de uma vez para pular mais de um nível THEN o sistema SHALL processar todos os níveis ganhos, não só um
3. WHEN a instância já está no nível máximo THEN o sistema SHALL continuar aceitando XP (para não punir jogar mais) mas nunca subir além do teto

**Independent Test:** conceder XP suficiente para 3 níveis de uma vez a uma instância nível 1, confirmar que ela termina exatamente no nível esperado, não em nível 2.

---

### P1: Nível mais alto fortalece o pet em batalha, fora de `BattleSim` ⭐ MVP

**User Story:** Como jogador, quero que o nível do meu pet realmente importe no próximo combate, não só ser um número decorativo.

**Acceptance Criteria:**
1. WHEN um pet de nível N > 1 entra numa batalha THEN o sistema SHALL aplicar um bônus de atributo proporcional ao nível, na montagem da partida — nunca dentro do núcleo
2. WHEN o mesmo pet está no nível 1 (base) THEN o sistema SHALL aplicar exatamente os atributos de catálogo, sem bônus — zero regressão do comportamento de hoje

**Independent Test:** duas partidas com o mesmo `CatalogId`, uma como instância nível 1 e outra nível 5 (com os mesmos atributos base), produzem `Attack`/`Defense`/`Speed`/`MaxHealth` efetivos diferentes na montagem — comparação direta, sem precisar rodar combate.

---

### P2: Nível/XP visível na consulta da coleção

**User Story:** Como jogador, quero ver o nível e XP de cada pet quando consulto minha coleção, para acompanhar o progresso de cada um.

**Why P2:** o MVP já entrega o mecanismo funcionando; expor os números na consulta já existente (`LoadCollection`) é uma extensão pequena, não bloqueio.

**Acceptance Criteria:**
1. WHEN o jogador consulta a coleção THEN o sistema SHALL incluir nível e XP atual de cada instância na resposta

---

## Edge Cases

- WHEN os dois lados de uma batalha correspondem a pets já capturados pelo MESMO jogador (Standalone, jogador contra si mesmo via IA usando um catálogo que ele também possui) THEN o sistema SHALL conceder XP apenas à instância do lado do JOGADOR LOCAL — o "oponente" nunca ganha XP, mesmo que exista na coleção (ele não é quem o jogador está controlando)
- WHEN a batalha termina em abandono (Sala e Pareamento Simples) THEN o sistema SHALL tratar como resultado válido para XP, mesma regra de captura já estabelecida — o evento é o mesmo `BatalhaEncerrada`
- WHEN o cálculo de bônus de nível seria float por causa de arredondamento THEN o sistema SHALL usar apenas aritmética inteira, mesmo padrão de todo o resto do projeto (percentual inteiro, `Valor * Percentual / 100`)
- WHEN uma instância nunca recebeu XP (recém-capturada) THEN o sistema SHALL começar no nível 1 com o bônus zero — idêntico ao catálogo, consistente com o Success Criteria de zero regressão

---

## Decision Points (para o Design)

- **DP-nivel-01:** onde o gancho de XP se conecta — mesmo ponto de `ABattleArena::CheckForCapture` (Coleção e Captura), ou um método irmão separado? Design.md decide se compartilham a mesma varredura do trace ou são dois métodos.
- **DP-nivel-02:** valores de XP por vitória/derrota/empate, tabela de limiares de nível, teto de nível, e fórmula do bônus de atributo por nível — todos parâmetros de balanceamento, constantes nomeadas (DP-nivel do design, mesmo padrão de `BattleArenaConstants`/`BattleNetConstants`).
- **DP-nivel-03:** onde o bônus de nível é aplicado na montagem — dentro de `FBattleDataTranslator` (precisaria saber o nível na hora de traduzir) ou uma função nova que ajusta `FPetState` depois da tradução, antes do `FBattleResolver::ResolveTurn`. Afeta se `TranslateMatchup`/`TranslatePet` precisam de mais um parâmetro ou se fica um passo separado.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| NIVEL-01 | P1: Vitória concede XP ao pet do jogador na coleção | Specify | Pending |
| NIVEL-02 | P1: Vitória concede mais XP que derrota/empate | Specify | Pending |
| NIVEL-03 | P1: Pet não capturado não gera XP fantasma | Specify | Pending |
| NIVEL-04 | P1: Captura de oponente e XP do próprio pet coexistem | Specify | Pending |
| NIVEL-05 | P1: XP acumulado sobe de nível por tabela determinística | Specify | Pending |
| NIVEL-06 | P1: Múltiplos níveis de uma vez são processados corretamente | Specify | Pending |
| NIVEL-07 | P1: Nível máximo não é ultrapassado | Specify | Pending |
| NIVEL-08 | P1: Nível mais alto fortalece atributos na montagem | Specify | Pending |
| NIVEL-09 | P1: Nível 1 é idêntico ao catálogo, zero regressão | Specify | Pending |
| NIVEL-10 | P2: Nível/XP visível na consulta da coleção | Specify | Pending |

**Cobertura:** 10 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Uma instância que vence 5 batalhas seguidas termina com nível mais alto e atributos efetivos mensuravelmente maiores que uma instância idêntica que nunca lutou
- [ ] Conceder XP suficiente para pular 3 níveis de uma vez resulta no nível correto, não em incrementos perdidos
- [ ] Nenhuma instância ultrapassa o teto de nível, mesmo recebendo XP em excesso repetidamente
- [ ] `Tools/audit_determinism.sh` continua limpo — nenhum float introduzido no cálculo de bônus de nível
- [ ] Regressão zero: 70 testes de `BattleSquare` e 52 de `BattleSim` continuam passando; uma instância nível 1 (ou um pet ainda não capturado) produz exatamente o mesmo resultado de montagem de hoje
