# Combate Online — Design

**Spec:** `.specs/features/combate-online/spec.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare` (camada de jogo). **`BattleSim` não é tocado** — nenhuma linha. O resolvedor que roda online é literalmente o mesmo `FBattleResolver::ResolveTurn` de hoje.

---

## Princípio que governa todo o resto

AD-005 já decidiu isto em 2026-08-24, antes de existir uma linha de rede:

> o cliente envia apenas o commit das três ações. O servidor simula e devolve um **trace de eventos** ordenado. O cliente anima o trace; ele não decide nada.

Esta feature não é uma decisão de arquitetura nova — é a **execução** de AD-005. Três consequências que valem como regra, não como preferência:

1. **Nenhuma predição de cliente.** É por isso que GAS foi rejeitado (AD-007): ele traz predição e replicação contínua embutidas, que é o oposto do que um jogo por turnos com commit às cegas quer. Não se reintroduz predição por outra porta.
2. **O trace é o único canal de "o que aconteceu".** Nenhum RPC do tipo `Client_PlayAttackAnimation`. Se a animação precisa de um número, esse número está num `FBattleEvent` (BTL-22, já auditado por `Tools/audit_no_recalculation.sh`).
3. **O RNG mora no estado, e o estado mora no servidor.** `FBattleState::Random` já viaja dentro do estado por AD-004. O cliente nunca gera número aleatório de combate — não porque seria errado, mas porque ele não tem o que fazer com ele.

---

## Decision Points resolvidos

### DP-online-01: topologia — **autoridade no código, topologia no deploy**

**Decisão:** o código não escolhe topologia. Toda lógica de resolução fica atrás de `HasAuthority()`, e as três `NetMode` da Unreal (`Standalone`, `ListenServer`, `DedicatedServer`) rodam o **mesmo caminho de código**. O alvo de confiança para jogo competitivo é **Dedicated Server**; `ListenServer` é suportado para desenvolvimento e LAN, com a ressalva de segurança registrada abaixo.

**Por que isto não é fugir da pergunta:** na Unreal, "quem é o servidor" é uma propriedade de execução, não de arquitetura. O mesmo `ABattleArena` com `bReplicates = true` e resolução guardada por `HasAuthority()` atende os três modos sem um único `if (bIsOnline)`. Escolher topologia no código seria criar dois caminhos de resolução — exatamente o que NET-10 proíbe, e a maneira mais confiável de fazer online e offline divergirem em silêncio.

**Reforço concreto:** `Source/BattleSquareServer.Target.cs` já existe no projeto desde o início, com o comentário *"Existe desde já para que nada no jogo compile só porque há cliente presente."* A intenção estava registrada antes desta feature; aqui ela só passa a ser exercida.

**Ressalva de segurança, registrada honestamente:** em `ListenServer`, o jogador que hospeda tem acesso de memória ao processo autoritativo. Ele não consegue forjar um resultado *pelo protocolo* (o resolvedor roda na máquina dele, então não há nada a forjar), mas consegue ler o commit do oponente antes da revelação — o que **quebra o commit às cegas**, que é o coração do jogo (AD-005/BTL-02). Portanto: **`ListenServer` nunca é modo competitivo.** Isto não é um detalhe de operação, é uma propriedade do jogo, e por isso está aqui e não num runbook.

**Trade-off aceito:** Dedicated Server custa infraestrutura que ainda não existe. Como o código é o mesmo, esse custo é adiável sem dívida técnica — o dia em que houver infra, não há migração a fazer.

---

### DP-online-02: transporte do commit — **RPC nativo da Unreal**

**Decisão:** `UFUNCTION(Server, Reliable, WithValidation)` sobre o `NetDriver` padrão. Nenhum serviço HTTP/WebSocket separado para o commit.

**Razão:**
- O payload é minúsculo — `FTurnCommit` são 3 pares (tipo, direção), 6 bytes de dado útil. Montar um canal HTTP autenticado para 6 bytes é desproporcional.
- O `NetDriver` já entrega confiabilidade, ordenação e associação a uma `UNetConnection` — que é a identidade de sessão que esta feature precisa. Um serviço separado precisaria de autenticação própria, e **contas de jogador não existem** (M7). Inventar identidade só para o commit seria construir metade do M7 sem a spec dele.
- `apps/api-battle-pets` administra **pets**, não partidas. Enfiar o combate ali violaria a separação que AD-014 estabeleceu de propósito.

**O que fica aberto sem custo:** matchmaking futuro pode perfeitamente ser um serviço HTTP separado — ele acontece **antes** da partida, então não interfere no protocolo em-partida. Esta decisão não fecha aquela porta.

