# Combate Núcleo — Especificação

**Status:** Draft — DP-01 decidida; DP-02 a DP-05 seguem abertas (nenhuma bloqueia o Design)
**Neutra de motor:** esta spec descreve regras e contratos, não Unreal. A implementação é C++ sem dependência de `UWorld`, `AActor` ou tick.

---

## Problem Statement

Combate por turnos alternados é lento e previsível: você reage ao que já aconteceu e a habilidade vira decorar ordem de iniciativa. Queremos um combate onde os dois lados comprometem três ações **às cegas** e as veem colidir ao mesmo tempo, numa grade minúscula onde posição importa. A habilidade passa a ser ler a intenção do oponente.

O protótipo anterior tinha o modelo esboçado mas nunca uma regra fechada — o README e o código divergiam sobre como a resolução simultânea funciona. Esta spec fecha essa regra antes de qualquer linha de código.

## Goals

- [ ] Um turno completo resolve em menos de 10 segundos de apresentação
- [ ] Determinismo bit a bit: mesma seed + mesmas ações = mesmo trace, cliente e servidor
- [ ] Nenhuma regra de combate codificada por pet — tudo vem de dados
- [ ] Toda a resolução coberta por testes de snapshot rodando fora do editor

## Out of Scope

| Item | Razão |
|---|---|
| Mais de 1 pet por lado | A regra de resolução é a mesma; N pets é escala, não design. Entra em M3. |
| Tipos elementais, fraquezas e resistências | Multiplicador sobre a fórmula de dano. Não muda a estrutura da resolução. Entra em M3. |
| Casas com propriedades (bloqueio, dano, buff) | Arena homogênea na v1. |
| Progressão, experiência, captura | Marco 4. |
| Matchmaking | v1 usa código de sala. |
| IA competente | v1 tem oponente que sorteia ações válidas, só para poder jogar sozinho. |

---

## Modelo Conceitual

### Arena

Grade **3x3 compartilhada** pelos dois lados — não é uma grade por jogador. Casas indexadas `(coluna, linha)`, ambas de 0 a 2, origem no canto superior esquerdo do ponto de vista neutro.

**Coabitação é permitida:** mais de um pet pode ocupar a mesma casa. Casa compartilhada por pets de lados opostos é o que cria o corpo a corpo.

### Ação

Uma ação é o par **(Tipo, Direção)**.

| Tipo | Usa direção | Efeito |
|---|---|---|
| `MOVER` | sim | Desloca uma casa na direção |
| `ATACAR` | sim | Atinge a primeira casa naquela direção |
| `MAGIA` | sim | Atinge a primeira casa naquela direção, ignora esquiva |
| `DEFENDER` | não | Reduz dano recebido neste slot |
| `ESQUIVAR` | sim | Anula dano físico neste slot, sem sair da casa |
| `AGUARDAR` | não | Nada. É o preenchimento de slot vazio ou de timeout |

Direções são as 8: `CIMA`, `BAIXO`, `ESQUERDA`, `DIREITA` e as quatro diagonais.

> **Por que direcional.** Numa grade 3x3, "atingir casas adjacentes" a partir do centro cobre o tabuleiro inteiro e torna a posição irrelevante. Amarrar ataque a uma direção é o que faz a grade valer alguma coisa — e é a leitura mais forte do README original, que lista as direções *junto* das ações, não separadas delas.
>
> **DP-01 decidida em 2026-08-25: par (Tipo, Direção).** Esta seção deixa de ser proposta e passa a ser regra.

### Turno e Slot

Um **turno** é uma rodada completa: cada jogador enfileira **3 ações**, comete às cegas, e as ações resolvem em três **slots** consecutivos (slot 1, depois 2, depois 3). Terminados os três slots, se a batalha não acabou, começa um novo turno com novo commit.

### Fases dentro de um slot

Cada slot resolve em cinco fases, sempre nesta ordem:

| Fase | Nome | O que acontece |
|---|---|---|
| F1 | Declaração | Lê a ação de cada pet vivo para este slot. Nada muda. |
| F2 | Postura | `DEFENDER` e `ESQUIVAR` entram em vigor. Valem só neste slot. |
| F3 | Movimento | Todas as intenções de `MOVER` resolvem **simultaneamente**. |
| F4 | Combate | `ATACAR` e `MAGIA` resolvem sobre as posições **pós-movimento**. O dano é **acumulado, não aplicado**. |
| F5 | Encerramento | O dano acumulado é aplicado de uma vez. Mortes são verificadas. Posturas expiram. Eventos são emitidos. |

