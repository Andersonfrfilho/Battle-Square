# Roteiro de Verificação de Rede Real — Combate Online

**Feature:** `.specs/features/combate-online/`
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado com rede de verdade.

Este documento cobre o que o `design.md` marca como ❌ na tabela "Limite de Ferramenta" — itens que exigem dois processos e/ou uma conexão de rede real, e que **não puderam ser testados automaticamente** nesta sessão. A causa não é falta de esforço: T11 (`tasks.md`) investigou formalmente e encontrou um bloqueio de infraestrutura, não de código — ver `STATE.md`, AD-020/B-004.

## Por que este roteiro existe (contexto do bloqueio)

`Build.sh BattleSquareServer` falha com `Server targets are not currently supported from this engine distribution`. A engine instalada é a build binária do Epic Games Launcher; `TargetType.Server` só compila em engines compiladas a partir do código-fonte da Epic (GitHub, requer conta vinculada). Até essa troca de engine acontecer, os itens abaixo só podem ser verificados manualmente, e só parcialmente (via `ListenServer`, já que `DedicatedServer` não compila de jeito nenhum aqui).

---

## Item 1 — `Actions[3]` atravessa o fio inteiro entre processos

- [ ] **Não verificado**

**Risco que isto cobre:** `FNetTurnCommit` foi desenhado com 3 campos nomeados exatamente para eliminar a dúvida sobre se um array C estático seria truncado na serialização de rede (ver `design.md`, "A decisão técnica que a investigação mudou"). A verificação abaixo é a prova final de que essa decisão realmente resolveu o problema em trânsito real, não só na teoria.

**Passo concreto:**
1. Compilar o target `BattleSquare` (Game) normalmente — não precisa do Server, `ListenServer` roda como `TargetType.Game`.
2. Rodar duas instâncias do editor/jogo na mesma máquina: uma como host (`-log -ListenServer` ou via PIE com "Number of Players: 2" e "Net Mode: Play as Listen Server"), outra como cliente conectando em `127.0.0.1`.
3. No cliente, montar um commit com 3 ações claramente distintas entre si (ex.: `Mover Cima`, `Atacar Direita`, `Defender`) via `UBattleActionQueueComponent` (chamada direta em Blueprint/console, já que a UI UMG ainda não existe — T14 da Apresentação do Combate).
4. No log do host (servidor), procurar a linha de `FBattleResolveResult` ou instrumentar um `UE_LOG` temporário em `UBattleTurnCoordinator::ResolveWithCommits` imprimindo as 3 ações de cada `FTurnCommit` recebido.
5. Confirmar que as 3 ações do cliente aparecem **intactas e na ordem certa** no log do servidor — não só a primeira.

---

## Item 2 — cliente e servidor separados resolvendo um turno

- [ ] **Não verificado**

**Passo concreto:**
1. Mesma configuração de 2 instâncias do Item 1.
2. Host monta um commit próprio (ele também é jogador, em `ListenServer`); cliente monta o dele.
3. Confirmar que `UBattleTracePlayer::PlayTrace` roda nos DOIS processos (host e cliente) com o mesmo trace — comparar contagem de eventos e primeiro/último evento nos dois logs.
4. Confirmar que nenhum dos dois processos calculou o resultado localmente antes de receber a resposta do servidor — só o processo com `HasAuthority() == true` deve logar a chamada a `FBattleResolver::ResolveTurn`.

---

## Item 3 — reconexão após queda de rede real

- [ ] **Não verificado**

**Passo concreto:**
1. Partida em andamento (Item 2 já rodando), pelo menos 2 turnos resolvidos.
2. No cliente, forçar desconexão real (fechar o processo do cliente, ou usar `Open 127.0.0.1` novamente depois de matar a conexão — não uma chamada de função simulada).
3. Reconectar o cliente à mesma partida (mesma porta/host).
4. Confirmar que o cliente reconstrói a cena a partir do `FBattleState` atual (pets nas posições e vidas corretas), **sem** reproduzir a animação dos turnos perdidos — nenhuma barra de vida "voltando no tempo" ou pet "revivendo" durante a reconstrução.
5. Confirmar que o servidor não travou esperando o cliente durante a desconexão além do `CommitTimeoutSeconds` normal (45s) — se o turno resolveu por timeout enquanto o cliente estava fora, isso é esperado e correto.

---

## O que NÃO precisa deste roteiro

Estes itens já têm cobertura automatizada headless e não exigem rede real — não repetir aqui:

- Validação/conversão do tipo de fio (`BattleSquare.Net.ValidateNetTurnCommit.*`, `BattleSquare.Net.RoundTripConversion`)
- Máquina de estado do coordenador — acumular, resolver, timeout, corrida (`BattleSquare.Net.TurnCoordinator*`)
- Reconexão e abandono, lógica isolada (`BattleSquare.Net.ReconnectionAndAbandon.*`) — o que falta é só a queda de conexão **real**, não a lógica de resposta a ela
- Sonda anti-replicação do commit (`Tools/audit_no_commit_replication.sh`)
- Regressão do modo Standalone (44 testes `BattleSquare` + 44 `BattleSim`)

---

## Pré-requisito para desbloquear o roteiro completo (Dedicated Server)

Os 3 itens acima são verificáveis com `ListenServer` (host = jogador). Para verificar com `DedicatedServer` de verdade (o modo de produção pretendido, ver `design.md` DP-online-01), é necessário primeiro resolver B-004: trocar a instalação da engine por uma build compilada a partir do código-fonte da Epic. Isso é uma decisão de infraestrutura, registrada em `STATE.md`, fora do escopo desta feature.

## Registro de execução

| Data | Quem | Itens verificados | Modo (ListenServer/Dedicated) | Resultado |
|---|---|---|---|---|
| — | — | nenhum ainda | — | — |
