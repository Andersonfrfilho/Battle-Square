# Commit por pet — a ação tem dono

## O que está errado hoje, em uma frase

O turno pergunta **"o que o LADO ESQUERDO faz?"**. Deveria perguntar **"o que
ESTE pet faz?"** — e a diferença não é de estilo: é o que torna o requisito
**BTL-05** (colisão entre aliados) inalcançável por qualquer teste.

---

## De onde isto vem: B-003, órfão desde 25/08/2026

`ApplyMovement` recebe **uma ação por LADO**, não por PET — reflexo direto de
`FTurnCommit` ser por lado, que é o contrato de rede de v1 (`combate-nucleo`,
`design.md`: *"Custo em rede: 6 bytes por turno, por jogador. É o payload
inteiro do BTL-19"*).

Com uma direção única por lado, dois aliados partindo de casas **diferentes**
nunca convergem para a mesma casa. Não é teste esquecido — é **matematicamente
inatingível** pela interface pública.

E o código de detecção de colisão **existe e está correto**. Ele fica
inalcançável.

### O que quase aconteceu, e por que isso importa mais que o defeito

Quase se escreveu um teste que passava **sem exercitar o caminho** e se chamou
isso de cobertura de BTL-05. Foi corrigido antes de rodar: hoje o teste
`BattleSim.Phase.Movement.AllyCollisionIsUnreachableUnderV1Contract`
(`Source/BattleSim/Private/Tests/BattlePhaseMovementTest.cpp`) documenta a
lacuna com `AddWarning` em vez de fingir prova.

**Esta feature tem de TROCAR esse aviso por prova.** Deixar os dois é pior que
não ter feito nada: um aviso que sobrevive ao conserto vira ruído, e na próxima
leitura alguém conclui que a lacuna continua aberta.

### E a resolução foi adiada para um marco que fechou sem ela

B-003 diz: *"adiada para M3, junto da expansão do contrato de rede para commit
por pet"*. **M3 fechou sem ela.** É por isso que isto está órfão hoje, e é por
isso que ele destrava N pets por lado — nenhuma outra coisa destrava.

---

## MEDIÇÃO — DP-02 (não-coabitação) JÁ ESTÁ IMPLEMENTADA

A decisão do usuário de 27/08/2026 (`oponente-com-criterio/spec.md`, *"Próxima
decisão, já tomada pelo usuário"*) foi medida **no resolvedor**, não deduzida de
comentário. Resultado: **está feita, e está provada.**

| onde | o que faz |
|---|---|
| `ApplyMovement`, mapa `ClaimsByCell` — `Source/BattleSim/Private/Battle/BattlePhaseMovement.cpp` | agrupa a casa FINAL de **todo pet vivo** (o que se move e o que está parado). Casa com 2+ pretendentes: cada pretendente que tinha intenção entra em `PetsBarrados` e recebe `EmitBlocked` — **ninguém entra** |
| `EmitEncounter`, mesmo arquivo | par de lados OPOSTOS na mesma casa emite `EBattleEventType::EncontroNoMesmoPonto` |
| `ApplyCombat` — `Source/BattleSim/Private/Battle/BattlePhaseCombat.cpp` | relê `EncontroNoMesmoPonto` do traço do slot e chama `ApplyHitAgainst` **nos dois sentidos**, com `MovePower=0`. É o **ataque mútuo** que a decisão pedia |
| `ResolveTarget` — mesmo arquivo | o comentário registra a inversão: *"A busca por oponente COABITANDO a própria casa saiu daqui com a inversão do DP-02: F3 impede que dois pets terminem no mesmo ponto"*. `FindLivingOpponentAtCell` continua existindo, mas já não é chamada para a própria casa |

Provado por `Source/BattleSim/Private/Tests/SameCellEncounterTest.cpp`
(`BattleSim.Encounter.SameCell.BlocksBoth`, `.HurtsBoth`,
`.WalkingIntoStandingOpponent`) e por
`BattleSim.Phase.Movement.OpposingSidesCannotCoexist`, que é **o antigo teste de
coabitação convertido** em vez de apagado.

**O comentário de `CLAUDE.md` que diz que a grade "mostra a coabitação sem
explicação" está DEFASADO.** Ele foi o motivo de medir, e não podia ser a
conclusão: era exatamente o caso de "formar impressão olhando o desenho".

### O que a medição corrige em B-003

B-003 diz que a detecção de colisão está *"morta/inalcançável"*. Isso hoje é
**meia verdade**, e a metade que sobrou é a que importa:

| ramo | alcançável hoje? | por quê |
|---|---|---|
| lados OPOSTOS na mesma casa | ✅ sim, e provado | um pet por lado basta: Left vai para a direita, Right para a esquerda, os dois miram o meio |
| **ALIADOS** na mesma casa | ❌ não | `CollectIntent` tem um `break` no fim do laço de pets: **só o primeiro pet vivo do lado recebe a ação**. Dois aliados no `State.Pets` e um só se move |

E B-003 aponta um símbolo que **não existe mais**: ele e o comentário do teste
citam `DestinationClaimsBySide`; o código de hoje tem `ClaimsByCell`, e ele
agrupa por **casa**, não por lado+destino. Foi essa troca que tornou o ramo
cross-side alcançável e deixou o de aliados atrás do `break`.

---

## O inventário da suposição "uma ação por lado"

Tudo isto foi lido, não presumido. É a superfície que a feature encosta:

| camada | onde | a suposição |
|---|---|---|
| núcleo — tipo | `FTurnCommit`, `Source/BattleSim/Public/Battle/BattleTypes.h` | `FBattleAction Actions[3]`, com `static_assert(sizeof(FBattleAction) == 2)` amarrado ao custo de rede |
| núcleo — entrada | `FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit)` | dois commits, um por lado |
| núcleo — fases | `ApplyPostures` / `ApplyMovement` / `ApplyCombat(State, LeftAction, RightAction, …)` | duas ações por slot |
| núcleo — busca | `FBattleState::FindAlivePetOnSide(uint8)` | *"o"* pet do lado; usada por `ApplyPostures` (duas vezes) e por `ResolveAttackForSide` |
| núcleo — movimento | o `break` no fim do laço de `CollectIntent` | só o primeiro pet vivo do lado se move |
| núcleo — casa inicial | `FBattleState::PlaceDuelistsAtStartingCells` | **todo** pet do lado 0 vai para `(0, linha do meio)` — dois aliados nasceriam empilhados |
| fio | `FNetTurnCommit`, `Source/BattleSquare/Public/Net/BattleNetTypes.h` | três campos NOMEADOS (`ActionA/B/C`) porque não se conseguiu provar que `FRepLayout` expande array C estático |
| tela | `UBattleActionQueueComponent::BuildCommit()` | uma fila de 3 ações, sem dono |
| coordenação | `UBattleTurnCoordinator`: `SubmitCommit(uint8 Side, …)`, `PendingCommitSide0/1`, `MakeWaitOnlyCommit()` | um commit pendente por lado |
| arena | `ABattleArena`: `ResolveTurnWithCommits`, `StoredLocalCommit`, `LastCommitBySide[2]` | dois commits |
| IA | `FDumbOpponentAI::GenerateRandomValidCommit(State, Side, …)`, `FTacticalOpponentAI::GenerateCommit(State, Side, …)` | decide por lado |

---

## O cano que JÁ EXISTE, e que a feature só precisa alcançar

Não há detecção de colisão a escrever. Há detecção de colisão a **tornar
alcançável**:

- `ClaimsByCell` já agrupa por casa e já barra todo pretendente com intenção;
- `PetsBarrados` já impede o movimento de quem foi barrado;
- `EmitEncounter` já discrimina lados opostos de aliados (`Side != Side`), então
  o ramo de aliados **existe e é o silêncio deliberado**: aliado que se trombou
  fica parado e não se machuca;
- `OrdemDeResolucao.Sort` já desempata por `PetId` na derrubada de obstáculo —
  é o desempate que CP6 vai medir, não inventar;
- `FBattleState::ComputeHash` já ordena `Pets` por `PetId` antes de somar, então
  N pets já hasheiam de forma estável.

---

## O que NÃO muda o hash, e o que muda

Medido em `FBattleState::ComputeHash`
(`Source/BattleSim/Private/Battle/BattleState.cpp`): **nenhum campo de commit
entra no hash.** Trocar a forma do commit não invalida, por si só, nenhum
snapshot de determinismo.

O que invalida é **pet a mais no estado** — e aí o hash muda porque o estado
mudou de verdade, que é o hash funcionando. Snapshot de cenário com 1 pet por
lado tem de continuar batendo bit a bit; é o contrapeso de CP3.

---

## Os cuidados que esta feature carrega

**Rede, replay e hash de uma vez.** O commit é tipo de fio dedicado
(`FNetTurnCommit`) exatamente porque array C estático não se provou replicável —
e o modo de falhar era **silencioso**: o jogo compilava, conectava e resolvia
turnos, só que as ações 2 e 3 do oponente não existiam. Crescer para N pets é
reencontrar esse mesmo modo de falhar, agora com o pet 2 em vez da ação 2.

**A auditoria do commit é por REGEX, e regex não conhece tipo novo.**
`Tools/audit_no_commit_replication.sh` procura literalmente
`\b(FNetTurnCommit|FTurnCommit)\b` ao lado de um `UPROPERTY(… Replicated)`. Um
tipo novo que não entre nessa lista sai da cobertura **sem nenhum vermelho** — a
auditoria continua dizendo "limpo", só que sobre menos coisa.

**A tela não decide regra (DP-ui-01).** Todo botão encaminha ao
`UBattleActionQueueComponent`, que já tem a regra e o teste. Escolher "para qual
pet é esta ação" é escolha de **dono da ação**, não de regra — a regra continua
onde está.

**`BattleSim` é determinístico.** Sem float, sem `FMath::Rand`, sem relógio.
Desempate entre aliados sai de `PetId`, nunca de ordem de contêiner.

---

## Aceite

Dois aliados partindo de casas diferentes, cada um com a **sua** direção,
convergem para a mesma casa: **os dois ficam onde estavam**, e o traço registra
o bloqueio dos dois. O MESMO cenário com o commit por lado é impossível de
montar — não dá direção diferente a dois aliados.

E o `AddWarning` de BTL-05 **não existe mais** em `Source/`.