> **Por que o dano é acumulado em F4 e aplicado em F5.** É o que torna a simultaneidade real: dois pets que se matam no mesmo slot morrem os dois. Se o dano fosse aplicado na hora, quem foi processado primeiro venceria — e a ordem do array decidiria a partida.

### Trace de Eventos

A resolução não devolve só o estado final: devolve uma **lista ordenada e tipada de eventos** (`moveu`, `bloqueado`, `atingiu`, `defendeu`, `esquivou`, `morreu`, `turno_encerrado`). O cliente **anima o trace**; ele não recalcula nada. Se a animação precisa de um número, esse número veio no trace.

Isso entrega replay, espectador e reconexão sem trabalho adicional.

---

## User Stories

### P1: Enfileirar três ações às cegas ⭐ MVP

**User Story:** Como jogador, quero escolher três ações para meu pet sem ver as do oponente, para que a partida seja uma leitura de intenção e não uma reação.

**Why P1:** é a mecânica que define o jogo. Sem ela não há jogo.

**Acceptance Criteria:**
1. WHEN o jogador seleciona uma ação válida e a fila tem menos de 3 THEN o sistema SHALL acrescentar a ação ao fim da fila e informar a contagem atualizada
2. WHEN a fila já tem 3 ações THEN o sistema SHALL recusar novas ações e indicar o limite atingido
3. WHEN o jogador remove a última ação THEN o sistema SHALL desfazer apenas ela, preservando as anteriores
4. WHEN o turno está em fase de commit THEN o sistema SHALL NOT revelar nenhuma informação sobre as ações do oponente
5. WHEN o jogador confirma o commit THEN o sistema SHALL travar a fila até o fim do turno

**Independent Test:** enfileirar, remover, tentar a quarta ação e confirmar — tudo verificável sem resolução nem rede.

---

### P2: Resolver o turno simultaneamente ⭐ MVP

**User Story:** Como jogador, quero que minhas três ações e as do oponente resolvam ao mesmo tempo, slot a slot, para que o resultado seja consequência das duas decisões.

**Why P1:** é o núcleo da regra. (Prioridade P1 apesar do rótulo da seção.)

**Acceptance Criteria:**
1. WHEN os dois lados commitaram THEN o sistema SHALL resolver os slots 1, 2 e 3 em ordem, cada um pelas cinco fases
2. WHEN um pet declara `MOVER` para fora da grade THEN o sistema SHALL anular o movimento, manter a posição e emitir evento `bloqueado`
3. WHEN dois pets do mesmo lado declaram movimento para a mesma casa THEN o sistema SHALL anular ambos os movimentos e emitir `bloqueado` para os dois
4. WHEN pets de lados opostos ocupam a mesma casa ao fim de F3 THEN o sistema SHALL permitir a coabitação
5. WHEN dois pets trocam de casa no mesmo slot THEN o sistema SHALL permitir a troca sem colisão
6. WHEN um ataque e um contra-ataque se abatem no mesmo slot THEN o sistema SHALL aplicar os dois danos em F5, mesmo que ambos os pets morram
7. WHEN um pet morre em F5 THEN o sistema SHALL descartar suas ações dos slots restantes do turno
8. WHEN a resolução termina THEN o sistema SHALL devolver o estado final e o trace completo de eventos

**Independent Test:** alimentar a simulação com dois conjuntos fixos de ações e comparar o trace com um snapshot gravado. Roda headless.

---

### P3: Sentir o triângulo ataque / defesa / esquiva ⭐ MVP

**User Story:** Como jogador, quero que defender e esquivar tenham contrapartidas distintas, para que adivinhar o tipo da ação do oponente importe tanto quanto adivinhar a direção.

**Why P1:** sem contrapartida, atacar sempre é a jogada ótima e o commit às cegas perde a tensão.

**Acceptance Criteria:**
1. WHEN um pet com `ESQUIVAR` ativo recebe um `ATACAR` neste slot THEN o sistema SHALL anular o dano integralmente e emitir `esquivou`
2. WHEN um pet com `ESQUIVAR` ativo recebe uma `MAGIA` neste slot THEN o sistema SHALL aplicar o dano integralmente
3. WHEN um pet com `DEFENDER` ativo recebe qualquer dano neste slot THEN o sistema SHALL reduzir o dano pelo fator de defesa e emitir `defendeu`
4. WHEN o slot termina THEN o sistema SHALL expirar toda postura, sem acúmulo entre slots
5. WHEN o dano calculado é menor que o mínimo THEN o sistema SHALL aplicar o dano mínimo, nunca zero nem negativo