---

### DP-online-03: reconexão — **`FBattleState` completo, sem replay do trace**

**Decisão:** ao reconectar, o servidor envia o `FBattleState` atual. O cliente reconstrói a visão (`SpawnPetViews` + `SetInitialState`, já existentes) a partir dele. O trace acumulado **não** é reenviado.

**Razão:** o trace é um artefato de **apresentação**, não de estado. `FBattleState` já é a representação completa e autossuficiente da partida — foi desenhado assim (é serializável e tem `ComputeHash()` para detecção de dessincronia, ver `BattleState.h`). Reenviar 20 turnos de trace para o jogador assistir animação que ele perdeu é dado e complexidade a mais para entregar algo que ninguém quer: ninguém que caiu da rede quer esperar a animação alcançar o presente.

**Consequência assumida:** o jogador que reconecta **perde a animação** dos turnos que aconteceram enquanto esteve fora, e reaparece direto no estado presente. Isso é aceitável e coerente — é o mesmo comportamento de `SkipToEnd` (T7), que já existe e já foi decidido como um caminho legítimo (PRES-10).

**Não fecha a porta de espectador/replay:** o backlog já registra *"Espectador e replay a partir do trace de eventos — sai quase de graça do AD-005"*. Guardar o histórico de traces no servidor para replay é uma feature futura, independente desta, e não precisa de nada aqui.

---

### DP-online-04: temporização — **constantes nomeadas, não números soltos**

Parâmetros de balanceamento, no `*.constant.ts`-equivalente de C++ (`BattleNetConstants.h`), mesmo padrão de `MaxTurns` no núcleo:

| Constante | Valor proposto | Fundamento |
|---|---|---|
| `CommitTimeoutSeconds` | 45 | O critério de sucesso da Apresentação do Combate mede "jogador novo monta um turno em menos de 20s". 45s dá folga de mais de 2x para quem está pensando, sem prender ninguém. |
| `AbandonTimeoutSeconds` | 120 | Tempo para reconectar de uma queda de rede real (trocar de Wi-Fi para 4G, reiniciar roteador). Abaixo disso, declara-se abandono cedo demais. |

Valores são chute fundamentado, não medição — só jogo real ajusta. Por isso são constantes nomeadas num arquivo só.

---

## A decisão técnica que a investigação mudou: o tipo de fio do commit

**`FTurnCommit` não vai no RPC. Vai um tipo de fio explícito, `FNetTurnCommit`.**

Isto não é preferência de estilo — vem de uma investigação concreta feita antes de escrever este design:

**O que foi verificado:**
1. `UFUNCTION(Server, Reliable) void Server_SubmitCommit(FTurnCommit)` **compila** — plantei o RPC no módulo e a build passou. UHT aceita e gera `P_GET_STRUCT`.
2. Mas `FTurnCommit` contém um **array C estático**, `FBattleAction Actions[3]` — o mesmo campo que já forçou `BuildCommit()` a ficar fora do Blueprint (UHT rejeita array estático exposto a Blueprint; verificado em `UhtFunction.cs:1047`).
3. `FStructProperty::NetSerializeItem` em UE 5.8, para struct **sem** `NetSerialize` customizado, cai em `UE_LOGF(LogProperty, Fatal, "Deprecated code path")` (`PropertyStruct.cpp:194`).
4. Parâmetros de RPC não usam esse caminho — usam `FRepLayout::InitFromFunction`, que expande `ArrayDim` **do parâmetro** (`RepLayout.cpp:6506`), que no nosso caso é 1.

**O que NÃO foi verificado, e é justamente o que importa:** se a recursão do `FRepLayout` dentro do struct expande o `Actions[3]` interno em 3 comandos, ou se serializa só o elemento 0. Provar isso exige um teste de runtime com dois processos.

**Por que isso decide o design em vez de virar "verificar depois":** o modo de falha é **silencioso e caro**. Se só `Actions[0]` atravessar, o jogo compila, conecta, resolve turnos — e as ações 2 e 3 do oponente simplesmente não existem. Ninguém vê um erro. É exatamente a forma de morte que L-001 registra sobre o protótipo Cocos, e a razão de AD-004 existir.

**A solução custa quase nada e ganha três coisas:**

```cpp
// Tipo de fio: campos nomeados, zero array estático, zero dúvida.
USTRUCT()
struct FNetTurnCommit
{
    GENERATED_BODY()
    UPROPERTY() FBattleAction ActionA;
    UPROPERTY() FBattleAction ActionB;
    UPROPERTY() FBattleAction ActionC;
};
```

