# Sala e Pareamento Simples — Especificação

**Status:** Draft — aguarda aprovação
**Depende de:** Combate Online (concluído) — esta feature é o que finalmente coloca dois jogadores reais no MESMO `ABattleArena`. Sem ela, `UBattleTurnCoordinator`/`UBattleNetCommitComponent` existem e são testados, mas nada no jogo os invoca de verdade: não há `AGameModeBase`, `APlayerController` nem `APawn` — a sessão de rede não existe ainda (ver `.specs/project/STATE.md`, blocker B-004 é sobre o *binário* do servidor; esta lacuna é sobre a *sessão de jogo em cima dele*, e é ortogonal).

---

## Problem Statement

Combate Online resolveu **como** dois commits se tornam um turno resolvido com segurança. Não resolveu **como dois jogadores acabam na mesma partida**. Hoje, literalmente não existe um caminho — nenhum `AGameModeBase` decide quem é jogador 1 e quem é jogador 2, nenhum `APlayerController` possui um `ABattleArena`, e não existe UI nem comando para "entrar numa partida".

Esta feature fecha essa lacuna com o menor pareamento que ainda é um jogo de verdade: **um código de sala**. Um jogador cria uma sala e recebe um código; o outro digita o código e entra. Sem fila, sem ranking, sem servidor de matchmaking — o código de sala É o pareamento.

## Goals

- [ ] Um jogador cria uma sala e recebe um código curto, memorizável, que pode compartilhar por qualquer canal fora do jogo (voz, texto, etc.)
- [ ] Um segundo jogador digita o código e entra na mesma partida, sem passar por fila nem espera
- [ ] Ao entrar os dois, o `ABattleArena` é montado com um `UBattleTurnCoordinator` real conectando os dois lados — não mais um coordenador de teste criado à mão
- [ ] Um jogador que abandona (fecha o jogo, cai da rede e não volta) é detectado pela sessão, e o `UBattleTurnCoordinator::DeclareAbandonment` (já existente, Combate Online) é acionado de verdade — hoje ele só é testável chamando a função diretamente
- [ ] Nenhuma conta, login ou identidade persistente é necessária para jogar uma partida — o código de sala é a única credencial

## Out of Scope

| Item | Razão |
|---|---|
| Matchmaking por habilidade/ranking (MMR, fila automática) | Já era Out of Scope da spec de Combate Online, pelo mesmo motivo — decisão de operação/produto, não de arquitetura de sessão |
| Contas de jogador, autenticação persistente | M7 — o código de sala é deliberadamente efêmero e anônimo |
| Reconexão automática sem o código (ex.: "voltar sozinho" sem redigitar) | A lógica de reconexão do Combate Online (`GetCurrentBattleState`) já existe; **como** o cliente prova que é o mesmo jogador para reconectar é desta feature, mas um mecanismo robusto (token de sessão) é mais que o mínimo — ver Decision Points |
| Espectador entrando numa sala já cheia | Sala é estritamente 2 jogadores; espectador é feature futura, não obstruída por esta |
| UI final (fontes, layout, tema visual) da tela de lobby | Mesma separação lógica/visual da Apresentação do Combate — esta spec cobre o C++ testável; o UMG é autoria posterior |
| Múltiplas salas simultâneas no mesmo servidor testadas sob carga | Funcional para N salas por construção (cada sala é independente), mas performance sob carga real é fora do escopo de uma spec de gameplay |

---

## User Stories

### P1: Criar uma sala e receber um código ⭐ MVP

**User Story:** Como jogador, quero criar uma sala e receber um código curto, para convidar um amigo a jogar comigo sem precisar de conta nem fila.

**Acceptance Criteria:**
1. WHEN um jogador pede para criar uma sala THEN o sistema SHALL gerar um código curto (formato a definir em design.md — ex.: 4-6 caracteres alfanuméricos, sem caracteres ambíguos como `0`/`O`, `1`/`I`)
2. WHEN uma sala é criada THEN o sistema SHALL colocar o jogador criador nela como o único ocupante, aguardando o segundo
3. WHEN dois códigos são gerados em sequência THEN o sistema SHALL garantir que não colidem enquanto ambas as salas estiverem ativas (unicidade só precisa valer entre salas vivas — um código pode ser reciclado depois que a sala dele fecha)

