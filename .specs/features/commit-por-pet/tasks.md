# Commit por pet — tarefas

Todas dependem de as caixas de `GOAL.md` virarem `- [ ]`. Antes disso a feature
está **parada por decisão do usuário**, e as três decisões estão no fim deste
arquivo.

**Verificação em toda task**, sem exceção:

```bash
./Tools/sync_module_manifest.sh        # L-025: manifesto defasado faz teste novo sumir da contagem
./Tools/run_tests.sh                   # hoje 843 — o número só sobe
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
./Tools/audit_localizable_text.sh && ./Tools/audit_test_helper_names.sh
./Tools/audit_no_commit_replication.sh
```

---

## CP1 — MEDIR o teto de pets por lado ✅ FEITO
> 🤖 Modelo: `sonnet`

**Dependências:** nenhuma.

Nenhum número de pets por lado está escrito neste projeto, e **não se inventa
um**. `State.Pets` é um `TArray<FPetState>` sem teto declarado; o teto real está
espalhado por quem consome o estado. Esta task descobre **o que quebra
primeiro** montando estados com 2, 3 e 4 pets por lado e medindo:

| o que medir | por que este |
|---|---|
| a grade tem casa para todos? | `PackCell` guarda 4 bits por eixo, teto 15 por eixo — e a grade sai do `DefaultGame.ini`, podendo ser 3x2 |
| `ComputeHash` continua estável entre execuções? | ele ordena por `PetId`; a medição confirma em vez de presumir |
| o painel de batalha cabe? | altura FIXA, teto de 12 linhas — linha a mais sai por cima |
| `ABattleArena` desenha todos? | ator que nasce com componente visual e sem asset passa em todo teste e não existe na tela |

**Verificação:** `./Tools/run_tests.sh BattleSim.PetsPerSide`

**MEDIDO em 02/09/2026:**

| medida | valor | quem impõe |
|---|---|---|
| teto de eixo da grade | 15 | `PackCell`, 4 bits por eixo |
| teto de `PetId` | 255 | o campo é `uint8` |
| ações por turno | 3 | `FTurnCommit::ActionsPerTurn` |
| cabem por lado (3 linhas) | 3 | altura da coluna da ponta |
| **quebra primeiro em** | **2 por lado** | `PlaceDuelistsAtStartingCells` |

Nenhum teto ESTRUTURAL é o limite. O que quebra primeiro é a colocação inicial,
e ela quebra no SEGUNDO pet — com 2 por lado já há 2 pares dividindo casa; com
4, doze. O teto natural por lado é a ALTURA da grade.

**Aceite:** o teto **aparece como número** na saída do teste, com o nome de
quem o impõe. Sem isso a decisão do usuário sobre quantos pets por lado seria um
palpite com cara de escolha.

**Contrapeso obrigatório:** o teste NÃO afirma que o teto é aceitável nem
reprova por ele ser baixo — congelar uma decisão de produto dentro de um teste
de medição é como se cobra zero onde o certo é cobrar o parâmetro.

---

## CP2 — Aliados nascem em casas DIFERENTES
> 🤖 Modelo: `sonnet`

**Dependências:** CP1 (o teto decide quantas casas iniciais precisam existir).

`FBattleState::PlaceDuelistsAtStartingCells` manda **todo** pet do lado 0 para
`(0, linha do meio)` e todo do lado 1 para `(última coluna, linha do meio)`. Com
dois aliados eles nascem **um dentro do outro** — e aí a não-coabitação já
implementada (DP-02) os barra no primeiro slot, como se dois inimigos estivessem
se trombando.

A casa inicial sai do `DefaultGame.ini`, seção
`[/Script/BattleSquare.BattleArena]`, como o tamanho da grade já sai —
**nunca uma coordenada escrita à mão**.

**Verificação:** `./Tools/run_tests.sh BattleSim.StartingCells`

**Aceite:** dois aliados começam em casas distintas, e nenhum
`EncontroNoMesmoPonto` nem `MovimentoBloqueado` aparece no traço antes de
qualquer um deles se mover. O defeito PARECERIA: a batalha abre e os dois
aliados já estão se empurrando sem ninguém ter escolhido nada.

**Contrapeso obrigatório:** **1 pet por lado continua nascendo exatamente onde
nascia** — coluna 0 e última coluna, linha do meio arredondando para baixo. Um
duelo que muda de casa inicial invalida todo snapshot de determinismo de cenário
que nem tem aliado.

