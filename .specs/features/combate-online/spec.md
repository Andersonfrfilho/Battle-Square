# Combate Online — Especificação

**Status:** Draft — aguarda aprovação
**Depende de:** Combate Núcleo (concluído), Backend de Dados de Pet (concluído), Apresentação do Combate (concluída) — esta feature substitui o `FDumbOpponentAI` local por um oponente humano real, sem tocar no núcleo determinístico nem na apresentação.

---

## Problem Statement

Hoje um turno se resolve inteiramente num processo: o jogador monta o commit, `FDumbOpponentAI` gera o commit do "oponente" no mesmo instante, e `FBattleResolver::ResolveTurn` roda local — não existe rede em lugar nenhum (ver `ABattleArena::HandlePlayerCommitted`, Apresentação do Combate). Esta feature troca a origem do segundo commit: em vez de uma IA burra decidindo na hora, um segundo jogador real decide às cegas, em outra máquina, sem ver a fila do primeiro (BTL-02 já garante isso na apresentação — falta garantir que a rede não vaze o commit antes da hora).

**Requisito explícito do projeto: isto não pode custar o modo local.** M1 (offline contra IA) já funciona e é o modo de desenvolvimento/demonstração mais barato que existe — a Unreal resolve isso de fábrica com `NetMode`: o mesmo `AGameModeBase`/replicação que atende dois jogadores reais num `Listen Server` também atende `Standalone` sem servidor nenhum, sem branch de código separado. A feature é "adicionar replicação", não "reescrever para depender de rede".

O segundo problema, greenfield deste projeto: **onde a simulação roda**. AD-014 já decidiu que o servidor de combate lê pets de um espelho local, nunca da rede em tempo real — essa fronteira continua igual. O que muda é *quem* é o servidor: hoje é o mesmo processo do cliente (não existe separação); esta feature introduz a separação autoritativa (design.md decide a topologia exata — Listen Server vs Dedicated Server, ver Decision Points).

## Goals

- [ ] Dois jogadores reais, em máquinas diferentes, jogam um combate 1v1 completo, um turno de cada vez, às cegas um do outro
- [ ] O modo offline contra `FDumbOpponentAI` continua funcionando sem nenhuma mudança de código do lado do jogador — é o mesmo `ABattleArena`, outro `NetMode`
- [ ] A resolução do turno continua acontecendo em UM lugar de confiança (autoritativo) — nenhum cliente pode alterar o resultado enviando um trace fabricado
- [ ] Um jogador que cai da rede no meio de uma partida consegue voltar e ver o estado real, sem que o outro jogador perca a partida por causa disso

## Out of Scope

| Item | Razão |
|---|---|
| Matchmaking real (fila, ranking, MMR) | Fora do escopo — esta feature é sala com código, não matchmaking competitivo (ver Sala e Pareamento Simples, ROADMAP M2) |
| Infraestrutura de deploy de dedicated server (containers, orquestração, regiões) | Decisão de operação, não de design de jogo — registrar como blocker se vier a bloquear, não especificar aqui |
| Chat / comunicação entre jogadores | Não faz parte do ciclo de combate |
| Espectador / replay assistido por terceiros | Reconexão do próprio jogador é suficiente para este marco |
| Contas de jogador, autenticação real | M7 (Contas e Moderação) — esta feature usa identificação de sessão efêmera, não login |
| Anti-cheat de detecção de bot/macro | Fora de escopo — o que já existe (BTL server-autoritativo, verificação de assinatura do espelho de pets) continua sendo a defesa; não se adiciona detecção de comportamento aqui |

---

## User Stories

### P1: Dois jogadores reais resolvem um turno às cegas ⭐ MVP

**User Story:** Como jogador, quero que meu commit de 3 ações viaje para o servidor sem que o oponente veja nada antes da resolução, para que o combate às cegas (AD-005/BTL-02) continue valendo quando o oponente é uma pessoa real.

**Why P1:** é a própria razão de existir da feature — sem isso, não há "online", só um menu que não faz nada.

**Acceptance Criteria:**
1. WHEN um jogador confirma o commit (mesmo fluxo de `UBattleActionQueueComponent::Commit`, já existente) THEN o sistema SHALL enviar o `FTurnCommit` para o servidor autoritativo, nunca para o outro cliente diretamente
2. WHEN o servidor recebe o commit de só um dos dois jogadores THEN o sistema SHALL aguardar o segundo, sem revelar o primeiro a ninguém — nem ao próprio jogador que já commitou
3. WHEN o servidor recebe os dois commits THEN o sistema SHALL rodar `FBattleResolver::ResolveTurn` (o resolvedor já existente, sem modificação) e replicar o `FBattleState` + trace resultantes para os dois clientes
4. WHEN um cliente recebe o trace do servidor THEN o sistema SHALL alimentá-lo em `UBattleTracePlayer` (já existente) exatamente como hoje alimenta o trace local — a apresentação não sabe se o oponente foi IA ou jogador real