**Independent Test:** criar uma sala, receber um código, confirmar que ele aparece em algum estado consultável (log, ou propriedade replicada), sem um segundo jogador ainda ter entrado.

---

### P1: Entrar numa sala existente pelo código ⭐ MVP

**User Story:** Como segundo jogador, quero digitar o código que recebi e entrar direto na partida, sem esperar nem escolher nada.

**Acceptance Criteria:**
1. WHEN um jogador informa um código válido de uma sala com 1 ocupante THEN o sistema SHALL colocá-lo como o segundo jogador daquela sala
2. WHEN um jogador informa um código inválido (não existe, ou sala já cheia) THEN o sistema SHALL recusar com um motivo claro (não encontrado vs. sala cheia são casos distintos, tratados diferente na UI depois)
3. WHEN a sala atinge 2 ocupantes THEN o sistema SHALL disparar o início da partida automaticamente — sem passo manual de "pronto?"/"iniciar" (mantém o MVP mínimo; um botão de "pronto" é melhoria futura, não bloqueio)

**Independent Test:** com uma sala já criada (código conhecido), um segundo processo/cliente entra usando o código e o sistema reporta 2/2 ocupantes.

---

### P1: Sala montada vira uma partida real de `ABattleArena` ⭐ MVP

**User Story:** Como jogador, quero que assim que a sala estiver cheia, a partida comece de verdade — com o `ABattleArena`, o `UBattleTurnCoordinator` real e os dois `UBattleNetCommitComponent` conectados aos dois jogadores, para que tudo que o Combate Online já construiu passe a valer fora de teste.

**Acceptance Criteria:**
1. WHEN uma sala atinge 2 ocupantes THEN o sistema SHALL instanciar (ou reaproveitar) um `ABattleArena`, criar um `UBattleTurnCoordinator`, e conectar os dois jogadores via `ConfigureNetworkedOpponent`/`SetServerCoordinator` (já existentes)
2. WHEN a partida começa THEN o sistema SHALL atribuir Side 0 e Side 1 de forma determinística e sem ambiguidade (ex.: quem criou a sala é sempre Side 0)
3. WHEN a partida começa THEN o sistema SHALL montar o `FBattleState` inicial com pets reais (reaproveitando `FBattleDataTranslator`/`PetDataLoader`, já existentes do Backend de Dados de Pet) — não um estado fabricado à mão como nos testes

**Independent Test:** duas sessões entrando na mesma sala resultam num turno resolvível de ponta a ponta, sem nenhuma chamada de teste direta ao coordenador — só through a sala.

---

### P1: Abandono é detectado pela sessão, não só testável por chamada direta ⭐ MVP

**User Story:** Como jogador cujo oponente sumiu, quero que o jogo perceba isso sozinho (sem eu precisar fazer nada) e me declare vencedor depois do prazo, para que `DeclareAbandonment` (que já existe desde Combate Online) seja acionado pela realidade da sessão, não só por um teste chamando a função.

**Acceptance Criteria:**
1. WHEN um jogador se desconecta (fecha o cliente, perde a conexão) THEN o sistema SHALL registrar o horário da desconexão para aquela sala
2. WHEN o jogador desconectado não reconecta dentro de `AbandonTimeoutSeconds` (já existente, Combate Online) THEN o sistema SHALL chamar `UBattleTurnCoordinator::DeclareAbandonment` com o lado presente
3. WHEN o jogador desconectado reconecta ANTES do timeout de abandono, usando o mesmo código de sala THEN o sistema SHALL tratá-lo como o mesmo jogador, reconstruindo a visão dele via `GetCurrentBattleState` (já existente)

**Independent Test:** derrubar a conexão de um cliente de teste, aguardar o tempo injetado passar de `AbandonTimeoutSeconds`, confirmar que `DeclareAbandonment` foi chamado automaticamente pela sessão — não por uma chamada de teste direta ao coordenador.

---

### P2: Sala fecha sozinha quando ninguém mais está nela

**User Story:** Como operador do servidor, quero que salas abandonadas por completo (os dois jogadores saíram) sejam limpas automaticamente, para que o servidor não acumule sessões mortas.