1. **Remove a incerteza inteira** — três `FStructProperty` simples, cada um um campo nomeado. Nada de `ArrayDim` em lugar nenhum do caminho de rede.
2. **É onde a validação mora** (NET: "cliente adversarial"). O commit chega de fora e é entrada não confiável; ele precisa de um ponto de validação de qualquer forma. O tipo de fio é o lugar natural — a conversão `FNetTurnCommit → FTurnCommit` **é** a fronteira de validação.
3. **Torna o contrato de rede explícito e auditável.** AD-005 diz que o commit é *o* contrato de rede; ter um tipo que existe só para isso é honesto sobre essa importância. `FTurnCommit` continua sendo o tipo do núcleo, livre para mudar sem quebrar o fio.

**Custo:** ~20 linhas e uma conversão em dois pontos. Barato o suficiente para não precisar de justificativa maior que "o modo de falha alternativo é invisível".

**Nota de futuro:** B-003 já registra que `FTurnCommit` é **por lado, não por pet**, e que expandir para commit por pet está adiado para M3. Quando isso acontecer, `FNetTurnCommit` é exatamente o lugar onde a versão do contrato de rede vai ser negociada — mais um argumento para ele existir separado.

---

## Arquitetura

### Onde cada peça roda

```
CLIENTE (NetMode: Client)                SERVIDOR (Authority)
─────────────────────────                ────────────────────
UBattleActionQueueComponent
  (já existe, T1–T4)
       │ Commit()
       ▼
UBattleNetCommitComponent  ──RPC──►  Server_SubmitCommit(FNetTurnCommit)
  (novo)                                    │
                                            ▼
                                     Validação (fronteira)
                                            │ FTurnCommit
                                            ▼
                                     UBattleTurnCoordinator (novo)
                                       - guarda commit por lado
                                       - NUNCA replica commit
                                       - aguarda os dois / timeout
                                            │ os dois presentes
                                            ▼
                                     FBattleResolver::ResolveTurn
                                       (INALTERADO, BattleSim)
                                            │ {NextState, Trace}
                                            ▼
UBattleTracePlayer  ◄──Multicast──   Client_PlayTurn(State, Trace)
  (já existe, T7)
       │ OnEventApplied
       ▼
APetView (já existe, T8)
```

### O erro que esta arquitetura existe para não cometer

**O commit jamais vira `UPROPERTY(Replicated)`.** Escrever

```cpp
UPROPERTY(Replicated) FNetTurnCommit PendingCommit;  // ERRADO
```

replicaria o commit para **todos** os clientes assim que chegasse — entregando a fila do jogador A ao jogador B antes da resolução, e destruindo BTL-02 (commit às cegas) sem produzir erro nenhum. O jogo continuaria "funcionando".

Por isso o commit vive **só em memória do servidor**, dentro de `UBattleTurnCoordinator`, num campo **sem** `UPROPERTY(Replicated)`. O que replica é exclusivamente o **resultado** (estado + trace), e só depois dos dois commits.

Isto é verificável por sonda, no mesmo espírito de `probe_isolation.sh` e `audit_no_recalculation.sh` — ver Verificação abaixo.

### Componentes novos

| Componente | Onde | Responsabilidade | Roda em |
|---|---|---|---|
| `FNetTurnCommit` | `Public/Net/BattleNetTypes.h` | Tipo de fio do commit + validação | ambos |
| `UBattleNetCommitComponent` | `Public/Net/BattleNetCommitComponent.h` | Envia o commit local via RPC; recebe resultado | cliente |
| `UBattleTurnCoordinator` | `Public/Net/BattleTurnCoordinator.h` | Acumula os dois commits, aplica timeout, dispara resolução | **servidor** |
| `BattleNetConstants.h` | `Public/Net/` | `CommitTimeoutSeconds`, `AbandonTimeoutSeconds` | ambos |

### O que muda em código existente

`ABattleArena::HandlePlayerCommitted` é o único ponto tocado. Hoje:

```cpp
const FTurnCommit OpponentCommit = FDumbOpponentAI::GenerateRandomValidCommit(...);
const FBattleResolveResult Result = FBattleResolver::ResolveTurn(CurrentState, PlayerCommit, OpponentCommit);
```

Passa a delegar para o coordenador, que decide a origem do segundo commit **por presença de oponente real, não por flag de modo**:

- oponente humano conectado → aguarda o commit dele (ou timeout)
- nenhum oponente humano → `FDumbOpponentAI`, exatamente como hoje (NET-09)