**Independent Test:** dois processos do jogo (Listen Server + um cliente, ou dois clientes contra um dedicated server local) resolvem um turno completo, cada lado só vendo o resultado depois dos dois commitarem.

---

### P1: Timeout de commit não trava a partida ⭐ MVP

**User Story:** Como jogador, quero que a partida continue mesmo se o oponente demorar demais para decidir, para que uma pessoa AFK não prenda a outra indefinidamente.

**Acceptance Criteria:**
1. WHEN um jogador não commita dentro do tempo limite do turno THEN o sistema SHALL preencher o commit dele automaticamente com `Aguardar` nas 3 ações (mesma regra de preenchimento automático já existente em `UBattleActionQueueComponent::Commit`, aplicada pelo servidor no lugar do jogador ausente)
2. WHEN o preenchimento automático por timeout acontece THEN o sistema SHALL registrar isso no trace ou em um canal separado, de forma que a apresentação possa (opcionalmente) indicar "oponente demorou", sem inventar um evento que o núcleo não emite

---

### P1: Reconexão não perde a partida ⭐ MVP

**User Story:** Como jogador que caiu da rede, quero voltar para a mesma partida em andamento com o estado real, para não perder o progresso do combate por um problema de conexão.

**Acceptance Criteria:**
1. WHEN um jogador reconecta a uma partida em andamento THEN o sistema SHALL enviar o `FBattleState` atual e permitir reconstruir a visão local (spawn de `APetView`, `SetInitialState`) a partir dele — sem replay do trace acumulado, ver DP-online-03
2. WHEN um jogador está desconectado THEN o sistema SHALL continuar aplicando a regra de timeout de commit para o lado dele — a ausência por queda de rede não é distinguível de AFK do ponto de vista do turno
3. WHEN os dois jogadores ficam desconectados por tempo maior que um limite THEN o sistema SHALL encerrar a partida (edge case — ver Edge Cases)

---

### P1: Modo local continua funcionando sem servidor dedicado ⭐ MVP

**User Story:** Como desenvolvedor (e como jogador offline), quero continuar jogando contra `FDumbOpponentAI` sem precisar de nenhuma infraestrutura de rede, para que o modo M1 não regrida com a chegada do M2.

**Acceptance Criteria:**
1. WHEN o jogo roda em `NetMode::Standalone` (sem servidor separado) THEN o sistema SHALL resolver o turno exatamente como hoje — mesmo `ABattleArena::HandlePlayerCommitted`, mesma chamada a `FDumbOpponentAI::GenerateRandomValidCommit`
2. WHEN a replicação é adicionada a `ABattleArena`/`UBattleActionQueueComponent` THEN o sistema SHALL usar os mecanismos nativos de replicação da Unreal (RPCs, `UPROPERTY(Replicated)`) de forma condicional a `NetMode`, nunca duplicando a lógica de resolução em dois caminhos de código diferentes

---

### P2: Abandono é tratado sem travar a UI do jogador que ficou

**User Story:** Como jogador cujo oponente desistiu, quero uma indicação clara de abandono e um jeito de encerrar a partida, para não ficar esperando um commit que nunca vai chegar.

**Why P2:** o timeout de commit (P1 acima) já evita o travamento turno a turno; isto cobre o caso "o outro foi embora de vez", que é uma UX melhor, não uma trava funcional.

**Acceptance Criteria:**
1. WHEN um jogador se desconecta e não reconecta dentro do limite (ver Edge Cases) THEN o sistema SHALL declarar vitória por abandono ao jogador que ficou, usando o mesmo evento `BatalhaEncerrada` (não um evento novo) com o `WinningSide` do jogador presente

---

## Edge Cases

