# A duplicação suspeita entre "a arena é o lugar" e "a arena veste o bioma"

**Respondida em 02/09/2026**, contra o `⚠️` registrado no `ROADMAP.md` (seção
M8, item "A arena é o LUGAR"):

> Há uma duplicação por conferir: a outra frente entregou "a arena veste o
> bioma de onde o encontro aconteceu (item 16)" no mesmo dia — dois sistemas
> decidindo a mesma coisa, e ninguém verificou se concordam.

## A pergunta

Duas frentes tocaram a arena no mesmo dia (30/08/2026). Uma faz o TABULEIRO
nascer do pedaço de mundo onde o encontro aconteceu (árvore vira casa
bloqueada, rio vira água funda). A outra faz a arena VESTIR o bioma de onde o
encontro veio (mata, serra, paleta do chão). É a mesma decisão contada duas
vezes — e por isso capaz de discordar — ou são duas perguntas diferentes que
só parecem uma pelo nome "arena" em comum?

## O que foi medido

### Caminho 1 — o TABULEIRO (layout: bloqueio, água)

`FEncounterMatchAssembler::AssembleFromEncounter`
(`Source/BattleSquare/Private/World/EncounterMatchAssembler.cpp:23`) monta o
`FBattleState` inicial. Quando há amostras de mundo, ele delega a
`FArenaFromWorld::Build` (linha 143) e escreve o resultado direto no layout:

```cpp
// EncounterMatchAssembler.cpp:143-146
const TArray<uint8> DoMundo = FArenaFromWorld::Build(Terreno, &FluidosDoMundo);
if (DoMundo.Num() == OutInitialState.CellLayout.Num())
{
    OutInitialState.CellLayout = DoMundo;
```

`FArenaFromWorld::Build` (`Source/BattleSquare/Public/World/ArenaFromWorld.h:88`,
implementação pura, sem `UWorld`) só enxerga um vocabulário de CINCO terrenos
(`EWorldFeatureKind::Solid/DeepWater/Shore/TrainingGround`) — nada de bioma.
Quem alimenta essas amostras é `FArenaWorldSampler::Collect`
(`Source/BattleSquare/Public/World/ArenaWorldSampler.h:21`), chamado em
`UWorldEncounterFlow::HandleEncounterTriggered`
(`Source/BattleSquare/Private/World/WorldEncounterFlow.cpp:41-42`), no ponto
onde o ENCONTRO aconteceu (`Encounter->GetActorLocation()`).

O teste que cobra este lado é `ArenaFromWorldTest.cpp`: só afirma
`ECellProperty::Blocked/Water/ShallowWater/None` a partir de
`EWorldFeatureKind`. Não menciona bioma nenhuma vez.

### Caminho 2 — a APARÊNCIA (mata, serra, paleta do chão)

`ABattleArena::AdoptAmbienceFromWorldLocation`
(`Source/BattleSquare/Private/Battle/BattleArena.cpp:931`) faz duas coisas, na
ordem:

```cpp
// BattleArena.cpp:938-943
EncounterBiome = IslandGeography::BiomeAt(FVector2D(WorldLocation));
RebuildScenaryForBiome();
```

`RebuildScenaryForBiome` (linha 911) só toca `ForestBackdrop->BuildForest(...)`
(a mata: espécie, densidade, chão do bioma) e `MountainRange->BuildRange(...)`
(a serra, pelo clima do bioma via `ResolveScenaryClimate`, linha 877). Nenhuma
das duas escreve em `CellLayout`, `CellFluid` ou qualquer campo de
`FBattleState` — o alvo é `ForestBackdrop`/`MountainRange`, atores de cenário
independentes do tabuleiro. O restante da função (o *line trace* por chão e
`AdoptedFloorMaterial`) decide só a paleta do material herdado, e falha
graciosamente sem chão sondável (`FArenaKeepsOwnPaletteWithoutGroundTest`).

