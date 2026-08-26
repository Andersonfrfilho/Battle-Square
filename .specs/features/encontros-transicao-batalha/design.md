# Encontros e Transição para Batalha — Design

**Spec:** `.specs/features/encontros-transicao-batalha/spec.md`
**Status:** Draft — aguarda aprovação

---

## Princípio que governa o design inteiro

A feature anterior (Streaming de Mundo) foi a primeira sem núcleo determinístico a proteger. **Esta volta a ter.** Ela toca o caminho que monta uma partida, e montagem de partida é exatamente onde L-021 e L-022 já mostraram que o projeto escorrega: código legítimo de montagem (`TranslateMatchup`, `ApplyLevelBonus`) que a sonda `audit_no_recalculation.sh` confunde com recálculo proibido.

Consequência prática, decidida antes de qualquer outra coisa: **todo o cálculo desta feature é geometria e decisão de montagem, e nenhum grama dele calcula dano, atributo ou resultado.** O encontro decide *quem* entra e *quando*; o `BattleSim` continua sendo o único que decide *o que acontece*. Se em algum momento um arquivo desta feature precisar multiplicar um `Attack`, o design está errado.

## DP-enc-01: O pet do mundo é um ator, e ele só carrega dados

**Decisão:** `AWorldEncounterActor` (novo, em `Source/BattleSquare/{Public,Private}/World/`), um `AActor` com:
- `FName CatalogId` — qual pet é, resolvido pelo `FPetDataLoader` que já existe
- `float EncounterRadiusUnits` — raio de disparo
- `bool bIsResolved` — encontro já consumido, não dispara de novo
- um `UStaticMeshComponent` de placeholder para ser visível

**Razão:** o ator é **dado posicionado**, não comportamento. Nada de `Tick`, nada de IA, nada de `UPrimitiveComponent` de overlap. Isso não é minimalismo por preguiça: é o que torna a detecção testável headless (DP-enc-02). Um ator que dispara por evento de overlap da engine só é verificável com física real rodando, e este projeto já decidiu (AD-004, DP-streaming-05) que verificação que depende de subsistema assíncrono da engine é verificação frágil.

**Sobre a malha de placeholder:** o cubo padrão da engine, como em `streaming-de-mundo`. Arte é conteúdo, e conteúdo está fora de escopo pela spec.

## DP-enc-02: A detecção é uma função pura, e o componente só a chama

**Decisão:** duas peças separadas.

1. `FEncounterDetector::FindTriggeredEncounter(const FEncounterDetectionParams&)` — **função estática pura**, sem `UWorld`, sem ator, sem tempo. Recebe a posição do pawn e um `TArray<FEncounterCandidate>` (cada um: posição, raio, `CatalogId`, `bIsResolved`) e devolve o índice do encontro disparado, ou `INDEX_NONE`. Comparação por distância **ao quadrado**, para não pagar `Sqrt` e não introduzir imprecisão desnecessária.
2. `UEncounterDetectionComponent` — `UActorComponent` no pawn do jogador, que a cada `TickComponent` coleta os `AWorldEncounterActor` do mundo, monta os `FEncounterCandidate`, chama a função pura, e dispara um delegate quando ela devolve um índice válido.

**Razão:** esta é a mesma separação que `FPetCollectionService` já pratica (`CaptureIfNewInMemory` puro, persistência à parte) e que fez aquela feature ser testável sem disco. Aqui ela permite testar toda a regra de disparo — entra no raio, não entra, empate entre dois candidatos, candidato já resolvido — sem montar um `UWorld`.

**Desempate quando dois pets estão no raio ao mesmo tempo:** o **mais próximo** vence. Se houver empate exato de distância, o de **menor índice** no array vence. A segunda metade dessa regra existe para o resultado ser determinístico, não porque o empate importe: sem ela, a ordem de iteração do mundo decidiria, e isso é o tipo de não-determinismo silencioso que este projeto já recusou em AD-004.

## DP-enc-03: A transição não troca de nível — a arena nasce longe

**Decisão:** `UWorldBattleTransitionService`, que ao receber um encontro:
1. **captura** o `FTransform` do pawn do jogador e desliga o `UEncounterDetectionComponent`;
2. **spawna** o `ABattleArena` num deslocamento fixo e nomeado (`BattleArenaWorldOffsetUnits`), longe da área jogável do mundo, no **mesmo `UWorld`**;
3. monta a partida pelos caminhos que já existem (`ABattleSquareGameMode`, `FPetDataLoader`, `ApplyOwnedPetProgressionBonus`);
4. ao receber `BatalhaEncerrada`, **destrói** a arena, restaura o `FTransform` capturado, marca `bIsResolved` no `AWorldEncounterActor` do encontro, e **só então** religa o componente de detecção.