---

## CP3 — O commit do núcleo endereça `PetId`
> 🤖 Modelo: `opus` 🧠 — mexe no contrato que rede, replay e hash compartilham

**Dependências:** CP2.

`FTurnCommit` (`Source/BattleSim/Public/Battle/BattleTypes.h`) passa a carregar
as três ações **por pet**, com o dono identificado por `PetId`.
`FBattleResolver::ResolveTurn` deixa de receber `LeftCommit`/`RightCommit`.

**Por que `PetId` e não índice do array:** `BTL-17` já diz que a ordem de
`State.Pets` não é garantia de determinismo — `ComputeHash` ordena por `PetId`
justamente por isso. Endereçar por índice faria uma ação trocar de dono se
alguém reordenasse o array, e o sintoma seria "o pet errado atacou", sem
nenhuma pista de que a causa é ordenação.

`static_assert(sizeof(FBattleAction) == 2)` continua valendo — o custo de rede
passa a ser 6 bytes **por pet**, e é isso que CP7 tem de carregar pelo fio.

**Verificação:** `./Tools/run_tests.sh BattleSim` e
`./Tools/audit_determinism.sh`

**Aceite:** um cenário de 1 pet por lado produz **o mesmo hash e o mesmo traço,
evento por evento**, antes e depois da mudança. O defeito PARECERIA: nada na
tela muda, e a partida gravada de ontem deixa de reproduzir.

**Contrapeso obrigatório:** o teste de determinismo com 1 pet por lado é
executado **antes** da mudança e o hash é anotado; ele bate depois. O commit não
entra no hash (medido em `ComputeHash`), então uma divergência aqui não é
"esperada pela mudança de contrato" — é defeito.

---

## CP4 — As fases agem sobre o pet ENDEREÇADO
> 🤖 Modelo: `sonnet` — a forma foi decidida em CP3

**Dependências:** CP3.

Três coisas param de significar "o pet do lado":

1. `FBattleState::FindAlivePetOnSide(uint8)` — usada por `ApplyPostures` (duas
   vezes) e por `ResolveAttackForSide`;
2. o `break` no fim do laço de pets em `CollectIntent`, que é literalmente o que
   faz *só o primeiro pet vivo do lado* se mover;
3. as assinaturas `ApplyPostures` / `ApplyMovement` / `ApplyCombat(State,
   LeftAction, RightAction, …)`.

**O cano que já existe, e que esta task só alcança:** `ClaimsByCell`,
`PetsBarrados`, `EmitBlocked` e `EmitEncounter` em `BattlePhaseMovement.cpp`
estão escritos, corretos e testados no ramo de lados opostos. Não há detecção de
colisão a escrever aqui — há detecção de colisão a **deixar de ser inalcançável**.

**Verificação:** `./Tools/run_tests.sh BattleSim.Phase`

**Aceite:** dois aliados com direções DIFERENTES se movem os dois no mesmo slot.
O defeito PARECERIA: o segundo aliado fica plantado na casa dele para sempre, e
o painel não diz por quê — que é exatamente o comportamento de hoje.

**Contrapeso obrigatório:** aliado que se trombou **não se machuca**.
`EmitEncounter` já discrimina `Side != Side` de propósito: trombada entre
aliados é bloqueio silencioso, não ataque mútuo. Aplicar o golpe mútuo aos dois
lados faria aliados se matarem por andar junto.

---

## CP5 — O AVISO de BTL-05 vira PROVA
> 🤖 Modelo: `sonnet`

**Dependências:** CP4.

O teste `BattleSim.Phase.Movement.AllyCollisionIsUnreachableUnderV1Contract`
(`Source/BattleSim/Private/Tests/BattlePhaseMovementTest.cpp`) existe para **não
fingir cobertura**: ele afirma o comportamento real de v1 e emite `AddWarning`
dizendo que BTL-05 não tem cobertura de execução.

Ele **vira** o teste da regra nova: dois aliados convergindo de verdade, os dois
barrados, os dois eventos no traço. O `AddWarning` **sai**.

**Por que converter e não acrescentar um teste novo ao lado:** quem ler o
histórico precisa achar a regra nova onde a antiga morava — foi assim que a
inversão do DP-02 foi tratada em `SameCellEncounterTest.cpp`, e é o padrão da
casa. Aviso que sobrevive ao conserto faz a próxima leitura concluir que a
lacuna continua aberta.