Quem chama esta função é `UWorldBattleTransitionService::BeginTransition`
(`Source/BattleSquare/Private/World/WorldBattleTransitionService.cpp:49`), e o
ponto amostrado é `CapturedPawnTransform.GetLocation()` — a posição do PAWN do
jogador no instante da transição, não a do encontro (ver seção "Uma
assimetria menor" abaixo).

Os testes deste lado (`ArenaGeometryTest.cpp`: `AdoptsAmbienceFromWorld`,
`KeepsOwnPaletteWithoutGround`, `WearsEncounterBiome`, `LearnsBiomeWithoutGround`,
`WithoutEncounterKeepsConfiguredClimate`) afirmam `EncounterBiome`,
`GetAdoptedFloorMaterial()`, `GetForestBackdrop()->GetRegionGroundRole()` e
`ResolveScenaryClimate()`. Nenhum deles lê `CellLayout`.

### A ordem real de execução (decidível por leitura, sem precisar de sonda)

Em `UWorldEncounterFlow::HandleEncounterTriggered`
(`Source/BattleSquare/Private/World/WorldEncounterFlow.cpp:31-67`), a sequência
é:

1. `FArenaWorldSampler::Collect(...)` — amostra o mundo no local do ENCONTRO.
2. `FEncounterMatchAssembler::AssembleFromEncounter(...)` — grava `CellLayout`
   no `FBattleState` (ainda sem arena viva).
3. `TransitionService->BeginTransition(...)` — nasce a `ABattleArena` e, dentro
   dela, roda `AdoptAmbienceFromWorldLocation` (grava `EncounterBiome`, refaz
   mata/serra).
4. `Arena->BeginBattle(InitialState, Presentations)` — copia o `FBattleState`
   pronto para `CurrentState` e só então chama `ApplyArenaLayoutIfNeeded()`.

`ApplyArenaLayoutIfNeeded` (`BattleArena.cpp:1346`) é o antigo caminho por
`Arenas.json`, e ele mesmo se declara subordinado:

```cpp
// BattleArena.cpp:1350-1355
// Montagem que já trouxe terreno manda: só preenche quem chegou neutro.
// Sobrescrever apagaria a arena escolhida por quem montou a partida.
const bool bTodaNeutra = !CurrentState.CellLayout.ContainsByPredicate(
    [](uint8 Propriedade) { return Propriedade != static_cast<uint8>(ECellProperty::None); });
if (!bTodaNeutra) { return; }
```

Ou seja: o CAMPO `CellLayout` só é escrito UMA vez por partida nascida de
encontro (pelo Caminho 1, no passo 2); o catálogo antigo só entra quando não
houve mundo por trás (batalha avulsa, teste). E o campo `EncounterBiome` só é
escrito uma vez também (pelo Caminho 2, no passo 3). Não há dois escritores
disputando o mesmo campo — são dois campos diferentes, escritos por dois
sistemas diferentes, cada um com seu próprio "só se ninguém decidiu antes".

## O veredito

**Dividem o trabalho sem se sobrepor.** `FEncounterMatchAssembler::AssembleFromEncounter`
(com `FArenaFromWorld::Build`) decide o TABULEIRO — bloqueio e água, campo por
campo da grade; `ABattleArena::AdoptAmbienceFromWorldLocation` (com
`RebuildScenaryForBiome`) decide a APARÊNCIA — mata, serra e paleta do chão. Um
não lê o que o outro escreveu, e cada lado tem teste próprio que nunca cita o
campo do outro. A suspeita nasceu de dois sistemas terem tocado "a arena" no
mesmo dia — mas "arena" aqui é o nome do ator, não o nome da decisão; dentro
dele moram um `TArray<uint8> CellLayout` (regra de jogo) e um
`TOptional<EIslandBiome> EncounterBiome` + dois atores de cenário (regra de
tela), e as duas frentes tocaram exatamente um cada.

### Frase para substituir o ⚠️ no ROADMAP.md (proposta — não editado aqui)

> Duplicação conferida (02/09/2026): não há sobreposição. `EncounterMatchAssembler`
> decide o tabuleiro (bloqueio, água) a partir de `ArenaFromWorld`; `ABattleArena`
> decide a aparência (mata, serra, paleta) a partir do bioma do lugar. Campos
> diferentes, escritores diferentes, testes diferentes — ver
> `ACHADOS-duplicacao-bioma.md`.

## Uma assimetria menor — não é a duplicação perguntada, mas é fato, não hipótese

Os dois caminhos amostram o mundo em PONTOS diferentes:

- O tabuleiro (Caminho 1) amostra em `Encounter->GetActorLocation()`
  (`WorldEncounterFlow.cpp:41`) — a posição do INIMIGO do encontro.
- A aparência (Caminho 2) amostra em `CapturedPawnTransform.GetLocation()`
  (`WorldBattleTransitionService.cpp:24,49`) — a posição do PAWN do jogador no
  instante em que a transição começa.

Isto é lido no código, não hipótese: são duas variáveis com dono diferente
(`AWorldEncounterActor` vs. `WorldPawn`), então em geral são pontos distintos.
Quão distantes ficam na prática — e se a distância chega a cruzar a fronteira
de um bioma na borda de um setor de `IslandGeography` — **não é decidível por
leitura**: depende de a que distância o `EncounterDetectionComponent` dispara o
encontro em relação ao raio dos setores de bioma, e isso só se mede em runtime
(colocar os dois pontos no painel de debug — `FBattleDebugScreen::Show` — numa
sessão em PIE perto de uma fronteira de bioma, e comparar `IslandGeography::BiomeAt`
nos dois). Não vira tarefa de consolidação porque as duas perguntas ("que
terreno tem aqui" vs. "que bioma é isto") são legitimamente diferentes; fica
registrado só para o caso de alguém um dia ver grama de floresta crescendo ao
lado de uma casa de gelo bloqueada.