- WHEN os dois jogadores commitam no mesmo instante (corrida) THEN o sistema SHALL processar ambos os commits recebidos antes de rodar `ResolveTurn` — nunca resolver com um commit e completar o outro por timeout se ele já chegou, só atrasado na rede
- WHEN um cliente tenta enviar um `FTurnCommit` depois de já ter commitado no turno THEN o sistema SHALL rejeitar (idempotência — o servidor é quem trava, não confia no estado do cliente)
- WHEN um cliente envia um commit malformado (índice de ação fora do range, tipo inválido) THEN o sistema SHALL validar no servidor antes de alimentar o resolvedor — nunca deixar dado não confiável tocar `FBattleResolver`
- WHEN os dois jogadores ficam desconectados simultaneamente por mais que o limite de abandono (a definir em design.md) THEN o sistema SHALL encerrar a partida sem vencedor (mesmo `WinningSide = 0xFF` de empate já usado por `BattleOutcome`), nunca travar a sessão do servidor esperando alguém que não volta
- WHEN a partida atinge o limite de turnos (`MaxTurns`, já existente no núcleo) THEN o sistema SHALL aplicar exatamente a mesma regra de desempate por percentual de vida já implementada — nenhuma regra nova de "quem tem mais tempo de conexão" ou critério de rede entra no resultado

---

## Decision Points (para o Design)

Registrados aqui para não perder o fio — resolvidos em design.md, não nesta spec:

- **DP-online-01:** topologia de servidor — Listen Server (um dos dois jogadores hospeda) vs Dedicated Server (processo separado, sem jogador nenhum hospedando). Afeta diretamente o Goal "servidor de confiança": um Listen Server dá ao host acesso de memória ao processo autoritativo, o que é uma superfície de trapaça que Dedicated Server não tem.
- **DP-online-02:** protocolo de transporte do commit — RPC nativo da Unreal (`Server_SubmitCommit`, replicado sobre o `NetDriver` padrão) vs um serviço HTTP/WebSocket separado do backend de pets (`apps/api-battle-pets`). RPC nativo é mais barato de implementar; serviço separado abriria caminho para matchmaking futuro sem depender do `NetDriver` de jogo.
- **DP-online-03:** forma de reconexão — reenviar `FBattleState` completo (mais simples, "foto" do momento) vs reenviar o trace acumulado desde o início da partida (mais fiel para uma apresentação que "recupera a animação perdida", mas mais dado e mais complexidade). A spec já assume a opção mais simples (estado completo) como piso aceitável — a design.md pode revisitar se a apresentação exigir mais.
- **DP-online-04:** valor do timeout de commit e do limite de abandono — parâmetro de balanceamento, não uma decisão de arquitetura; registrar em design.md como constante nomeada, mesmo padrão de `MaxTurns`.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| NET-01 | P1: Commit viaja só para o servidor, nunca ao outro cliente | Specify | Pending |
| NET-02 | P1: Servidor aguarda os dois commits antes de revelar qualquer coisa | Specify | Pending |
| NET-03 | P1: Servidor roda o resolvedor real e replica estado+trace | Specify | Pending |
| NET-04 | P1: Cliente alimenta o trace recebido no `UBattleTracePlayer` já existente | Specify | Pending |
| NET-05 | P1: Timeout de commit preenche automaticamente com Aguardar | Specify | Pending |
| NET-06 | P1: Timeout é registrado sem inventar evento fora do vocabulário do núcleo | Specify | Pending |
| NET-07 | P1: Reconexão recebe o `FBattleState` atual e reconstrói a visão local | Specify | Pending |
| NET-08 | P1: Jogador desconectado continua sujeito a timeout de commit | Specify | Pending |
| NET-09 | P1: Modo Standalone resolve turno exatamente como hoje, sem servidor separado | Specify | Pending |
| NET-10 | P1: Replicação usa mecanismo nativo condicional a NetMode, sem duplicar lógica de resolução | Specify | Pending |
| NET-11 | P2: Abandono declara vitória ao jogador presente, via BatalhaEncerrada existente | Specify | Pending |

**Cobertura:** 11 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Dois processos do jogo (mínimo: Listen Server + 1 cliente local) completam uma partida inteira, do commit do turno 1 até `BatalhaEncerrada`, sem nenhum dos dois ver o commit do outro antes da resolução
- [ ] O mesmo `ABattleArena` roda offline (`NetMode::Standalone`, contra `FDumbOpponentAI`) sem nenhuma regressão nos 30 testes automatizados já existentes da Apresentação do Combate
- [ ] Um cliente desconectado à força no meio de um turno reconecta e vê o `FBattleState` correto, sem travar o outro jogador além do timeout configurado
- [ ] Nenhum commit malformado ou fora de ordem enviado por um cliente de teste consegue alterar o resultado de um turno — auditável com um teste que simula um cliente adversarial enviando payload inválido