**Verificação:**

```bash
./Tools/run_tests.sh BattleSim.Phase.Movement
grep -rn "AddWarning" Source/          # não pode achar BTL-05
```

**Aceite:** o grep não acha BTL-05, e o teste convertido REPROVA se o `break` de
`CollectIntent` voltar. Teste que passa sem exercitar o caminho que afirma
testar é o mesmo problema de antes, com roupagem de verde.

**Contrapeso obrigatório:** atualizar **B-003 em `.specs/project/STATE.md`** na
mesma task, incluindo o símbolo defasado: B-003 e o comentário do teste citam
`DestinationClaimsBySide`, e o código de hoje tem `ClaimsByCell`, agrupando por
CASA e não por lado+destino. Fechar o blocker deixando o registro apontando para
um símbolo que não existe é trocar uma lacuna por outra.

---

## CP6 — MEDIR se a ordem entre aliados decide algo
> 🤖 Modelo: `opus` 🧠 — é decisão de determinismo

**Dependências:** CP4.

`BattleResolver.cpp` carrega uma nota: as quatro fases foram desenhadas para
Left e Right resolverem simetricamente, e *"não existe, hoje, um ponto de
decisão em v1 (1 pet por lado) onde a ordem Left-antes-de-Right mude o
resultado"*. O desempate por velocidade ficou como infraestrutura de M3
*"onde múltiplos pets do MESMO lado podem competir por precedência"* — e é
agora.

Esta task **mede quantos pontos de decisão existem** com N pets por lado, e a
resposta é o número que sai do teste. Candidatos a olhar, todos lidos:
derrubada de obstáculo (que já ordena por `PetId`), acúmulo de dano em
`ApplyCombat`, e a ordem de coleta de intenções.

**O cano que já existe:** `OrdemDeResolucao.Sort` em `ApplyMovement` já
desempata por `PetId`. O desempate não se inventa — se estende.

**Verificação:** `./Tools/run_tests.sh BattleSim.Resolver.Ordering`

**Aceite:** o teste embaralha `State.Pets` e afirma **hash idêntico** em todas as
permutações. O defeito PARECERIA: a mesma partida com a mesma semente dá dois
resultados, e o culpado aparente é o RNG.

**Contrapeso obrigatório:** se a medição achar ZERO pontos de decisão, o
desempate por velocidade **não é implementado** — e a nota de
`BattleResolver.cpp` é reescrita dizendo que foi medido e não é necessário.
Implementar desempate sem ponto de decisão é o código morto que B-003 já
custou uma vez.

---

## CP7 — O fio carrega N pets
> 🤖 Modelo: `opus` 🧠 — rede, e a auditoria que a protege é regex

**Dependências:** CP3.

`FNetTurnCommit` (`Source/BattleSquare/Public/Net/BattleNetTypes.h`) tem três
campos NOMEADOS — `ActionA`, `ActionB`, `ActionC` — e o comentário diz por quê:
não se conseguiu provar que `FRepLayout` expande array C estático em vez de
serializar só o elemento 0, e *"o modo de falha alternativo é silencioso: o jogo
compilaria, conectaria e resolveria turnos, só que as ações 2 e 3 do oponente
não existiriam"*.

Crescer para N pets é reencontrar o mesmo modo de falhar, agora com o **pet 2**
em vez da ação 2. `ValidateNetTurnCommit`, `ToTurnCommit` e `ToNetTurnCommit`
acompanham; `UBattleTurnCoordinator` (`SubmitCommit(uint8 Side, …)`,
`PendingCommitSide0/1`, `MakeWaitOnlyCommit()`) e `ABattleArena`
(`LastCommitBySide[2]`) deixam de pensar em dois.

⚠️ **`Tools/audit_no_commit_replication.sh` procura literalmente
`\b(FNetTurnCommit|FTurnCommit)\b`.** Tipo novo fora dessa lista sai da
cobertura **sem nenhum vermelho**: a auditoria continua imprimindo "limpo", só
que sobre menos coisa. Nome novo entra na regex **na mesma task**.

**Verificação:**

```bash
./Tools/audit_no_commit_replication.sh
./Tools/run_tests.sh BattleSquare.Net
```

