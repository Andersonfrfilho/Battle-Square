# Traversal e Câmera de Mundo — Especificação

**Status:** Draft — aguarda aprovação
**Depende de:** Streaming de Mundo e Encontros e Transição para Batalha (as duas primeiras features de M5, ✅ concluídas). Terceira e última feature de M5.

---

## Problem Statement

O mundo existe (streaming, células, HLOD) e os encontros funcionam (o pet no mundo vira batalha e a batalha volta ao mundo). Falta a única coisa que o jogador de fato faz entre um encontro e outro: **andar**.

Hoje quem se move é o `DebugRoutePawn` — um `ADefaultPawn` que segue waypoints fixos com `UDebugRouteMoverComponent`. Ele foi escrito para **provar streaming sem input humano**, e cumpriu isso: é determinístico, testável headless, e não depende de ninguém segurando uma tecla. Mas ele voa em linha reta, atravessa geometria, não tem gravidade e ignora o teclado. Não é traversal — é uma régua que se move.

**O que falta é o pawn do jogador de verdade:** um personagem que anda pelo chão, colide com o cenário, é dirigido por input, e é seguido por uma câmera que o mantém legível enquanto ele atravessa as células do mundo.

**Restrição que herdamos e não vamos quebrar:** o `DebugRoutePawn` **continua existindo**. Ele é o que torna o roteiro de verificação de streaming reproduzível sem um humano no teclado, e os dois roteiros manuais (`streaming-de-mundo.md`, `encontros-transicao-batalha.md`) dependem dele. Traversal **soma** um pawn de jogador; não substitui o de debug.

**E o núcleo continua intocado:** `BattleSim` não sabe que existe mundo, encontro ou câmera. Andar não calcula dano.

## Goals

- [ ] Existe um pawn de jogador que anda pelo mundo com input real, colidindo com o chão e o cenário — não voando nem atravessando geometria
- [ ] Uma câmera acompanha esse pawn de forma legível durante a travessia, sem atravessar parede e sem exigir que o jogador a corrija a cada passo
- [ ] O movimento é relativo à câmera (para frente é "para onde eu olho"), que é a convenção que qualquer jogador de terceira pessoa já traz de casa
- [ ] O pawn de jogador dispara encontros exatamente como o pawn de debug — reusando o `UEncounterDetectionComponent` que já existe, sem um segundo caminho
- [ ] O `DebugRoutePawn` continua funcionando e continua sendo o que os roteiros manuais usam

## Out of Scope

| Item | Razão |
|---|---|
| Animação de personagem (locomoção, blend spaces, ABP) | Autoria de conteúdo/arte. Esta feature entrega o movimento e a câmera; a cápsula pode andar sem animar, e o roteiro manual assume isso |
| Pulo, corrida, agachar, nadar, escalar | Verbos de traversal adicionais. O MVP é andar; cada verbo extra é decisão de design de jogo que ninguém tomou ainda |
| Câmera em primeira pessoa, ou troca de ombro | Uma câmera bem resolvida vale mais que três meia-boca. Terceira pessoa é a que serve a um jogo de pets com encontros visíveis (DP-enc-02: o jogador precisa VER o pet a distância) |
| Input de gamepad e de toque | Gamepad é barato de somar depois pelo mesmo `InputMappingContext`; toque é M6 (Mobile), onde o device real existe para validar |
| Replicação do traversal em rede | M2 replicou só a batalha. Explorar em rede é escopo novo, já declarado fora em `encontros-transicao-batalha` |
| Substituir o `DebugRoutePawn` | Ele é o instrumento de verificação reproduzível dos dois roteiros anteriores. Removê-lo quebraria a verificação de duas features já entregues |

---

## User Stories

### P1: Andar pelo mundo com input, colidindo com o cenário ⭐ MVP

**User Story:** Como jogador, quero andar pelo mundo com o teclado e esbarrar no cenário como um corpo esbarra, para que o mundo pareça um lugar e não um plano de fundo.