> Triângulo pretendido: **Ataque** fura Defesa por atrito, **Magia** fura Esquiva, **Esquiva** anula Ataque. Os números são balanceamento (M3), não estrutura.

**Independent Test:** matriz de 5 tipos contra 5 tipos, resultado esperado tabelado.

---

### P4: Encerrar a batalha com resultado claro

**User Story:** Como jogador, quero saber sem ambiguidade se venci, perdi ou empatei.

**Acceptance Criteria:**
1. WHEN todos os pets de um lado estão com vida zerada e os do outro não THEN o sistema SHALL declarar o lado sobrevivente vencedor
2. WHEN todos os pets dos dois lados zeram a vida no mesmo F5 THEN o sistema SHALL declarar empate
3. WHEN os três slots resolvem e os dois lados seguem vivos THEN o sistema SHALL iniciar um novo turno de commit
4. WHEN o limite de turnos é atingido THEN o sistema SHALL declarar vencedor o lado com maior percentual de vida restante, e empate se forem iguais

---

### P5: Determinismo verificável

**User Story:** Como desenvolvedor, quero que a simulação seja determinística, para que servidor autoritativo, replay e testes de regressão sejam possíveis.

**Acceptance Criteria:**
1. WHEN a simulação roda duas vezes com a mesma seed e as mesmas ações THEN o sistema SHALL produzir traces idênticos
2. WHEN há empate em qualquer critério de ordenação THEN o sistema SHALL desempatar por chave estável (velocidade, depois identificador do pet), nunca por ordem de contêiner
3. WHEN a simulação precisa de aleatoriedade THEN o sistema SHALL usar exclusivamente o gerador semeado do estado de batalha
4. WHEN qualquer cálculo de combate é feito THEN o sistema SHALL usar apenas aritmética inteira ou de ponto fixo

**Independent Test:** rodar 10.000 combates com seeds fixas e comparar o hash dos traces entre execuções e entre plataformas.

---

### P6: Autoridade do servidor (M2)

**User Story:** Como jogador, quero que o resultado venha do servidor, para que ninguém possa trapacear.

**Acceptance Criteria:**
1. WHEN o cliente comete THEN o sistema SHALL enviar apenas a lista de três ações, nunca posições, dano ou resultado
2. WHEN o servidor recebe os dois commits THEN o sistema SHALL simular e transmitir o trace a ambos os clientes
3. WHEN um jogador não comete dentro do tempo limite THEN o sistema SHALL preencher os slots faltantes com `AGUARDAR` e prosseguir
4. WHEN um cliente reconecta THEN o sistema SHALL reenviar o estado e o trace dos turnos que ele perdeu
5. WHEN um cliente envia uma ação inválida THEN o sistema SHALL rejeitar o commit inteiro e tratá-lo como ausente

---

### P7: Apresentação dirigida pelo trace

**User Story:** Como jogador, quero ver o que aconteceu de forma legível, entendendo por que perdi vida.

**Acceptance Criteria:**
1. WHEN o trace chega THEN o sistema SHALL animar os eventos na ordem recebida, respeitando fronteiras de slot
2. WHEN dois eventos pertencem à mesma fase THEN o sistema SHALL apresentá-los simultaneamente, não em sequência
3. WHEN a apresentação precisa de qualquer valor numérico THEN o sistema SHALL lê-lo do trace, nunca recalculá-lo
4. WHEN o jogador pula a animação THEN o sistema SHALL saltar ao estado final sem divergir do trace

---

## Edge Cases

- WHEN um pet declara `ATACAR` numa direção sem alvo THEN o sistema SHALL emitir `errou` sem dano
- WHEN um pet morre em F5 do slot 1 THEN o sistema SHALL manter suas ações dos slots 2 e 3 descartadas, sem preencher com `AGUARDAR`
- WHEN os dois lados commitam apenas `AGUARDAR` por um turno inteiro THEN o sistema SHALL avançar o contador de turnos normalmente (protege contra impasse infinito via limite de turnos)
- WHEN um pet ataca uma casa ocupada por um aliado THEN o sistema SHALL — ver **DP-03**
- WHEN a fila é commitada com menos de 3 ações THEN o sistema SHALL completar com `AGUARDAR` até 3
- WHEN os dois jogadores desconectam THEN o sistema SHALL encerrar a sala sem resultado registrado
- WHEN um pet tem velocidade igual à do oponente e ambos disputam o mesmo desempate THEN o sistema SHALL usar o identificador do pet como critério final

