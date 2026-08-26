# Encontros e Transição para Batalha — Especificação

**Status:** Draft — aguarda aprovação
**Depende de:** Streaming de Mundo (primeira feature de M5, ✅ concluída) — existe um nível com World Partition e um pawn que se move nele. Depende também de M1–M4: `ABattleArena`, `UBattleTurnCoordinator`, `FPetCollectionService` (captura) e `FPetProgressionService` (XP) já existem e já funcionam.

---

## Problem Statement

Hoje existem duas metades que nunca se tocam.

De um lado, **a batalha**: `ABattleArena` é spawnada sob demanda, recebe dois pets montados por `ABattleSquareGameMode`, resolve o combate pelo `BattleSim` e, ao terminar, concede XP (`GrantExperienceIfOwned`) e captura o pet derrotado se ele ainda não estiver na coleção. Ela funciona — 129 testes provam — mas **alguém precisa mandá-la existir**. Hoje quem manda é um código de sala digitado por um humano (M2), ou um teste.

Do outro lado, **o mundo**: desde a feature anterior existe um nível com World Partition onde um pawn atravessa células que carregam e descarregam. Ele funciona — mas **não há nada nele**. É cenário sem consequência: o pawn anda, a memória sobe e desce, e nenhuma dessas duas coisas produz uma partida.

**Esta feature é a costura entre as duas.** O jogador anda pelo mundo, encontra um pet, e a batalha nasce daquele encontro — não de um código digitado. Quando a batalha acaba, o jogador volta para o mundo, no lugar onde estava, com o resultado do combate já refletido na sua coleção e na sua progressão.

Isto é o que a captura já esperava: `colecao-e-captura` foi escrita com a ressalva explícita de que a captura dispara por vitória "já que M5 (Mundo Aberto) ainda não existe para dar um mecanismo de encontro". **Esta feature é aquele mecanismo de encontro.**

**O que NÃO muda, e é a restrição mais importante desta spec:** `BattleSim` não é tocado. O núcleo determinístico não sabe que existe um mundo, e não vai saber. O encontro decide *quais pets* entram na batalha e *quando* ela começa — decisões de montagem, na mesma categoria arquitetural de `TranslateMatchup` e `ApplyOwnedPetProgressionBonus` (L-021/L-022), que rodam **antes** do núcleo ver o estado inicial.

## Goals

- [ ] Existem pets visíveis no mundo, que o jogador pode ver e evitar ou abordar — o encontro é uma decisão, não um sorteio invisível
- [ ] Abordar um pet no mundo inicia uma batalha real, com o `ABattleArena` e o `BattleSim` que já existem — zero caminho de combate paralelo
- [ ] Ao fim da batalha, o jogador volta ao mundo na posição onde estava, e o resultado (XP, captura) já está aplicado pelos serviços que já existem
- [ ] A transição mundo → batalha → mundo acontece sem tela de carregamento, coerente com a promessa que a feature anterior estabeleceu
- [ ] O estado do mundo sobrevive à batalha: as células carregadas, a posição do jogador e os outros pets no mundo continuam válidos ao voltar

## Out of Scope

| Item | Razão |
|---|---|
| Traversal e câmera de mundo definitivos | Terceira feature de M5 ("Traversal e Câmera de Mundo"). Esta feature usa o pawn de debug/mínimo que já existe — ela prova o encontro, não a sensação de andar |
| IA de movimento dos pets no mundo (patrulha, perseguição, fuga) | Comportamento de mundo é conteúdo; esta feature precisa que o pet **exista e seja abordável**, não que ele seja esperto. Um pet parado no lugar prova o encontro |
| Encontros multiplayer (dois jogadores vendo o mesmo pet no mundo) | M2 replicou só a batalha. Replicar entidades de mundo aberto é escopo novo e grande — a decisão de M2 (`ListenServer` nunca é competitivo) nem sequer se aplica aqui |
| Fuga da batalha ("correr") | Muda o `BattleSim` (um resultado novo além de vitória/derrota/empate). Se for desejado, é feature de combate, não de encontro |
| Balanceamento de quais pets aparecem onde (tabelas de encontro por bioma/nível) | Depende de conteúdo de mundo que não existe (não há biomas — a feature anterior entregou 225 cubos coloridos). Esta feature precisa de um mecanismo de spawn, não de uma curva de balanceamento |
| Persistência do mundo entre sessões (qual pet já foi derrotado, respawn com o tempo) | Depende de save de mundo, que não existe. `UPetCollectionSaveGame` salva a coleção, não o estado do mundo |

---

## User Stories

### P1: Pet visível no mundo inicia uma batalha ao ser abordado ⭐ MVP

**User Story:** Como jogador, quero ver um pet no mundo e escolher me aproximar dele para batalhar, para que o combate seja consequência de uma decisão minha no mundo, e não de um menu.