**Aceite:** um teste manda o commit de N pets pelo caminho de rede e afirma que
**o pet N chegou** — nome, ação e direção. O defeito PARECERIA: a batalha online
roda, ninguém vê erro, e o aliado do fundo simplesmente aguarda todo turno.

**Contrapeso obrigatório:** um teste que marca o tipo NOVO como
`UPROPERTY(Replicated)` e confirma que a auditoria **reprova**. Auditoria que
não reprova quando deveria é pior que auditoria ausente — a ausente ninguém
confunde com garantia.

---

## CP8 — Quem joga escolhe ação POR PET, e a tela diz de quem é
> 🤖 Modelo: `sonnet`

**Dependências:** CP3, CP7.

`UBattleActionQueueComponent::BuildCommit()` monta uma fila de 3 ações sem dono.
Ela passa a ter dono. **A tela não decide regra (DP-ui-01):** o botão encaminha
ao componente, que já tem a regra e o teste — escolher "para qual pet é esta
ação" é escolher **dono**, não regra.

Na tela, por `FBattleDebugScreen::Show` e pela barra de botões
(`FBattleDebugToolbar`), que é o caminho que comprovadamente funciona em PIE:

- o painel diz **de qual pet** é cada ação escolhida, com `Key` fixa por pet
  (estado que muda a cada turno se atualiza no lugar, não empilha);
- a grade desenhada já mostra quem está em cada casa — com aliados ela passa a
  distinguir **qual** aliado;
- o clique aparece no painel (`clique: Atacar — pet 2`). Se o clique aparece e a
  ação não muda nada, o defeito está depois do clique; se nem o clique aparece,
  o widget não está recebendo. Uma rodada, duas respostas.

Texto do jogador em `LOCTEXT`/`NSLOCTEXT` com argumentos **nomeados**
(`{Pet}`, nunca `{0}`), e `./Tools/gather_text.sh` depois.

**Verificação:**

```bash
./Tools/audit_localizable_text.sh
./Tools/gather_text.sh
./Tools/run_tests.sh BattleSquare.ActionQueue
```

Roteiro em `docs/verification/commit-por-pet.md`.

**Aceite:** dá para mandar o aliado A para a esquerda e o aliado B para a
direita **no mesmo turno**, e o painel diz qual foi qual. O defeito PARECERIA:
os dois aliados andam sempre juntos, como hoje — e sem a linha no painel ninguém
sabe se foi escolha ou limitação.

**Contrapeso obrigatório:** pet sem ação escolhida **aguarda**, e o painel diz
isso por escrito. Silêncio faria o jogador achar que escolheu para os dois; e
`MakeWaitOnlyCommit()` já existe justamente para esse caso.

---

## Decisões que são do usuário

Nenhuma destas se resolve por medição — as três mudam o produto, não o código.

1. **Quantos pets por lado.** CP1 mede o TETO que o sistema aguenta; o número
   dentro dele é escolha de jogo. `spec.md` de `combate-nucleo` diz *Out of
   Scope: "mais de 1 pet por lado"*, e sair disso é decisão, não consequência.

2. **As 3 ações são POR PET ou 3 no total, distribuídas entre os pets?** Muda o
   custo de rede (6 bytes por pet contra 6 bytes por turno, e o BTL-19 nomeia
   esse número) e muda o jogo: por pet, N pets agem N vezes mais; no total,
   escolher quem age é a decisão tática.

3. **O contrato antigo (commit por LADO) continua aceito durante a transição?**
   Aceitar os dois permite subir por partes, e cobra o preço de "duas maneiras
   de mandar um turno" — e duas maneiras concordam até a primeira edição, que é
   a causa registrada de L-032 e L-033. Recusar o antigo fecha a porta e exige
   CP3 a CP8 num salto.

---

## O que estas tarefas NÃO fazem

- **Não reabrem DP-02.** A não-coabitação está implementada e provada; a
  medição, com arquivo e função, está em `spec.md`. Nada aqui a re-decide.
- **Não decidem quantos pets por lado.** CP1 mede o teto e **para**.
- **Não implementam desempate por velocidade sem ponto de decisão.** CP6 mede
  primeiro; zero pontos significa nota reescrita, não código escrito.
- **Não autoram asset.** Pet a mais aparece pela malha e cor que `APetView` já
  atribui no construtor — e é o teste da ATRIBUIÇÃO que garante isso, não o olho.
- **Não mexem no traçado nem no gabarito.** Nada aqui encosta na carta.