A resolução em si (`ResolveTurn`) continua sendo uma chamada só, num lugar só. Não há dois caminhos.

---

## Limite de Ferramenta — o que eu consigo verificar e o que não

Mesma honestidade da Apresentação do Combate, que separou lógica testável de layout que exige editor. Aqui a divisão é **lógica pura vs. comportamento de rede real**:

| Item | Verificável headless por mim | Como |
|---|---|---|
| Validação de `FNetTurnCommit` (enum fora de range, etc.) | ✅ sim | função pura, teste de Automation |
| Conversão `FNetTurnCommit ↔ FTurnCommit` (ida e volta preserva as 3 ações) | ✅ sim | teste de Automation |
| Máquina de estado do coordenador (aguarda dois, timeout preenche com Aguardar, rejeita commit duplo) | ✅ sim | tempo injetado, sem `UWorld` real |
| Sonda "commit nunca é replicado" | ✅ sim | grep + reflexão, ver abaixo |
| Modo Standalone sem regressão | ✅ sim | os 30 testes atuais continuam rodando |
| **`Actions[3]` realmente atravessa o fio inteiro** | ❌ **não** | exige 2 processos de verdade |
| **Cliente e servidor separados resolvendo um turno** | ❌ **não** | exige 2 processos de verdade |
| **Reconexão real após queda de rede** | ❌ **não** | exige derrubar conexão de verdade |

**O que isso significa na prática:** o `FNetTurnCommit` de campos nomeados existe justamente para que a linha ❌ mais perigosa (serialização silenciosamente truncada) deixe de ser um risco em vez de virar um teste que eu não consigo rodar. As outras duas ❌ precisam de um teste de rede real — proponho tratá-las como uma tarefa de verificação explícita, com roteiro, no mesmo formato de `docs/verification/apresentacao-combate-visual.md`: honestamente marcado como **não verificado** até rodar.

Alternativa a avaliar na fase de Tasks: a Unreal tem *Functional Tests* com `PIE` multiplayer (`-PIEMultiplayer`, 2 instâncias). Não é headless limpo como Automation, mas é automatizável e roda os dois processos de verdade. Se funcionar no ambiente, sobe uma linha ❌ para ✅ — é a primeira coisa que eu investigaria ao começar a implementação.

---

## Verificação

Além dos testes de Automation por componente:

**Sonda nova — `Tools/audit_no_commit_replication.sh`:** falha se `FNetTurnCommit` (ou `FTurnCommit`) aparecer marcado com `UPROPERTY(Replicated` ou dentro de `GetLifetimeReplicatedProps` em qualquer arquivo de `BattleSquare`. Testada pelo ciclo já padronizado no projeto: plantar violação → esperar exit 1 → remover → esperar exit 0.

**Sondas existentes continuam valendo:** `audit_determinism.sh` (o núcleo segue sem float/rand), `audit_no_recalculation.sh` (a apresentação segue sem recalcular), `probe_isolation.sh` (`BattleSim` segue isolado). Nenhuma delas deve precisar de mudança — se alguma precisar, é sinal de que esta feature vazou para onde não devia.

---

## Riscos

| Risco | Gravidade | Mitigação |
|---|---|---|
| Serialização de struct pela rede truncar em silêncio | **alta** — falha invisível | `FNetTurnCommit` com campos nomeados elimina o caso; teste de ida-e-volta cobre a conversão |
| Commit vazar por replicação acidental | **alta** — destrói o jogo sem erro | commit fora de `UPROPERTY(Replicated)` + sonda dedicada |
| Online e offline divergirem por caminhos de código separados | média | um único `ResolveTurn`, guardado por `HasAuthority()`; NET-09 testado pelos 30 testes atuais |
| Falta de infra de dedicated server bloquear a entrega | baixa | `ListenServer` cobre desenvolvimento; código idêntico, migração zero |
| `ListenServer` ser usado como competitivo por engano | média | registrado como propriedade do jogo (DP-online-01), não como nota de operação |

---

## Modelo recomendado por fase (a definir em tasks.md)

| Etapa | Modelo |
|---|---|
| Tipo de fio, validação, conversão | `sonnet` |
| Máquina de estado do coordenador (timeout, commit duplo, corrida) | `sonnet` — **🧠 a lógica de corrida dos dois commits é fácil de errar sutilmente** |
| Fiação de replicação em `ABattleArena` | `sonnet` |
| Sondas e roteiro de verificação | `sonnet` / `haiku` |