**Why P2:** não bloqueia o jogo funcionar para os dois jogadores presentes; é limpeza de recurso, importante para rodar por muito tempo mas não para provar o conceito.

**Acceptance Criteria:**
1. WHEN os dois jogadores de uma sala se desconectam e nenhum reconecta dentro do limite THEN o sistema SHALL destruir a sala e liberar o código para reuso

---

## Edge Cases

- WHEN dois jogadores tentam entrar no MESMO código simultaneamente, e a sala só tem 1 vaga THEN o sistema SHALL aceitar só um (o primeiro a ser processado no servidor) e recusar o outro com "sala cheia" — nunca aceitar os dois e estourar o commit para 3 lados
- WHEN um jogador tenta criar uma sala enquanto já está em outra THEN o sistema SHALL decidir uma regra clara (ex.: recusar, ou sair da anterior automaticamente) — a definir em design.md, mas nunca deixar o jogador em duas salas ao mesmo tempo
- WHEN o código de sala é digitado com letras minúsculas/maiúsculas misturadas THEN o sistema SHALL normalizar (case-insensitive) — código de sala é para ser digitado rápido, não é senha
- WHEN a partida termina (BatalhaEncerrada, por qualquer motivo — vitória, empate, abandono) THEN o sistema SHALL manter a sala viva por um período curto (para os jogadores verem o resultado) e então fechá-la — não travar o código para sempre

---

## Decision Points (para o Design)

- **DP-sala-01:** onde a sala vive — um `AGameModeBase`/subsistema no processo do servidor de combate (mesmo processo que já roda `UBattleTurnCoordinator`) vs. um serviço de lobby separado. Repete a mesma pergunta de DP-online-02 (Combate Online): dado que não há conta de jogador nem necessidade de persistência entre partidas, um subsistema no próprio processo do jogo é provavelmente suficiente — mas registrar a decisão explicitamente.
- **DP-sala-02:** geração e armazenamento do código — em memória (perdido se o servidor reiniciar, aceitável dado que salas são efêmeras) vs. persistido. Piso proposto: em memória.
- **DP-sala-03:** atribuição de Side 0/Side 1 — "quem cria é sempre Side 0" (simples, determinístico) é a proposta desta spec; design.md confirma ou revisita.
- **DP-sala-04:** como a UI entrega o código (copiar para área de transferência, exibir em texto grande) — decisão de UX leve, não bloqueia a lógica.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| SALA-01 | P1: Criar sala, receber código | Specify | Pending |
| SALA-02 | P1: Unicidade de código entre salas vivas | Specify | Pending |
| SALA-03 | P1: Entrar numa sala pelo código | Specify | Pending |
| SALA-04 | P1: Recusa clara para código inválido/sala cheia | Specify | Pending |
| SALA-05 | P1: Início automático ao completar 2/2 | Specify | Pending |
| SALA-06 | P1: Sala cheia monta ABattleArena + UBattleTurnCoordinator reais | Specify | Pending |
| SALA-07 | P1: Atribuição determinística de Side 0/Side 1 | Specify | Pending |
| SALA-08 | P1: Estado inicial com pets reais (reaproveitando backend de dados) | Specify | Pending |
| SALA-09 | P1: Desconexão registrada, aciona DeclareAbandonment real após timeout | Specify | Pending |
| SALA-10 | P1: Reconexão pelo código antes do timeout de abandono | Specify | Pending |
| SALA-11 | P2: Sala fecha sozinha quando abandonada por completo | Specify | Pending |

**Cobertura:** 11 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Dois processos de teste, sem nenhuma chamada direta a `UBattleTurnCoordinator`/`ConfigureNetworkedOpponent`, terminam numa partida resolvível só usando "criar sala" + "entrar com código"
- [ ] Um cliente de teste que derruba a conexão e não volta gera `DeclareAbandonment` automaticamente, sem intervenção de teste
- [ ] Um código de sala reutilizado depois que a sala original fechou funciona para uma sala nova, sem colisão
- [ ] Nenhuma conta, senha ou identidade persistente é criada ou exigida em nenhum ponto do fluxo