---

## Decisões Pendentes

Áreas cinzentas onde existe mais de um caminho válido. Cada uma tem uma proposta; **confirmar antes do Design**.

### ~~DP-01: Ação é par (Tipo, Direção) ou lista plana de 12 ações?~~ ✅ DECIDIDA (2026-08-25)

**Resolvida:** ação é o par `(Tipo, Direção)`. Ver **AD-009** em STATE.md.
A alternativa de 12 ações planas foi descartada porque tornaria o ataque omnidirecional e a posição sem valor tático numa grade 3x3.

### DP-02: Coabitação de pets na mesma casa

- **Proposta:** permitida entre lados opostos, proibida entre aliados. É a leitura do README ("mesma posição gera ataque mútuo") e evita empilhar aliados numa casa.
- **Alternativa:** casa exclusiva, com bloqueio de entrada. Torna a posição mais tática, mas exige regra de desempate para quem entra primeiro.
- **Impacto se mudar depois:** médio. Afeta F3 e F4.

### DP-03: Fogo amigo

- **Proposta:** ataque em casa ocupada por aliado **não** causa dano — atravessa. Simplifica a v1 (1v1) e evita frustração.
- **Alternativa:** fogo amigo ativo. Adiciona profundidade tática quando houver 3v3, mas só faz sentido a partir de M3.
- **Impacto se mudar depois:** baixo enquanto for 1v1.

### DP-04: Alcance do ataque direcional

- **Proposta:** atinge a **primeira casa** na direção (alcance 1), mais a própria casa se houver oponente coabitando.
- **Alternativa:** atinge todas as casas na linha da direção (alcance infinito na reta). Numa grade 3x3 isso é forte demais.
- **Impacto se mudar depois:** baixo. É parâmetro de dados, não estrutura — desde que o alcance venha do DataAsset da skill.

### DP-05: Limite de turnos

- **Proposta:** 10 turnos (30 ações por lado). Suficiente para o alvo de 90 segundos por partida.
- **Impacto:** apenas balanceamento.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| BTL-01 | P1: Fila de 3 ações com limite | Specify | Pending |
| BTL-02 | P1: Commit trava a fila e esconde do oponente | Specify | Pending |
| BTL-03 | P2: Resolução em 3 slots, 5 fases | Specify | Pending |
| BTL-04 | P2: Movimento fora da grade é anulado | Specify | Pending |
| BTL-05 | P2: Colisão de movimento entre aliados | Specify | Pending |
| BTL-06 | P2: Coabitação entre lados opostos | Specify | Pending |
| BTL-07 | P2: Dano acumulado em F4, aplicado em F5 | Specify | Pending |
| BTL-08 | P2: Trace de eventos como saída da resolução | Specify | Pending |
| BTL-09 | P3: Esquiva anula ataque físico | Specify | Pending |
| BTL-10 | P3: Magia ignora esquiva | Specify | Pending |
| BTL-11 | P3: Defesa reduz todo dano | Specify | Pending |
| BTL-12 | P3: Postura expira ao fim do slot | Specify | Pending |
| BTL-13 | P3: Dano mínimo garantido | Specify | Pending |
| BTL-14 | P4: Vitória, derrota e empate | Specify | Pending |
| BTL-15 | P4: Limite de turnos e desempate por vida | Specify | Pending |
| BTL-16 | P5: Traces idênticos para mesma seed | Specify | Pending |
| BTL-17 | P5: Desempate por chave estável | Specify | Pending |
| BTL-18 | P5: Aritmética inteira apenas | Specify | Pending |
| BTL-19 | P6: Cliente envia só o commit | Specify | Pending |
| BTL-20 | P6: Timeout preenche com AGUARDAR | Specify | Pending |
| BTL-21 | P6: Reconexão com replay do trace | Specify | Pending |
| BTL-22 | P7: Apresentação anima o trace, não recalcula | Specify | Pending |

**Cobertura:** 22 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Uma partida completa em menos de 90 segundos
- [ ] 10.000 combates headless com seeds fixas produzem traces de hash idêntico entre execuções
- [ ] Nenhum `float` no caminho de resolução, verificado por revisão e por teste entre plataformas
- [ ] Um jogador novo entende por que perdeu vida, sem explicação externa, em 3 de 4 sessões observadas
- [ ] Adicionar um pet novo não exige nenhuma linha de código