**Acceptance Criteria:**
1. WHEN o jogador está explorando o nível de mundo THEN o sistema SHALL exibir pets em posições do mundo, visíveis a distância, cada um associado a um `CatalogId` do catálogo de pets
2. WHEN o pawn do jogador entra no raio de encontro de um pet do mundo THEN o sistema SHALL iniciar a transição para a batalha, exatamente uma vez por encontro (nunca disparar duas vezes pelo mesmo pet)
3. WHEN a batalha começa THEN o sistema SHALL montá-la com o pet do jogador de um lado e o pet do encontro (o `CatalogId` daquele ator no mundo) do outro, usando `ABattleSquareGameMode` e `ABattleArena` já existentes
4. WHEN o jogador passa perto de um pet SEM entrar no raio de encontro THEN o sistema SHALL deixá-lo seguir sem iniciar batalha alguma

**Independent Test:** com o mundo montado headless e um pet de encontro numa posição conhecida, mover o pawn programaticamente para dentro e para fora do raio e verificar: (a) exatamente um disparo de encontro ao entrar, (b) nenhum disparo ao passar fora do raio, (c) que a montagem resultante carrega o `CatalogId` correto nos dois lados.

---

### P1: Voltar ao mundo com o resultado aplicado ⭐ MVP

**User Story:** Como jogador, quero que ao fim da batalha eu volte para onde eu estava no mundo, com o que ganhei já valendo, para que explorar e batalhar sejam a mesma sessão contínua e não dois jogos separados.

**Acceptance Criteria:**
1. WHEN a batalha termina (`BatalhaEncerrada`, por qualquer resultado) THEN o sistema SHALL devolver o controle ao mundo, com o pawn do jogador na mesma posição e orientação em que estava quando o encontro disparou
2. WHEN o jogador vence contra um pet cujo `CatalogId` ele ainda não possui THEN o sistema SHALL capturá-lo, pelo `FPetCollectionService` que já existe — sem nenhum caminho de captura novo
3. WHEN a batalha termina THEN o sistema SHALL conceder XP pelo `FPetProgressionService` que já existe, nas mesmas regras de `GrantExperienceIfOwned`
4. WHEN o jogador volta ao mundo THEN o sistema SHALL garantir que o pet do encontro já resolvido não dispare outra batalha imediatamente ao jogador ainda estar dentro do raio

**Independent Test:** headless, com um encontro disparado e uma batalha resolvida por um resultado forçado, verificar que a posição restaurada é bit-idêntica à capturada, que a coleção mudou exatamente como `FPetCollectionService` já é testado a mudar, e que um segundo tick com o pawn parado no mesmo lugar não dispara um segundo encontro.

---

### P2: Transição sem tela de carregamento

**User Story:** Como jogador, quero que entrar em batalha e voltar não tenha tela de loading, para que a promessa de mundo contínuo não se quebre exatamente no momento mais frequente do jogo.

**Acceptance Criteria:**
1. WHEN a transição mundo → batalha acontece THEN o sistema SHALL fazê-la sem `OpenLevel`/`LoadStreamLevel` bloqueante — a arena nasce no mesmo `UWorld` do mundo aberto
2. WHEN a batalha está ativa THEN o sistema SHALL manter o mundo carregado ao redor, de forma que voltar seja instantâneo e não exija recarregar células

**Independent Test:** não automatizável de forma confiável (mesma razão de DP-streaming-05) — roteiro manual, medindo que o `stat streaming` durante a batalha ainda mostra as células do mundo carregadas.

---

## Decisões Pendentes

Marcadas como **PROPOSTA**: são decisões de produto, não técnicas. Se o usuário discordar de alguma, ela muda antes do `design.md`.

1. **Encontro é visível, não aleatório.** PROPOSTA: o pet fica no mundo, o jogador o vê e decide. Alternativa clássica (mato alto + sorteio invisível) foi descartada porque o jogo já é sobre decisão às cegas *dentro* da batalha (BTL-02) — somar aleatoriedade *antes* da batalha empilha duas camadas de sorte sobre a mesma decisão.
2. **O mundo não pausa durante a batalha.** PROPOSTA: a arena nasce no mesmo `UWorld`, deslocada para fora da área jogável (ou com o mundo simplesmente ignorado pela câmera), em vez de trocar de nível. Razão: é o único jeito de honrar P2 sem loading, e reaproveita `ABattleArena` exatamente como ela é.
3. **O pet derrotado some do mundo até o fim da sessão.** PROPOSTA: sem respawn. Persistência de mundo está fora de escopo, e um pet que volta imediatamente ao ser derrotado transforma o encontro num loop de farm acidental.
4. **Um pet por lado, como hoje.** PROPOSTA: o encontro não introduz N-vs-N. `ABattleArena` já monta 1v1; N pets por lado é escopo de M3 que não foi feito e não deve nascer de esgueira aqui.