**Razão:** `OpenLevel` mataria o mundo carregado e traria de volta exatamente a tela de loading que a feature anterior gastou um marco inteiro para eliminar (P2). Spawnar no mesmo `UWorld` com deslocamento é o que preserva as células carregadas — voltar é instantâneo porque nada foi descarregado.

**O deslocamento é uma constante nomeada, não um número solto** (§16 do `code-standart.md`), e é grande o bastante para a arena cair fora de qualquer célula de World Partition do mundo de teste (a feature anterior definiu a área de referência em 8000x8000uu).

**Ordem do passo 4 é a regra, não detalhe:** religar a detecção *depois* de marcar `bIsResolved` é o que satisfaz o critério 4 de P1 — o jogador volta parado em cima do pet derrotado e nada dispara. Fazer na ordem inversa reintroduz o bug que o critério existe para proibir.

## DP-enc-04: O que a batalha devolve já é o que os serviços existentes fazem

**Decisão:** esta feature **não escreve nenhuma linha de captura nem de XP**. `ABattleArena::GrantExperienceIfOwned` e o caminho de captura de `colecao-e-captura` já rodam ao fim da batalha, e já são testados. A transição só precisa não atrapalhá-los: destruir a arena **depois** que `BatalhaEncerrada` foi processado, nunca antes.

**Razão:** é literalmente a promessa que `colecao-e-captura` registrou ao ser escrita. Reimplementar aqui criaria dois caminhos de captura, e o segundo seria o não testado.

## DP-enc-05: O que é automatizável, e o que não é

| Verificação | Automatizável? | Como |
|---|---|---|
| Regra de disparo (entra/não entra/mais próximo/já resolvido/empate) | ✅ Sim | Teste headless puro sobre `FEncounterDetector`, sem `UWorld` |
| Montagem carrega o `CatalogId` certo nos dois lados | ✅ Sim | Teste headless sobre o serviço de transição, com `FPetDataLoader` real e catálogo de fixture |
| Posição restaurada é idêntica à capturada | ✅ Sim | Teste headless: capturar, transicionar, resolver batalha forçada, comparar `FTransform` |
| Não dispara segundo encontro ao voltar parado em cima do pet | ✅ Sim | Teste headless — é ordem de operações, não física |
| Ausência de tela de loading na transição | ❌ Não | Mesma razão de DP-streaming-05 — roteiro manual em `docs/verification/` |
| Células do mundo continuam carregadas durante a batalha | ❌ Não | `stat streaming` com o Editor aberto — roteiro manual |

**Precedente:** idêntico ao que `streaming-de-mundo` e `apresentacao-combate-visual` já fizeram. A divisão não é nova.

---

## O que muda

- **`AWorldEncounterActor`** (novo, `BattleSquare/World/`) — dados posicionados, sem comportamento
- **`FEncounterDetector`** (novo, `BattleSquare/World/`) — função pura de disparo
- **`UEncounterDetectionComponent`** (novo, `BattleSquare/World/`) — cola entre o mundo e a função pura
- **`UWorldBattleTransitionService`** (novo, `BattleSquare/World/`) — captura/spawn/restaura
- **Nível `WorldStreamingTest`** — ganha alguns `AWorldEncounterActor` posicionados na rota do `DebugRoutePawn`, e o pawn ganha o `UEncounterDetectionComponent`

## O que NÃO muda

- **`BattleSim`:** nenhuma linha. O núcleo não sabe que existe mundo, encontro ou transição.
- **`ABattleArena`, `UBattleTurnCoordinator`, `FPetCollectionService`, `FPetProgressionService`:** consumidos como estão. Se algum precisar mudar, isso é sinal de que o design acima errou, e a mudança precisa de justificativa própria.
- **As três sondas** (`audit_determinism.sh`, `audit_no_recalculation.sh`, `probe_isolation.sh`): continuam válidas e devem continuar limpas. **Atenção de L-022:** se `World/` passar a fazer cálculo de atributo, ele entra na lista de exclusão de `audit_no_recalculation.sh` — mas pelo DP-enc-01 ele não deve, e se precisar, o design está errado antes da sonda estar.