**Acceptance Criteria:**
1. WHEN o jogador aciona a entrada de movimento THEN o sistema SHALL mover o pawn na direção correspondente, **relativa à orientação da câmera** (frente é para onde a câmera olha, não um eixo fixo do mundo)
2. WHEN o pawn encontra geometria sólida THEN o sistema SHALL impedi-lo de atravessá-la, e SHALL mantê-lo apoiado no chão sob gravidade
3. WHEN nenhuma entrada de movimento está ativa THEN o sistema SHALL parar o pawn, sem deriva
4. WHEN o jogador se move THEN o sistema SHALL manter o pawn como fonte de streaming do World Partition, carregando e descarregando células como a primeira feature já provou

**Independent Test:** a conversão de entrada + orientação de câmera para vetor de movimento no mundo é uma função pura, testada headless: para frente com a câmera a 0° dá +X; com a câmera a 90° dá +Y; entrada nula dá vetor nulo; entrada diagonal é normalizada (andar na diagonal não é mais rápido).

---

### P1: Câmera que segue e não atravessa parede ⭐ MVP

**User Story:** Como jogador, quero que a câmera me siga sem entrar dentro de paredes e sem eu precisar ajeitá-la, para que eu consiga olhar para onde vou em vez de lutar contra o enquadramento.

**Acceptance Criteria:**
1. WHEN o pawn se move THEN o sistema SHALL manter a câmera a uma distância nomeada atrás dele, sem número mágico espalhado no código
2. WHEN existe geometria entre a câmera e o pawn THEN o sistema SHALL aproximar a câmera para que o pawn continue visível, em vez de deixar a câmera dentro da parede
3. WHEN o jogador aciona a entrada de olhar THEN o sistema SHALL girar a câmera ao redor do pawn
4. WHEN a câmera gira para cima ou para baixo THEN o sistema SHALL limitar o ângulo, para nunca chegar a um enquadramento invertido ou olhando para o próprio topo da cabeça

**Independent Test:** headless, sobre o pawn montado: braço de câmera existe, usa o comprimento da constante nomeada, tem teste de colisão ativo, e os limites de pitch do controlador estão configurados dentro da faixa declarada.

---

### P1: O pawn de jogador dispara encontros ⭐ MVP

**User Story:** Como jogador, quero que encostar num pet enquanto exploro comece a batalha, para que a feature anterior valha andando de verdade e não só na rota de debug.

**Acceptance Criteria:**
1. WHEN o pawn de jogador entra no raio de um `AWorldEncounterActor` não resolvido THEN o sistema SHALL disparar o encontro pelo `UEncounterDetectionComponent` já existente, sem nenhum caminho de detecção novo
2. WHEN a batalha termina THEN o sistema SHALL devolver o pawn de jogador à posição capturada, pelo `UWorldBattleTransitionService` já existente

**Independent Test:** headless, montando o pawn de jogador e um encontro no raio, confirmar que o disparo acontece pelo mesmo componente e que o serviço de transição é o mesmo — nenhuma classe nova de detecção ou transição.

---

### P2: O pawn de debug continua intacto

**User Story:** Como quem verifica o projeto, quero que a rota determinística continue existindo, para que os roteiros manuais das duas features anteriores continuem reproduzíveis.

**Acceptance Criteria:**
1. WHEN a feature é entregue THEN o sistema SHALL manter `UDebugRouteMoverComponent` e o `DebugRoutePawn` do nível funcionando exatamente como antes
2. WHEN os testes rodam THEN o sistema SHALL manter verdes todos os testes de `BattleSquare.World.DebugRouteMoverComponent`

**Independent Test:** a bateria existente continua passando, sem nenhuma linha alterada nesses arquivos.

---

## Decisões Pendentes

Marcadas como **PROPOSTA** — decisões de produto. Se você discordar, mudam antes do design.

1. **Terceira pessoa, câmera atrás do ombro.** PROPOSTA: é a única que serve a DP-enc-02 (o jogador precisa ver o pet a distância para decidir abordar). Primeira pessoa esconderia justamente o que a feature anterior tornou visível.
2. **Andar é o único verbo.** PROPOSTA: sem pulo/corrida no MVP. Cada verbo novo é conteúdo de design que ainda não foi decidido, e é barato somar depois no mesmo `InputMappingContext`.
3. **O pawn de jogador não é o de debug.** PROPOSTA: dois pawns coexistem, com papéis diferentes. Unificar economizaria uma classe e custaria a reprodutibilidade de dois roteiros já entregues.
