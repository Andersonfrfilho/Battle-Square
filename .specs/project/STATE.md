# State

**Última atualização:** 2026-08-26
**Trabalho atual:** **Esconderijos (DP-ia-04), fim da coabitação (DP-02), ferramental de controle dos dois jogadores, e guarda-queda no mundo.** Camuflar/voar/submergir entraram como REGRA com trocas distintas entre si — camuflar barra físico E magia e custa a ação seguinte; voar barra físico e o dano de casa, mas magia acerta 50% mais; submergir barra tudo e custa mover E atacar na seguinte. A cobrança do slot seguinte viaja dentro do `PostureFlags` para não entrar no hash do estado e invalidar snapshots de determinismo. **DP-02 foi invertida**: dois pets não ocupam a mesma casa; encontro no mesmo ponto vira golpe mútuo pelo MESMO caminho de dano do ataque, que é o que faz as posturas já registradas valerem sem reescrever nada. A decisão é por POSIÇÃO FINAL, porque checar contra o estado vivo bloquearia a troca de casas e dependeria da ordem de processamento. **Ferramental**: as ações do jogador 2 ficam sempre na barra desenhada em C++ e o turno fecha pelo botão normal do jogador 1 — três versões anteriores exigiam ligar um modo, trocar de lado e confirmar em duas etapas, e cada etapa a mais era uma chance de o caminho parecer destrutivo. O painel de depuração **se grava sozinho** em `Saved/BattleDebug.txt` a cada mudança, depois de a tecla de copiar falhar três vezes seguidas. **Movimento deslizando** substituiu o teleporte, com o olhar acompanhando a posição REAL (mirar a casa fazia a cabeça virar de repente e depois esperar). **Guarda-queda**: cair para fora do mundo devolve o explorador à última terra firme, com a velocidade zerada — sem isso ele reaparecia já caindo e atravessava o piso de novo. Bateria: **222/222** (159 BattleSquare + 63 BattleSim), quatro sondas limpas, Editor e Shipping compilam.

**Trabalho anterior:** **Traversal e Câmera de Mundo (M5, terceira feature) — completa (T1–T6). M5 ENCERRADO — as três features de mundo aberto entregues.** `FWorldTraversalMotion::ComputeMoveDirection` é a única parte do traversal que é decisão nossa (input 2D + rotação da câmera → direção no mundo) e por isso é pura e testável com números exatos: só o yaw entra na conta (com o pitch, olhar para baixo empurraria o personagem para dentro do chão), diagonal normalizada (senão andar na diagonal é ~41% mais rápido), entrada nula devolve vetor nulo sem normalizar. `AWorldExplorerCharacter` é `ACharacter` + `USpringArmComponent` (com `bDoCollisionTest`, que é o único motivo de a câmera não entrar na parede) + `UCameraComponent` sem rotação de controlador (a rotação vive num lugar só) + `bOrientRotationToMovement` com `bUseControllerRotationYaw` desligado — a combinação que separa "explorador" de "shooter andando de lado". Reusa o `UEncounterDetectionComponent` da feature anterior como subobjeto: **zero detecção nova**. Assets de Enhanced Input ficam fora do C++ por decisão (DP-trav-06), então a ausência deles é estado real e degrada para "não anda", com teste dedicado provando que não crasha. **O `DebugRoutePawn` NÃO foi substituído** (P2): ele é a régua determinística de que os roteiros manuais das duas features anteriores dependem. Nível ganhou 25 pisos (um por célula — laje única ficaria sempre carregada e confundiria a verificação de streaming) e o `WorldExplorer`. **Dois achados no processo:** (1) `Tools/sync_module_manifest.sh` — L-025 virou ferramenta em vez de disciplina, porque a defasagem do manifesto reincidiu mais uma vez; (2) um teste MEU crasheu o processo (`SetupPlayerInputComponent(nullptr)` desreferencia dentro da engine) e abortou a bateria antes dos testes seguintes, o que se manifestou como "só 2 dos 10 testes novos apareceram" — o código de produção estava certo, o teste é que era inválido. Bateria final: **164/164 (112 BattleSquare + 52 BattleSim), Fail: 0, três sondas limpas, L-020 aplicada.** Roteiro manual em `docs/verification/traversal-camera-mundo.md`, seis itens **não verificados ainda**; TRT-01 depende de criar os assets de Enhanced Input no Editor, que é o primeiro item da preparação. Próximo: **M6 — Plataformas**.

**Histórico — Encontros e Transição para Batalha (M5, segunda feature):** **Encontros e Transição para Batalha (M5, segunda feature) — completa (T1–T8).** A costura entre as duas metades que nunca se tocavam: o mundo (streaming, feature anterior) e a batalha (M1–M4). O jogador vê um pet no mundo, aborda, e a batalha nasce daquele encontro — não de um código de sala digitado. Quatro peças novas em `BattleSquare/World/`, todas testáveis headless: `FEncounterDetector` (função pura de disparo, sem `UWorld` e sem tempo — distância ao quadrado, mais próximo vence, empate exato resolve pelo menor índice para o resultado não depender da ordem de iteração do mundo), `AWorldEncounterActor` (dado posicionado, sem Tick/colisão/IA — há teste que trava `bCanEverTick` em false), `UEncounterDetectionComponent` (cola; o disparo desliga a própria detecção ANTES de anunciar, que é o que garante "exatamente uma vez") e `UWorldBattleTransitionService` (🧠). `FEncounterMatchAssembler` + `UWorldEncounterFlow` fecham a fiação. **`ABattleArena` ganhou `OnBattleFinished`** — única mudança em classe existente, aditiva e justificada: a arena não tinha como anunciar o próprio fim (`CheckForCapture`/`GrantExperienceIfOwned` varrem o trace em silêncio), e o delegate dispara sempre DEPOIS de captura e XP, o que permite ao ouvinte destruir a arena sem perder nenhum dos dois. A arena nasce no **mesmo `UWorld`**, deslocada de 1.000.000uu — `OpenLevel` traria de volta a tela de loading que a feature anterior gastou um marco para eliminar. A ordem da volta é a regra, não detalhe: destruir arena → restaurar transform → marcar resolvido → só então religar a detecção; invertida, o jogador volta parado em cima do pet derrotado e cai num segundo encontro (teste dedicado). Nível `WorldStreamingTest` ganhou 3 `AWorldEncounterActor` sobre a rota e o `DebugRoutePawn` ganhou o componente de detecção. **Ressalva honesta:** os `CatalogId` dos encontros são placeholders e não batem com o espelho de pets (o fixture é criptografado, os ids reais não são legíveis fora do `FPetDataLoader`) — com eles, a montagem é recusada de propósito e nenhuma batalha começa; trocar por ids reais é o primeiro item da preparação do roteiro manual. **Achado no processo — ver L-025**, que custou três diagnósticos falsos: `Binaries/Mac/UnrealEditor.modules` pode apontar para um dylib mais velho que o recém-buildado, e o teste novo simplesmente não aparece na contagem, sem erro nenhum. Bateria final: **154/154 (102 BattleSquare + 52 BattleSim), Fail: 0, três sondas limpas, L-020 aplicada.** `Source/BattleSim/` sem uma linha tocada. Roteiro manual em `docs/verification/encontros-transicao-batalha.md`, quatro itens **não verificados ainda** (DP-enc-05). Sessão em modo de execução contínua (`OBJECTIVE.md`). Próximo: **Traversal e Câmera de Mundo**, terceira e última feature de M5, do Specify em diante.

**Histórico — Streaming de Mundo (M5, primeira feature):** **Streaming de Mundo (M5, primeira feature) — completa (T1–T7).** Primeira feature do projeto com trabalho real de Editor, feito via `unreal-mcp` (plugin oficial da Epic) contra um Editor vivo. Nível `/Game/Maps/WorldStreamingTest` criado por duplicação de `/Engine/Maps/Templates/OpenWorld` — o mesmo template que o "New Level → Open World" do Editor usa, já que o toolset não expõe criação de nível. World Partition habilitado, `MainGrid` com **célula 1600uu / raio 3200uu**: os 512uu de DP-streaming-02 são **inalcançáveis**, a engine clampa em 1600 (`URuntimePartitionLHGrid::PostEditChangeProperty`) — emendado em DP-streaming-02a preservando a intenção (raio = 2 células, vizinhança 5x5), ver **L-023**. Conteúdo: 225 cubos (9 por célula numa grade 5x5, área 8000x8000uu) com 25 `MaterialInstanceConstant` de matiz distinta por célula. `DebugRoutePawn` (`ADefaultPawn`, `AutoPossessPlayer = Player0`) com o `UDebugRouteMoverComponent` de T1: rota de 3 waypoints, 21.600uu a 400 uu/s = **54s**. HLOD: `HLOD_StreamingPlaceholder` (Instancing) atribuída aos 225 cubos, **36 atores de HLOD construídos** pelo `WorldPartitionHLODsBuilder`. `BattleSim`/`BattleSquare` — zero linhas tocadas fora do `UDebugRouteMoverComponent` de T1, que já estava commitado. **Achado no processo — ver L-024:** um `set_properties` num `HLODLayer` travou o game thread do Editor por 18 minutos (diálogo modal, provavelmente por `editorLoadingBehavior: ForceLoaded`); recuperado matando e reabrindo o Editor, sem perda, porque tudo já estava salvo. Bateria final: **129/129 (77 BattleSquare + 52 BattleSim), Fail: 0, três sondas limpas, L-020 aplicada (rebuild real depois de `probe_isolation.sh`).** Roteiro manual em `docs/verification/streaming-de-mundo.md`, todos os cinco itens **não verificados ainda** — streaming real, ausência de loading e ganho de HLOD não são automatizáveis de forma confiável (DP-streaming-05). Sessão em modo de execução contínua (`OBJECTIVE.md`); gate B-001 já liberado pelo usuário. Próximo: **Encontros e Transição para Batalha** (segunda feature de M5), do Specify em diante.

**Histórico — Níveis, Experiência e Evolução (M4, segunda feature):** **Níveis, Experiência e Evolução (M4, segunda feature) — completa (T1–T6). M4 encerrado (as duas features planejadas concluídas).** `FPetProgressionService` (`Meta/`): `GetLevel` sempre derivado de `Experience` (nunca campo próprio, evita duas fontes de verdade), `GrantExperience` processa múltiplos níveis de uma vez, `ApplyLevelBonus` escala Attack/Defense/Speed/MaxHealth em aritmética inteira (`BonusPercent = 100 + (Nível-1)*5`) e restaura `Health = MaxHealth`. XP concedida ao FIM da batalha (`ABattleArena::GrantExperienceIfOwned`, ambos os caminhos Standalone e em rede) só ao pet do JOGADOR (`Side == LocalPlayerSide`) e só se ele já estiver na coleção — "sem fantasma" por decisão de design, funciona sem seleção de time. Bônus de nível é aplicado na MONTAGEM da partida (`ABattleSquareGameMode::ApplyOwnedPetProgressionBonus`, estático e testável isoladamente), antes do `BattleSim` ver o pet — mesma categoria arquitetural de `TranslateMatchup`. "Evolução" nesta spec é crescimento de atributo por nível no mesmo `CatalogId`, não troca de forma/espécie (catálogo não tem campo de cadeia evolutiva; mudar o schema do backend ficou fora de escopo). **Achado no processo — ver L-022**: mesmo falso positivo de L-021 reapareceu, agora em `Meta/` (`ApplyLevelBonus` sinalizado por `audit_no_recalculation.sh`); corrigido estendendo a mesma exclusão. Bateria final: **127/127 (75 BattleSquare + 52 BattleSim), Fail: 0, três sondas limpas, L-020 aplicada (rebuild real depois de `probe_isolation.sh` antes do run final que conta).** Sessão em modo de execução contínua (`OBJECTIVE.md`) — próximo: aproximar-se de B-001 (gate de M5, Mundo Aberto), que por decisão já registrada em `OBJECTIVE.md` continua exigindo parada explícita para o usuário decidir, mesmo sob a instrução de não parar.

**Histórico — Coleção e Captura (M4, primeira feature):** **Coleção e Captura (M4, primeira feature)** — `spec.md`/`design.md`/`tasks.md` escritos, **feature completa (T1–T6)**. `BattleSim` não foi tocado. Captura disparada por vitória em batalha contra um pet de catálogo (`CatalogId`) ainda não possuído — decisão do usuário, já que M5 (Mundo Aberto) ainda não existe para dar um mecanismo de encontro. Coleção é local (`UPetCollectionSaveGame`, `USaveGame`), não de conta (M7 não existe ainda). `FPetPresentationInfo` ganhou `CatalogId`/`Type`, preservados até `ABattleArena` para saber, no momento em que a batalha termina, qual pet capturar — a fronteira do núcleo já descarta essa informação de propósito. `FPetCollectionService` separa lógica pura (`CaptureIfNewInMemory`, testável sem disco) de persistência real. **Achado no processo: `Tools/audit_no_recalculation.sh` (L-021) tinha escopo grande demais** — sinalizava como violação o cálculo legítimo de `TranslateMatchup` (Escala de Pets e Skills, decisão já revisada), corrigido para excluir `Data/`/`Balance/` do escopo, com o porquê documentado no próprio script. Bateria final: **70/70 BattleSquare, 52/52 BattleSim, sondas limpas.** Sessão em modo de execução contínua (`OBJECTIVE.md`) — próximo: Níveis, Experiência e Evolução (segunda feature de M4), sem pausa entre fases. Diferente da feature anterior, esta TOCA `BattleSim` de verdade: `ECellProperty`/`CellLayout` (9 posições) viajam DENTRO de `FBattleState` (mesma razão de `FBattleRandom`, AD-004) e entram no `ComputeHash()`. Bloqueio reaproveita `MovimentoBloqueado` (zero evento novo); dano de casa soma em `PendingDamage` sem evento próprio (F5 já cobre — inclusive morte simultânea combinando dano de casa e combate, testada); buff é contextual (Attack ao atacar, Defense ao defender), nunca persiste em `FPetState`. `BattleSim` nunca importa `Json` — `FArenaLayoutCatalog` (nomeação/autoria) fica inteiramente em `BattleSquare`, mesmo padrão de `FTypeEffectivenessTable`. `ABattleArena::BeginBattle` agora retorna `bool`, rejeitando montagem com pet em casa bloqueada. **Dois bugs reais encontrados e corrigidos no processo:** (1) meu próprio bug de lógica — o early-return de `ApplyMovement` pulava o cálculo de dano de casa quando ninguém tentava mover; pego pelos próprios testes, corrigido reestruturando sem early-return. (2) **L-020**: `Tools/probe_isolation.sh` deixa o `.dylib` de `BattleSim` quebrado depois de rodar (planta sonda, espera falhar, remove, nunca recompila de volta) — 3 tentativas de teste falharam com "módulo não encontrado" até a causa ser isolada; disciplina nova: sempre rebuildar depois de qualquer sonda que planta código. Bateria final: **66/66 BattleSquare, 52/52 BattleSim, `audit_determinism`/`probe_isolation` limpos.** **M3 completo — as duas features planejadas (Escala de Pets e Skills, Arenas Variadas) estão concluídas.** Efetividade de tipo resolvida INTEIRAMENTE fora de `BattleSim` (DP-escala-01, design.md) — `Attack` chega pré-multiplicado ao núcleo, zero linhas mudadas em `BattleSim` pela feature em si. `FTypeEffectivenessTable` (JSON local, `Config/TypeEffectiveness.json`), `TranslateMatchup` (🧠, testado com caso assimétrico), `FBattleBalanceSimulator::RunBatchSimulation` (determinístico por seed, prova com 200 simulações que tipo super efetivo vence mais). **Bug real de produção encontrado e corrigido — ver L-019**: `BatalhaEncerrada` nunca disparava em M1/M2 porque nada chamava `BattleOutcome::EvaluateOutcome` fora dos testes isolados do núcleo; corrigido em `ABattleArena`, `UBattleTurnCoordinator` e no simulador novo. `BattleOutcome::EvaluateOutcome` também não tinha `BATTLESIM_API` (mesmo defeito de L-016, 2ª ocorrência). Bateria final: **64/64 BattleSquare, 44/44 BattleSim, `audit_determinism`/`audit_no_recalculation` limpos.** **M3 (primeira feature) encerrada.** Fases 1–2 (T1–T6): `UBattleRoomRegistry` — código único entre salas vivas, `CreateRoom`/`JoinRoom` distinguindo `NotFound`/`Full`, desconexão/reconexão por segredo (`FGuid`), timeout de abandono (🧠 T5), sala vazia destruída com código reciclável. Fase 3 (T7–T9): `ABattleSquareGameMode` (RoomRegistry, `Tick` chamando `CheckAbandonment`/`CheckEmptyRooms` 1x/s) + `ABattleSquarePlayerController` (RPCs finos) + `AssembleMatchForRoom` (monta `ABattleArena`+`UBattleTurnCoordinator` reais, testável sem tocar no espelho de pets) + `HandleRoomReady` (caminho de produção, carrega pets reais via `FPetDataLoader`). **Um SIGSEGV real foi encontrado e corrigido nesta fase — ver L-018**: `BeginPlay` não dispara de forma confiável num `UWorld` de teste montado à mão; `RoomRegistry` virou inicialização preguiçosa (`EnsureRoomRegistry()`, mesmo padrão de `ABattleArena::TracePlayer`). Fase 4 (T10–T11): regressão completa e extensão de `docs/verification/combate-online-rede.md` com os itens de sala que só rede real prova (Itens 4–5). Bateria final: **59/59 BattleSquare, 44/44 BattleSim, Fail: 0**. **Feature Sala e Pareamento Simples encerrada — M2 completo.**

**Histórico — Combate Online (M2), feature completa (T1–T14), Fases 1–4:** Fase 4: T10 `Tools/audit_no_commit_replication.sh` (sonda anti-replicação do commit, ciclo plantar→falhar→remover→limpo confirmado nos dois padrões — `UPROPERTY(Replicated)` e `GetLifetimeReplicatedProps`). T11 investigação de PIE multiplayer real: **não viabilizou** — `Build.sh BattleSquareServer` falha com `Server targets are not currently supported from this engine distribution` (engine do Epic Games Launcher não compila `TargetType.Server`; só builds de fonte compilam). Registrado como **AD-020/B-004**, blocker de infraestrutura, não de código. T12 pulada (dependia de T11). T13: `UBattleTurnCoordinator` ganhou `GetCurrentBattleState()` (reconexão sem replay de trace) e `DeclareAbandonment`/`OnAbandonment` (vitória por abandono via o `BatalhaEncerrada` já existente, nenhum evento novo). T14: `docs/verification/combate-online-rede.md`, roteiro manual para os 3 itens que só rede real prova, explicando o bloqueio de B-004. Bateria completa: **47/47 testes headless, Fail: 0**. **Feature Combate Online encerrada** (com a ressalva honesta de B-004 pendente de resolução de infraestrutura, fora do escopo de código). Fase 3: `UBattleNetCommitComponent` (T7) — lado cliente, `Server_SubmitCommit` valida ANTES de encaminhar (`WithValidation`), separado em `ValidateIncomingCommit`/`ProcessValidatedCommit` testáveis headless sem RPC real; resultado chega via `Client_ReceiveTurnResult` e expõe `OnResultReceived`. `ABattleArena::HandlePlayerCommitted` (T8) delega ao `UBattleTurnCoordinator` quando `ConfigureNetworkedOpponent` foi chamado — `FDumbOpponentAI` só roda por AUSÊNCIA de oponente humano, não por flag; provado por teste que o turno não resolve com só o commit do jogador quando há coordenador configurado (prova indireta de que a IA não foi chamada). T9: regressão completa — **44/44 BattleSquare, 44/44 BattleSim, Fail: 0** — `BattleSim` continua sem uma linha tocada. Sessão em `sonnet` por instrução explícita do usuário. Próximo: Fase 4 (T10–T14, sonda anti-replicação, investigação de PIE multiplayer real, reconexão/abandono, roteiro de verificação de rede). Fase 1: `FNetTurnCommit` (3 campos nomeados) + validação + conversão. Fase 2: `UBattleTurnCoordinator` (servidor) — acumula commit por lado sem `UPROPERTY(Replicated)` em nenhum campo (T4), dispara `FBattleResolver::ResolveTurn` real assim que os dois presentes, timeout injetado preenche o lado ausente com 3x Aguardar (T5), e a lógica de corrida (T6, 🧠): um commit que chega antes do timeout resolve na hora com os dois commits reais mesmo sem `CheckTimeout` ter sido chamado, e um commit que chega depois do timeout já ter resolvido é rejeitado sem crash. Tempo sempre injetado (nunca `FPlatformTime`/`GetWorld()`), o que tornou a corrida testável sem depender de tempo real. Bateria completa: **40/40 testes headless, Fail: 0** (34 anteriores + 6 novos). Sessão em `sonnet` por instrução explícita do usuário. Próximo: Fase 3 (T7–T9, `UBattleNetCommitComponent` lado cliente + fiação em `ABattleArena` + regressão Standalone). Design resolve os 4 decision points: DP-online-01 autoridade no código / topologia no deploy (com ressalva registrada de que `ListenServer` nunca é competitivo, porque o host lê o commit do oponente e isso quebra BTL-02); DP-online-02 RPC nativo (não serviço HTTP — contas de jogador não existem até M7); DP-online-03 reconexão por `FBattleState` completo, sem replay de trace; DP-online-04 timeouts como constantes nomeadas. **Decisão técnica derivada de investigação, não de preferência:** o commit não viaja como `FTurnCommit` (que tem array C estático `Actions[3]`), e sim como `FNetTurnCommit` de campos nomeados — verifiquei que o RPC *compila*, mas não consegui provar que `FRepLayout` expande o array interno em vez de serializar só `Actions[0]`, e esse modo de falha é silencioso. Ver L-017 (descoberta colateral dessa investigação). Fases 1–6 da Apresentação do Combate concluídas antes disto.

**Histórico — Apresentação do Combate — feature completa (T1–T14), Fases 1–6:** T13: `Tools/audit_no_recalculation.sh` — grep disciplinado contra `Attack *`/`* Attack`/`Defense -`/`- Defense`/`* 1.5` fora de `Source/BattleSim`; ciclo plantar→falha(1)→remover→limpo(0) confirmado manualmente. T14: `docs/verification/apresentacao-combate-visual.md` — roteiro dos itens P1 sem teste automatizado (PRES-04 alvo de toque, PRES-06/07/08 estética, layout DP-08, tilt-shift restrito à arena), todos marcados "não verificado ainda" até alguém rodar o roteiro no editor. **Feature Apresentação do Combate encerrada** — próximo trabalho é escolher a próxima feature do ROADMAP (M2 Combate Online, ou autoria visual/UMG assistida via MCP se disponível).

**Histórico — Fase 5 (T11–T12):** ver commit `00e31c7`.
**Histórico — Fases 1–4 (T1–T10):** T11 (`UBattleActionSelectorWidget`) e T12 (`UBattleResultWidget`) são classes base C++ de `UUserWidget` — só a exposição a Blueprint (`BindToQueue`, um `BlueprintCallable` por operação da fila, estado espelhado via delegates T2–T4; `ApplyBattleEndedEvent` traduzindo `WinningSide` do evento `BatalhaEncerrada` para Vitória/Derrota/Empate do ponto de vista do jogador local). Nenhum layout UMG criado — fica para autoria visual (DP-08, T14). Módulo `BattleSquare` passou a depender de `UMG`/`Slate`/`SlateCore`. Bateria completa: **30/30 testes headless, Fail: 0** (soma de T12: `BattleSquare.BattleResultWidget.ReadsAllThreeOutcomes`, 3 casos). Próximo: Fase 6 (T13 sonda anti-recálculo `Tools/audit_no_recalculation.sh`, T14 roteiro de verificação visual — não automatizado).

**Histórico — Fases 1–4 (T1–T10):** `UBattleTracePlayer::PlayTrace/SkipToEnd` (T7) despacha eventos via delegate multicast simples (não dinâmico, para permitir binding por lambda/AddUObject). `APetView` (T8) reage a eventos sem recalcular nada. `ABattleArena` (T9) tem câmera fixa em diorama com checagem de frustum programática (matemática de FOV/AspectRatio própria, sem depender de PlayerController/Viewport) e grade 3x3 configurável por material soft-pointer. Fiação completa (T10): `PlayerActionQueue` (componente) → `HandlePlayerCommitted` → `FDumbOpponentAI` gera commit do oponente → `FBattleResolver::ResolveTurn` roda de verdade → `UBattleTracePlayer` anima as `APetView`. Bateria completa de **29 testes rodada headless: Success 29 / Fail 0** (`BattleSquare.ActionQueue.*` 9, `BattleSquare.BattleArena.*` 2, `BattleSquare.BattleDataTranslator.*` 3, `BattleSquare.DumbOpponentAI.*` 4, `BattleSquare.PetCrypto.*` 2, `BattleSquare.PetDataLoader.*` 4, `BattleSquare.PetView.*` 1, `BattleSquare.TracePlayer.*` 4). Corrigido no processo: teste de `PetView` checava `TargetId` em vez de `ActorId` no evento `DanoAplicado` (esse evento carrega quem sofreu o dano em `ActorId`, `TargetId` é `BattleEventNoActor` — campo usado só em `AtaqueAcertou`/`AtaqueErrou`, F4). Funções que recebem `FBattleState`/`FBattleEvent`/`FPetState`/`FPetPresentationInfo` diretamente (não são `BlueprintType`) ficaram fora do Blueprint, mesmo padrão já usado em `BuildCommit()`. Próximo: Fase 5 (T11–T12, classes base C++ de widgets — sem UMG) e Fase 6 (T13 sonda anti-recálculo, T14 roteiro de verificação visual).

---

## Decisões Recentes

### AD-001: Unreal Engine 5 como motor, substituindo Cocos Creator (2026-08-24)

**Decisão:** migrar de Cocos Creator 3.8.6 para Unreal Engine 5.
**Razão:** o requisito de mundo aberto **contínuo, sem loading**, é o mais duro do projeto e é o único que um motor resolve ou não resolve por você. A Unreal entrega World Partition, HLOD e streaming de fábrica; o Cocos não tem equivalente e foi desenhado para mobile e casual, 2D primeiro.
**Alternativa considerada:** Unity. Perde por exigir que se construa a tecnologia de streaming (Addressables + solução própria), o que é inviável para equipe pequena. Ganharia em mobile e em proximidade de linguagem (C# vs TypeScript).
**Trade-off:** perde-se TypeScript, builds instantâneas, exportação web e a possibilidade de compartilhar a lógica de resolução entre cliente e servidor Node. Aceita-se 5% de royalty acima de US$ 1M.
**Impacto:** o protótipo Cocos é descartado. A escolha foi feita pelo mundo aberto, não pela batalha — a batalha é lógica instanciada e barata de escrever em qualquer motor.

### AD-002: A batalha é construída uma vez só, já no motor final (2026-08-24)

**Decisão:** não terminar o combate no Cocos para migrar depois.
**Razão:** o motor é ditado pelo mundo aberto. Construir o sistema de batalha sabendo que ele vai migrar é trabalho jogado fora.
**Impacto:** o que se leva do projeto anterior é o **design** — modelo de 3 ações, 12 tipos de ação, fluxo de resolução simultânea, atributos do pet. Nenhum arquivo de código.

### AD-003: Direção de arte Link's Awakening (2019), com ressalva (2026-08-24)

**Decisão:** low-poly, proporções de brinquedo, material fosco, paleta saturada. Nada de realismo ou PBR complexo.
**Razão:** barata de produzir sem estúdio de arte, leve em mobile, envelhece bem, e aceita 250 criaturas variadas sem cair no vale da estranheza.
**Ressalva registrada:** o visual original depende de *tilt-shift* e câmera fixa em diorama, o que existe porque o mundo dele é pequeno e emoldurado. Mundo aberto contínuo quer câmera livre e horizonte visível. **O que se adota é paleta, proporção e material; o efeito de miniatura vira decisão por contexto** — forte na batalha instanciada, sutil ou ausente no mundo.

### AD-004: Simulação determinística com aritmética inteira e RNG semeado (2026-08-24)

**Decisão:** a resolução de combate usa apenas inteiros (ou ponto fixo) e um gerador pseudoaleatório semeado e explícito. Zero ponto flutuante, zero `rand()` global, zero dependência de ordem de iteração de contêiner.
**Razão:** é o que permite servidor autoritativo barato, replay, espectador e testes de regressão por snapshot. Sem determinismo, cada um desses vira um projeto próprio.
**Trade-off:** disciplina permanente. Um `float` no cálculo de dano quebra tudo silenciosamente.
**Impacto:** desempate nunca pode vir da ordem de um array — vem sempre de uma chave estável (velocidade, depois id do pet).

### AD-005: Servidor autoritativo com commit/reveal e trace de eventos (2026-08-24)

**Decisão:** o cliente envia apenas o commit das três ações. O servidor simula e devolve um **trace de eventos** ordenado. O cliente anima o trace; ele não decide nada.
**Razão:** o payload é minúsculo, não há necessidade de rollback ou predição (é por turnos), e é resistente a trapaça por construção. O mesmo trace serve para replay, espectador e reconexão de graça.
**Impacto:** a apresentação nunca calcula resultado. Se a animação precisa de um número, ele veio no trace.

### AD-006: Web fora de escopo, via Pixel Streaming se necessário (2026-08-24)

**Decisão:** sem build para navegador. Se web voltar, é Pixel Streaming.
**Razão:** mundo aberto contínuo não roda em navegador em motor nenhum — teto de memória e streaming de assets pela rede. Pixel Streaming custa uma GPU na nuvem por jogador simultâneo, o que serve para demo, não para distribuição.

### AD-007: GAS não é adotado no combate da v1 (2026-08-24)

**Decisão:** o sistema de batalha por turnos **não** usa o Gameplay Ability System. Skills e efeitos são dados próprios, resolvidos pela simulação determinística.
**Razão:** GAS traz o próprio modelo de predição de cliente e replicação em tempo real, que colide com AD-005 — servidor simula, cliente só anima o trace. Num combate por turnos não se quer predição; quer-se que o cliente não decida nada. Usar GAS aqui seria pagar a complexidade dele para desligar metade.
**Trade-off:** abre-se mão do ferramental pronto de efeitos, cooldowns e stacking, que teria de ser escrito.
**Revisitar em:** M3 (escala de skills) e M5 (mundo aberto) — no mundo aberto, com efeitos de status em tempo real, GAS provavelmente passa a valer.

### ~~AD-008~~ SUPERADA por AD-014 (2026-08-25): Camada de dados — DataTable com CSV, DataAsset e Gameplay Tags

**Decisão:** os 250 pets vivem em `UDataTable` alimentada por **CSV versionado**; skills em `UDataAsset`; tipos, estados e classificação em `FGameplayTag`, nunca em `enum`.
**Razão:** DataTable é o certo para conjunto grande e homogêneo e permite editar balanceamento em planilha; DataAsset ganha quando os itens são poucos, variados e usam herança. Gameplay Tags são hierárquicas e criáveis por designer, e são a espinha dorsal do UE moderno.
**Trade-off registrado:** `UDataTable` é **binário e não diffável** em pull request. Por isso o CSV é a fonte da verdade e o `.uasset` é derivado — sem essa inversão, revisar o balanceamento de 250 pets seria impossível.
**CORRIGIDO em 2026-08-25 — Gameplay Tags NÃO entram no núcleo de simulação.** O caveat original dizia que depender do módulo `GameplayTags` era aceitável. Estava errado, e foi provado por experimento: `GameplayTags.Build.cs` declara **`Engine` em `PublicDependencyModuleNames`**, então qualquer módulo que dependa dele herda a engine inteira — includes e link. Ver AD-012.

**Regra corrigida:** `FGameplayTag` vive na camada `BattleSquare` (dados, apresentação, interface com designer). O núcleo `BattleSim` representa tipo, direção e classificação como **inteiros/enums próprios**, e a tradução tag ↔ inteiro acontece na fronteira. Isso reforça AD-004 em vez de ameaçá-lo: sem tags no núcleo, a ordem de registro deixa de ser um risco de determinismo.

### AD-009: Ação de combate é o par (Tipo, Direção) (2026-08-25)

**Decisão:** uma ação enfileirada é `(Tipo, Direção)`, não uma entrada de lista plana. Tipos: `MOVER`, `ATACAR`, `MAGIA`, `DEFENDER`, `ESQUIVAR`, `AGUARDAR`. Direções: as 8. `DEFENDER` e `AGUARDAR` ignoram a direção.
**Razão:** numa grade 3x3, ataque sem direção atinge o tabuleiro inteiro a partir do centro — a posição vira decorativa, que era o defeito silencioso do protótipo antigo. Com direção, errar o tipo e errar a direção são erros distintos, o que dobra a profundidade do commit às cegas.
**Alternativa descartada:** 12 ações planas (o modelo do protótipo Cocos). Mais simples de implementar e de exibir, mas esvazia a grade.
**Trade-off:** a UI de seleção fica mais cara — precisa compor tipo e direção sem virar doze botões, e precisa caber em toque de celular.
**Impacto:** fecha o contrato de ação e destrava a fase de Design. Resolve DP-01 na spec do Combate Núcleo.

### AD-010: Unreal Engine 5.8, e a razão é o MCP (2026-08-25)

**Decisão:** o projeto fixa **UE 5.8** (`EngineAssociation: "5.8"`), não a 5.6 que também está instalada.
**Razão:** verificado nas duas instalações — a 5.6 **não traz** `ModelContextProtocol` nem `ToolsetRegistry`. A 5.8 traz os dois, mais 25 toolsets (`GASToolsets`, `PCGToolset`, `GameplayTagsToolset`, `AutomationTestToolset`, entre outros). Sem 5.8 não existe controle do editor por MCP.
**Trade-off:** esses plugins vivem em `Engine/Plugins/Experimental/`. Experimental significa API sujeita a mudar entre versões — o custo é retrabalho pontual quando a 5.9 sair.
**Risco de conhecimento:** as skills instaladas documentam até a **5.7**. Para detalhe de API da 5.8, consultar `context7` ou a fonte da engine, nunca responder de memória.

### AD-011: Estrutura de módulos com o núcleo isolado por construção (2026-08-25)

**Decisão:** dois módulos runtime. `BattleSim` (`PreDefault`) depende apenas de `Core`, `CoreUObject` e `GameplayTags` — **sem `Engine`**. `BattleSquare` (`Default`) depende de `Engine`, `EnhancedInput` e de `BattleSim`.
**Razão:** transforma a fronteira do PROJECT.md em regra de build, não em disciplina.

**Correção de 2026-08-25:** a formulação original desta decisão listava `GameplayTags` entre as dependências do `BattleSim` e afirmava que a isolação era garantida. **Não era.** Ver AD-012 para o experimento que derrubou a afirmação. As dependências corretas do `BattleSim` são **apenas `Core` e `CoreUObject`**, e só com elas a violação vira erro de compilação.
**Impacto:** o alvo `BattleSquareServer` existe desde já, para que nada compile só porque há cliente presente.
**Nota:** `bAutoStartServer=True` em `Config/DefaultModelContextProtocol.ini` — o servidor MCP sobe com o editor, sem digitar `StartServer` a cada sessão.

### AD-012: A isolação do núcleo é verificada por experimento, não por afirmação (2026-08-25)

**Contexto:** ao criar o projeto, afirmei que omitir `Engine` das dependências do `BattleSim` tornava **impossível** referenciar `AActor` ou `UWorld` no núcleo. Testei em vez de confiar, e a afirmação caiu.

**Experimento (sonda negativa):** plantar um `.cpp` no `BattleSim` que referencia `AActor::StaticClass()` e verificar se o build falha.

| # | Configuração do `BattleSim` | Esperado | Real |
|---|---|---|---|
| 1 | `Core`, `CoreUObject`, `GameplayTags` — ponteiro nulo apenas | falha | ✅ compilou (sonda fraca: ponteiro nulo não exige símbolo) |
| 2 | idem, com referência real de símbolo | falha | ✅ compilou e **linkou** |
| 3 | idem + `NoSharedPCHs` + `bUseUnity = false` | falha | ✅ compilou — logo, **não era vazamento de PCH** |
| 4 | **`Core`, `CoreUObject` apenas** | falha | ❌ `fatal error: 'GameFramework/Actor.h' file not found` |

**Causa raiz:** `Engine/Source/Runtime/GameplayTags/GameplayTags.Build.cs` declara `"Engine"` em `PublicDependencyModuleNames`. Dependência pública é transitiva — quem depende de `GameplayTags` recebe a engine inteira de graça.

**Decisão:** `BattleSim` depende **exclusivamente** de `Core` e `CoreUObject`. Toda dependência nova nesse módulo exige rodar a sonda antes de entrar, porque qualquer módulo com `Engine` público refura a fronteira do mesmo jeito e sem aviso.

**Por que isto virou decisão registrada:** a fronteira do núcleo é a premissa de que dependem AD-004 (determinismo) e AD-005 (servidor autoritativo). Se ela é falsa e ninguém percebe, as duas ruem juntas — e ruiriam em silêncio, que é exatamente como o protótipo Cocos morreu (ver L-001).

### L-006: UHT exige nomes globalmente únicos entre TODOS os plugins carregados

**Contexto:** T1 declarou `UENUM() enum class EDirection`. Build falhou: `EDirection` já existe em `RigLogicModule` (plugin de animação da engine, sem relação nenhuma com o projeto).
**Problema:** tipos com `UENUM`/`USTRUCT`/`UCLASS` entram num namespace de reflexão único para o processo inteiro — não por módulo, não por namespace C++. Um nome genérico colide com qualquer plugin habilitado, mesmo um que o projeto nunca importa diretamente.
**Solução:** renomeado para `EBattleDirection`. Regra adotada: todo tipo refletido no domínio do jogo leva prefixo `Battle`.
**Previne:** nomear tipo refletido como se harness fosse C++ puro (namespace, escopo de arquivo) é o erro. Antes de nomear um `UENUM`/`USTRUCT`/`UCLASS` novo, assumir que o nome precisa ser único no projeto inteiro mais toda a engine mais todo plugin habilitado — na prática, sempre prefixar.

### AD-013: Desempate por velocidade não implementado em v1 — infraestrutura adiada honestamente (2026-08-25)

**Decisão:** T9 (`FBattleResolver::ResolveTurn`) não implementa nenhum desempate por velocidade/PetId, apesar de "Pronto quando" (tasks.md) listar isso.
**Razão:** analisado antes de codificar — as quatro fases (T5–T8) foram desenhadas para que Left e Right resolvam simetricamente dentro de cada fase (postura não compete, movimento coleta tudo antes de aplicar, combate acumula em vez de aplicar sequencialmente). Não existe, em v1 (1 pet por lado), nenhum ponto de decisão onde ordem Left-antes-de-Right mudaria o resultado. Implementar desempate agora seria código morto — mesmo problema já registrado em B-003 para BTL-05.
**Impacto:** desempate por velocidade é infraestrutura de M3 (N pets por lado), onde múltiplos pets do MESMO lado poderiam competir por precedência de fato. Fica documentado no comentário de `BattleResolver.cpp`, não escondido.

### AD-014: Dados de pet vêm de um backend externo, consultado ao carregar a batalha (2026-08-25)

**Decisão:** os 250 pets deixam de viver em `UDataTable`/CSV dentro do projeto Unreal (supera AD-008) e passam a viver num **backend próprio, separado do servidor de combate** — fonte da verdade em PostgreSQL, administrável por API.

**Arquitetura de leitura — réplica local sincronizada, não busca remota por batalha (refinado em 2026-08-25):**
O caminho de leitura NÃO é o servidor de combate chamando o backend remoto a cada partida. É um **espelho local sincronizado** (cache/réplica no processo do servidor de combate — SQLite embarcado ou estrutura em memória, atualizado por polling periódico ou push do backend) que fica sempre razoavelmente atualizado. A montagem da batalha lê **do espelho local**, nunca da rede diretamente. Isso resolve duas coisas ao mesmo tempo:
- **Performance:** nenhuma latência de rede no caminho de montar uma partida, mesmo com muitas batalhas simultâneas.
- **Resiliência:** se o backend cair, o espelho local segue servindo dados (podem ficar levemente desatualizados, mas o jogo não para).

**Fronteira de determinismo preservada:** a sincronização acontece **fora** do caminho de resolução de turno. Uma vez que a batalha monta `FBattleState` a partir do espelho local, os valores (inteiros, formato `FPetState`) ficam congelados — `FBattleResolver::ResolveTurn` nunca toca rede nem disco, sob nenhuma circunstância (AD-004, AD-011, AD-012 continuam intactos).

**Razão:** pets administráveis por banco de dados, desacoplado do servidor de combate (dois sistemas, decisão do usuário), sem pagar latência de rede nem criar ponto único de falha no caminho crítico.
**Trade-off:** dado no espelho local pode estar levemente atrasado em relação ao backend (janela de sincronização) — aceitável para balanceamento de pets, que não muda a cada segundo. Complexidade adicional: precisa de estratégia de sync (intervalo, invalidação, versionamento de schema entre backend e espelho).
**Impacto:** cria uma feature nova, fora do núcleo de combate — ver `.specs/features/backend-dados-pet/`. Não muda nada dentro de `BattleSim` (a fronteira do núcleo continua intacta); só muda quem preenche `FPetState` antes da batalha começar, e adiciona um componente de sincronização entre o backend e o servidor de combate.

### AD-015: Stack do backend de dados segue os padrões já documentados do usuário (2026-08-25)

**Decisão:** Bun + `Bun.serve`, TypeScript ESM, PostgreSQL via Drizzle ORM, validação Zod na fronteira, resposta em envelope (`{data}`/`{error}`), rotas versionadas (`/v1/...`), autenticação por token de curta duração.
**Razão:** não é uma escolha nova — é aplicar literalmente `nodejs.md`, `apis.md` e `database.md` (padrões globais do usuário) a este projeto. Nenhuma decisão de stack original aqui; só conformidade.
**Impacto:** qualquer desvio desses padrões neste backend deve ser justificado explicitamente, não presumido por conveniência.

### AD-016: Espelho local — criptografia em repouso + verificação de integridade por assinatura (2026-08-25)

**Decisão:** o espelho local (SQLite embarcado no servidor de combate) recebe duas camadas de proteção com propósitos distintos: criptografia em repouso (SQLCipher) e verificação de integridade por assinatura/HMAC por registro, emitida pelo backend na sincronização.
**Razão:** o usuário pediu criptografia para reduzir risco de "hacks alterarem o cache". Esclarecido antes de registrar: como o espelho só existe no processo do servidor autoritativo (nunca no cliente), o servidor precisa da chave para ler o próprio dado em runtime — logo, criptografia sozinha NÃO impede adulteração por quem já tem acesso equivalente ao processo do servidor. O que detecta e rejeita adulteração é a assinatura: se o arquivo for editado fora do fluxo de sincronização, a assinatura não bate e o registro é descartado.
**Trade-off:** nenhum — as duas camadas são baratas e não competem entre si. Cada uma cobre um cenário de ameaça diferente (disco roubado vs. adulteração em tempo de execução).
**Impacto:** PETDB-10 e PETDB-11 na spec do backend de dados de pet.

### AD-017: Adulteração de cache local não muda resultado — é sinal, não vetor de trapaça (2026-08-25)

**Decisão:** detectar cache de pet adulterado no cliente encerra a sessão daquele cliente e declara o oponente vencedor por W.O. Banimento persistente entre partidas fica fora de escopo até existir conta de jogador.
**Razão:** AD-005 (servidor autoritativo) já garante que o cliente nunca decide resultado — adulterar o cache local não consegue mudar quem vence, só sinaliza intenção hostil. A resposta correta é consequência de sessão, não "corrigir" um resultado que a arquitetura já protege.
**Por que banimento persistente fica de fora:** exige identidade de jogador entre sessões, que o projeto não tem — v1 usa código de sala (PROJECT.md, Out of Scope: matchmaking real). Prometer bloqueio permanente sem essa infraestrutura seria documentar algo que não pode ser implementado hoje.
**Impacto:** PETDB-12 na spec do backend de dados de pet. A trilha de auditoria do Nível 1 (evidência de adulteração) é desenhada para ser reaproveitada por um sistema de moderação futuro, sem precisar ser reconstruída quando conta de jogador existir.

### AD-019 (revisada em 2026-08-26): o SQLite vem da engine, não vendorizado

**Decisão original (2025-08-25):** módulo barreira `SQLiteLibrary` compilando a própria cópia de `sqlite3.c`, com regras de warning próprias.
**O que mudou:** a cópia própria colidia com o `SQLiteCore` da engine no build empacotado (270 símbolos duplicados — ver **L-027**). O projeto passou a depender do plugin `SQLiteCore`, que expõe a API C de propósito, e o módulo barreira foi removido por ter ficado sem conteúdo.
**O que se manteve:** a intenção original — `BattleSquare` continua consumindo `#include "sqlite3.h"` sem saber de onde vem — e o princípio que o próprio AD-019 já enunciava para o OpenSSL: consumir o que a engine já tem.

### AD-018: `bun:sqlite` não tem SQLCipher — criptografia por arquivo (AES-256-GCM) com temporário em RAM (2026-08-25)

**Descoberto:** ao implementar T12, testei `PRAGMA key` no `bun:sqlite` diretamente — foi aceito sem erro e o dado ficou em **texto plano** no arquivo. `bun:sqlite` não tem a extensão SQLCipher compilada; teria sido uma falsa sensação de segurança se eu não tivesse testado (mesmo padrão de L-004/L-007: verificar em vez de afirmar).
**Decisão:** o espelho local usa `AES-256-GCM` (autenticado) sobre o **arquivo inteiro** via `node:crypto` — decifra para um temporário, opera com `bun:sqlite`, cifra de volta. O temporário vive em **tmpfs (RAM)**, nunca em disco persistente, removido mesmo em caminho de erro.
**Trade-off explicitamente aceito:** existe uma janela onde texto plano existe na RAM do processo (não no disco). Só é explorável por quem já tem acesso de leitura à memória do processo — nível de privilégio que já compromete `MIRROR_ENCRYPTION_KEY`, `SYNC_API_TOKEN` e o processo inteiro por outras vias. SQLCipher nativo eliminaria essa janela por completo, mas o ganho marginal não justificou a dependência C nativa extra (avaliado e descartado com o usuário).
**Nomenclatura corrigida:** a env var `SQLCIPHER_KEY` virou `MIRROR_ENCRYPTION_KEY` — o nome antigo prometia um mecanismo que não é o que está implementado.

### AD-019: Fase 5 usa o OpenSSL já empacotado na engine, não libsodium (2026-08-25)

**Descoberto:** UE 5.8 traz `OpenSSL 1.1.1t` pronto em `Engine/Source/ThirdParty/OpenSSL`, com binários estáticos para Mac (`libcrypto.a`, `libssl.a`) — confirmado no `OpenSSL.Build.cs`, não presumido.
**Decisão:** T15/T16 deixam de vendorizar `libsodium`. Em vez disso: consumir o módulo `OpenSSL` da própria engine via `AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL")` (padrão preferido da skill `unreal-thirdparty`) para as duas necessidades de criptografia — decifrar o espelho local (`AES-256-GCM`, EVP API) e verificar assinatura (`Ed25519`, `EVP_PKEY`). OpenSSL 1.1.1 suporta as duas nativamente.
**SQLite:** continua sendo vendorizado, mas agora **puro** (sem extensão de criptografia) — a decisão AD-018 já havia movido a criptografia para fora do SQLite (arquivo inteiro cifrado por AES-256-GCM). É a amalgamation de domínio público (`sqlite3.c`/`sqlite3.h`), trivial de compilar como fonte dentro do módulo, sem a complexidade de `ModuleType.External` de biblioteca binária.
**Impacto:** zero binário terceiro novo para compilar/vendorizar — só consumo do que a engine já tem, mais uma fonte de domínio público de arquivo único. Reduz superfície de risco da Fase 5 significativamente frente ao plano original (SQLCipher + libsodium).

### AD-020: Server Target não compila nesta distribuição da engine — descoberto em T11 (Combate Online), registrado como blocker de infra (2026-08-26)

**Descoberto:** ao tentar T11 (spike de PIE multiplayer real, `.specs/features/combate-online/tasks.md`), rodei `Build.sh BattleSquareServer` pela primeira vez desde que `Source/BattleSquareServer.Target.cs` foi criado (na Fase de design do Combate Online). Erro: `Server targets are not currently supported from this engine distribution.`
**Causa:** a instalação da UE 5.8 deste projeto veio do Epic Games Launcher (binário pré-compilado). Builds de servidor dedicado (`TargetType.Server`) só são suportadas em engines **compiladas a partir do código-fonte** (via GitHub, com acesso vinculado à conta Epic). É uma restrição de distribuição, não do projeto nem do código — `BattleSquareServer.Target.cs` está correto, só não pode ser compilado com a engine instalada hoje.
**Impacto imediato:** T11 não pôde provar comportamento de rede real com um dedicated server de verdade — nem sequer compila. `ListenServer` (que não tem essa restrição, roda como o mesmo `TargetType.Game`) continua disponível para investigação futura, mas DP-online-01 já registrou que `ListenServer` nunca é modo competitivo (o host lê o commit do oponente).
**Não resolvido nesta sessão — registrado como blocker (B-004) de infraestrutura, não de código.** A lógica de servidor (autoridade via `HasAuthority()`, `UBattleTurnCoordinator`, etc.) está implementada e testada headless; o que falta é o **ambiente** para provar que ela roda como um processo `Server` de verdade.

---

## Blockers Ativos

### B-006 / B-006b / B-007: nenhuma plataforma móvel é compilável nesta máquina

**Descoberto:** 2026-08-26, ao abrir M6.
**Impacto:** **M6 (Plataformas) não é validável aqui.** Não é questão de esforço nem de teste — o toolchain não existe. Todo o roteiro de `docs/verification/mobile.md` está marcado BLOQUEADO, e o orçamento de mobile (`docs/performance/orcamento-mobile.md`) permanece com todos os números marcados **alvo**, nenhum medido.

| Id | O quê | Evidência exata | Remédio, e de quem é |
|---|---|---|---|
| **B-006** | Componente **IOS** da engine não instalado | `Build.sh BattleSquare IOS Development` → `Missing files required to build IOS targets. Enable IOS as an optional download component in the Epic Games Launcher.` | **Usuário:** Epic Games Launcher → instalar o componente IOS do UE 5.8 |
| **B-006b** | Sem identidade de assinatura, sem perfil de provisionamento | `security find-identity -v -p codesigning` → `0 valid identities found`; `~/Library/MobileDevice/Provisioning Profiles/` vazio | **Usuário:** conta Apple Developer + certificado + perfil |
| **B-007** | SDK/NDK Android ausente | Validação de plataforma da engine → `Android INVALID r27c` | **Usuário:** `SetupAndroid` da engine, ou Android Studio + NDK r27c |

**Nota:** a validação de plataforma da engine reporta `IOS VALID 26.1.1`, e isso **engana** — ela reporta o SDK do Xcode, não o componente iOS da própria Unreal. A prova real é tentar o build, e ele falha em menos de um segundo.
**Basta resolver:** B-006 + B-006b (caminho iOS) **ou** B-007 (caminho Android). Não são necessários os dois.
**Parentesco com B-004:** mesma categoria — a engine instalada pelo Launcher não traz tudo que o roadmap pede, e a descoberta só acontece quando se tenta.

### B-009: botões de skill no WBP — ✅ RESOLVIDO em 2026-08-29

O plugin voltou a estar disponível numa sessão seguinte, e os três botões foram criados: `Button_Camuflar`, `Button_Voar` e `Button_Submergir`, na mesma linha dos outros tipos, com rótulo e marcados como variável — que é o que faz o `BindWidgetOptional` do C++ encontrá-los.

**O obstáculo real não era o plugin, era o NOME.** `AddWidget` recusa criar um widget cujo nome já existe, e o `BindWidgetOptional` do C++ já reservava os três nomes na classe. A saída foi criar com nome temporário e renomear — o `RenameWidget` aceita o nome reservado justamente porque ele pertence à variável que o widget vai preencher.

**Uma tentativa parcial deixou lixo**, e isso vale registrar: a primeira execução criou `Tmp_Camuflar` e falhou no rename por parâmetro errado, deixando dois órfãos e um `Button_Camuflar` sem rótulo. Refiz o script de forma **idempotente** — conferindo a árvore antes de cada passo — em vez de repetir e duplicar. Script que mexe em asset precisa poder rodar duas vezes.

**Registro original do bloqueio:**

### B-009 (histórico): botões de skill no WBP exigem o plugin de edição

**O quê:** `WBP_BattleActionSelector` não tem botões para Camuflar, Voar e Submergir. O C++ já os espera por `BindWidgetOptional` (`Button_Camuflar`, `Button_Voar`, `Button_Submergir`) — criá-los com esses nomes basta, e eles se ligam sozinhos.
**Por que está bloqueado:** editar o asset exige o plugin `unreal-mcp`, indisponível na sessão em que as skills foram criadas. Editar `.uasset` por fora não é opção.
**Contorno em vigor:** as três aparecem na barra em Slate (C++) e por C/V/B no teclado — o jogador não fica sem elas, mas o widget principal não as mostra.
**Custo de resolver:** minutos no editor, por uma pessoa com ele aberto.

### B-008: Console exige licença e devkit

**Descoberto:** 2026-08-26.
**Impacto:** a segunda feature de M6 (Console) **não é iniciável**, nem em spec de implementação. Acesso ao SDK de qualquer console exige contrato de desenvolvedor registrado com o fabricante e hardware de desenvolvimento — nada disso é obtenível por código, e o próprio `ROADMAP.md` já previa isso ("exige licença de desenvolvedor e devkit").
**Remédio, e de quem é:** **Usuário** — registro como desenvolvedor junto ao fabricante e aquisição de devkit. Até lá, a feature fica PLANNED e sem spec, de propósito: escrever spec de implementação para uma plataforma cujo SDK não se pode nem ler produziria ficção.

### B-001: Escopo de mundo aberto contínuo

**Descoberto:** 2026-08-24
**Impacto:** mundo aberto contínuo é a categoria mais cara de jogo que existe, e é um salto de 10x a 100x sobre um battler de tabuleiro. O risco do projeto deixou de ser técnico e passou a ser de escopo: entregar nenhum dos dois.
**Mitigação:** M5 é bloqueado até M1–M3 estarem concluídos e o combate validado como divertido. Reavaliar em M4 se o mundo pode ser fatiado em zonas conectadas — isso derrubaria muito o custo.
**Resolução:** decisão consciente de go/no-go no fim de M3, com o combate na mão.

### ~~B-002~~ ✅ FALSO ALARME, resolvido em 2026-08-25

**O que eu pensava:** execução headless de `Automation RunTests` estava travando/falhando silenciosamente, sem log, sem crash.
**O que era de verdade:** os testes **sempre rodaram e sempre passaram** — em pelo menos 7 execuções distintas, `BattleSim.Random.RangeIsUnbiasedAndInclusive` e `BattleSim.Random.SameSeedProducesSameSequence` deram `Success`, sempre `TEST COMPLETE. EXIT CODE: 0`.
**A causa do falso alarme:** no macOS a Unreal grava o log em `~/Library/Logs/Unreal Engine/<NomeDoProjeto>Editor/`, **não** em `Saved/Logs/` dentro do projeto (que é o padrão no Windows/Linux, e o que eu presumi sem verificar). Toda vez que eu chequei `Saved/Logs` vazio, concluí "não rodou" — quando na verdade tinha rodado e passado, só que o log estava em outro lugar.
**Custo do erro:** um blocker registrado sem necessidade, pedidos repetidos ao usuário para rodar o mesmo comando, e a linha `*** Recognized (0x6d) family*** (24 cols X 18 rows)` que pareceu suspeita era só detecção de terminal/HID, irrelevante.
**Correção adotada:** comando de verificação padrão passa a checar **os dois locais possíveis**, priorizando o de macOS:
```bash
find "$HOME/Library/Logs/Unreal Engine/BattleSquareEditor" Saved/Logs -newer <marcador> -name "*.log" 2>/dev/null \
  -exec grep -l "Automation RunTests" {} \;
```


### B-003: BTL-05 (colisão entre aliados) sem cobertura de execução em v1

**Descoberto:** 2026-08-25, durante T6 (fase de movimento).
**Causa:** `ApplyMovement` recebe uma ação por LADO, não por PET — reflexo direto de `FTurnCommit` ser por lado (design.md, contrato de rede de v1). Com uma única direção por lado, dois aliados partindo de posições diferentes **nunca** convergem para o mesmo destino — é matematicamente inatingível, não uma lacuna de teste esquecida.
**O que existe:** o código de detecção de colisão (`DestinationClaimsBySide` em `BattlePhaseMovement.cpp`) está implementado e correto, mas fica morto/inalcançável pela interface pública atual.
**O que eu quase fiz:** escrever um teste que passava sem exercitar esse caminho, e chamá-lo de cobertura de BTL-05. Corrigido antes de rodar — o teste agora documenta a lacuna e emite `AddWarning`, em vez de fingir prova.
**Resolução:** adiada para M3, junto da expansão do contrato de rede para commit por pet (não por lado). Não bloqueia v1: o requisito é estrutural para N pets, e v1 é 1 pet por lado por definição da spec.

### B-004: Server Target não compila com a engine instalada (Launcher, não fonte)

**Descoberto:** 2026-08-26, T11 (Combate Online) — ver AD-020 para a investigação completa.
**Impacto:** nenhum teste real de dedicated server é possível nesta máquina até a engine ser trocada por uma build compilada da fonte (GitHub da Epic, requer vínculo de conta). `ListenServer` continua disponível, mas não é o modo de produção pretendido (DP-online-01).
**Mitigação:** nenhuma ação de código necessária — `UBattleTurnCoordinator`/`UBattleNetCommitComponent` já são escritos e testados de forma agnóstica a qual `NetMode` os executa (via `HasAuthority()`), então não há dívida técnica represada. O blocker é puramente de ambiente de teste/deploy.
**Resolução:** decisão de infraestrutura fora do escopo desta sessão — trocar a engine para build de fonte é uma decisão que envolve tempo de build (horas) e vínculo de conta GitHub↔Epic, então fica registrada para quando M2 for para produção de verdade, não bloqueando o trabalho de código já feito.

### B-005: Coleção local (`USaveGame`) não distingue jogadores numa partida em rede — correta só para Standalone/host

**Descoberto:** 2026-08-26, ao iniciar Níveis, Experiência e Evolução (M4), revisando a fiação de Coleção e Captura.
**Impacto:** `ABattleArena::CheckForCapture` (Coleção e Captura) e o gancho equivalente de XP desta feature escrevem no `USaveGame` do **processo** onde `ABattleArena` roda — que é o processo do SERVIDOR em Sala e Pareamento Simples (`ABattleSquareGameMode::AssembleMatchForRoom` spawna a `ABattleArena` autoritativa ali). Isso está certo para Standalone (um único jogador local, `Arena` roda no processo dele) e para o HOST de um `ListenServer` (ele também está localmente naquele processo) — mas para os DOIS jogadores remotos de uma partida em Sala e Pareamento via `DedicatedServer`/`ListenServer` com um segundo jogador remoto, a captura/XP cairia no save LOCAL DO SERVIDOR, não no de nenhum dos dois jogadores de verdade. Sem conta de jogador (M7), o servidor não tem como saber "de quem" é a coleção que deveria atualizar.
**Mitigação:** nenhuma mudança de código agora — Coleção e Captura e Níveis/Experiência/Evolução continuam corretas para o caso de uso atual e principal (Standalone, jogador único local, que é como o jogo é jogado hoje). O comportamento incorreto só se manifestaria com dois jogadores remotos de verdade numa sala — cenário que já esbarra em B-004 (verificação de `DedicatedServer` real bloqueada) antes mesmo de chegar aqui.
**Resolução:** registrado como limitação conhecida, não corrigido agora — resolver de verdade precisa de identidade de jogador persistente (M7, Contas de Jogador) para o servidor saber de quem é cada coleção. Até lá, progressão de pet é uma feature de single-player/host, documentada como tal.

---

## Lições Aprendidas

### L-001: O protótipo Cocos quebrou por contratos internos incompatíveis, não por falta de código

**Contexto:** ~960 linhas escritas, nenhuma executável.
**Problema:** o `EventBus` expunha `on({event, callback})` enquanto todos os chamadores usavam `on("EVENTO", fn)`; o container de DI expunha `register/resolve` enquanto o `GameManager` chamava `bind().to()`. Nenhum evento era registrado e o boot estourava.
**Solução:** nenhuma — o código foi descartado junto com o motor.
**Previne:** contratos de infraestrutura (barramento de eventos, injeção, formato do trace) precisam de teste antes de qualquer consumidor. São exatamente os que falham em silêncio.

### L-002: O README divergiu do código sem ninguém notar

**Contexto:** o README descrevia o fluxo de resolução verificando colisão de posição entre cada ação; o código aplicava ação por ação, sem essa verificação.
**Problema:** a divergência é o sintoma de decidir enquanto se digita.
**Previne:** a spec é a fonte da verdade da regra. Código que diverge da spec é bug, e spec que diverge da intenção se corrige na spec, não no commit.

---

### L-003: A spec foi escrita antes das skills de Unreal existirem no ambiente

**Contexto:** `PROJECT.md`, `ROADMAP.md`, `STATE.md` e a spec do combate foram produzidos com a `tlc-spec-driven` apenas. As cinco skills de Unreal e o plugin oficial da Epic foram instalados depois.
**Consequência:** a spec do combate se manteve válida por ser neutra de motor por construção, mas as decisões de **stack** no `PROJECT.md` estavam vagas e foram revisadas em 2026-08-24 (ver AD-008).
**Previne:** quando entrar no Design, consultar `unreal-best-practices` **antes** de decidir sistema, não depois. A spec neutra de motor é o que tornou essa correção barata — se ela tivesse nascido cheia de API, o retrabalho teria sido outro.

---

### L-004: Afirmação de arquitetura sem teste negativo é opinião

**Contexto:** afirmei ao usuário que a isolação do `BattleSim` era garantida pelo sistema de build.
**Problema:** era falso, por dependência transitiva que eu não tinha verificado. Se eu não tivesse testado, a premissa falsa entraria no Design e só apareceria muito depois.
**Solução:** quatro sondas negativas até achar a causa e a configuração que realmente aplica a regra.
**Previne:** toda regra arquitetural que se pretende "garantida pelo build" precisa de um **teste negativo que falhe de propósito**. Sem ele, a garantia é narrativa. Candidato a virar teste de CI: a sonda do AD-012, rodando com build esperado-falho.

---

### L-005: Configuração que parece aplicada e não está

**Contexto:** criei `Config/DefaultModelContextProtocol.ini` com `bAutoStartServer=True`. O arquivo nunca foi lido.
**Problema:** a classe declara `UCLASS(config=EditorPerProjectUserSettings)`, então o arquivo correto é `Config/DefaultEditorPerProjectUserSettings.ini`. Nome de seção e propriedades estavam certos — só o **arquivo** estava errado, e config errada na Unreal falha **em silêncio**: nenhum aviso, nenhum log, o default simplesmente prevalece.
**Solução:** ler o `UCLASS(...)` da classe de settings na fonte da engine antes de escrever qualquer `.ini`.
**Previne:** é o mesmo padrão da L-004 — algo que *parece* configurado e não está. Junto com o AD-012, forma a regra prática do projeto: **configuração e fronteira arquitetural só contam quando verificadas por observação, nunca por leitura do próprio código.**

### L-007: Local do log da Unreal no macOS não é onde eu presumi

**Contexto:** passei cinco tentativas concluindo que a execução headless de testes estava travando, baseado em `Saved/Logs/` vazio dentro do projeto.
**Problema:** no macOS o log vive em `~/Library/Logs/Unreal Engine/<Projeto>Editor/`. Eu nunca verifiquei esse caminho — assumi convenção de outra plataforma sem checar a documentação nem a fonte para o caso específico do Mac.
**Solução:** os testes sempre passaram; o "blocker" era ausência de observação no lugar certo.
**Previne:** é a mesma família de erro de L-004 e L-005 — algo que *parece* não ter acontecido e só não tinha sido observado no lugar certo. A regra prática do projeto ("configuração e fronteira só contam quando verificadas por observação") precisa de um corolário: **verificar no lugar certo é parte da verificação.** Checar `Saved/Logs` sem saber se é ali que o macOS escreve não é observação, é suposição disfarçada.

### L-008: Nomear função livre com nome genérico também colide (não só UENUM/USTRUCT)

**Contexto:** T3 declarou uma função `HashCombine(uint64, uint64)` num namespace anônimo. Build falhou: "call to 'HashCombine' is ambiguous".
**Problema:** a engine já tem `HashCombine` em `Templates/UnrealTemplate.h`, incluído transitivamente por `CoreMinimal.h`. Namespace anônimo não impede ambiguidade de overload quando a chamada é feita sem qualificação.
**Solução:** renomeado para `CombineBattleHash`.
**Previne:** a lição de L-006 (nomes refletidos precisam ser únicos globalmente) se estende a **qualquer símbolo livre**, mesmo dentro de namespace anônimo, quando chamado sem qualificação — a engine tem utilitários de nome genérico (`HashCombine`, `Swap`, `Sort`...) espalhados por `CoreMinimal.h`. Prefixar `Battle` também em funções livres do domínio, não só em tipos `UENUM`/`USTRUCT`.

### L-009: Um teste que falha por má configuração de cenário ainda é um teste que funcionou

**Contexto:** T9 rodou 36 testes; 1 falhou (`DeadPetSkipsRemainingSlotActions`).
**Causa raiz:** o teste posicionava os pets a 2 casas de distância (herdado de `MakeDuelState()`, pensado para testes de movimento), mas ataque direcional tem alcance 1 — o ataque errava sempre, e o pet "quase morto" nunca morria.
**Não era bug de produção.** Era erro no cenário do teste. Corrigido ajustando a posição de um pet para ficar adjacente antes do ataque.
**Por que isto é uma lição boa, não uma lição de erro:** é exatamente o processo funcionando como desenhado — um teste real pegou uma inconsistência real antes de eu declarar a tarefa pronta, e a falha veio com mensagem específica (`Expected 'Right morreu no slot 0' to be false`), não um `exit 0` mudo. Contraste com L-007: aquele falso alarme não tinha teste nenhum rodando, só observação no lugar errado. Este é o comportamento correto do sistema de testes, e vale registrar como calibração: nem toda falha é motivo de alarme — é motivo de olhar a mensagem.

### L-010: Bun carrega `.env` automaticamente — teste de validação de ambiente precisa rodar fora do diretório do projeto

**Contexto:** T1 (backend de pets) verificaria que `environment.ts` falha alto e claro sem `ADMIN_API_TOKEN`. Rodei o teste passando env vars incompletas na linha de comando, de dentro do diretório do projeto — o servidor subiu normalmente, como se a validação não existisse.
**Causa:** Bun carrega `.env` da pasta atual automaticamente, por cima de qualquer env var passada na linha de comando que não sobrescreva a mesma chave. O `.env` já tinha `ADMIN_API_TOKEN` de uma rodada anterior — ele preencheu a lacuna que o teste tentava criar.
**Solução:** rodar o teste de "falta env var" de um diretório SEM `.env` (`/tmp/...`), apontando para o script por caminho absoluto. Confirmado: `ZodError` apontando exatamente o campo ausente.
**Previne:** todo teste que simula ambiente incompleto num projeto Bun precisa isolar do `.env` real do projeto — de outra forma, o teste "passa" por engano, exatamente como aconteceu aqui na primeira tentativa.

### L-011: Mensagem de erro não é código, e "too_small" não é sempre "ausente"

**Contexto:** T3 (validação de pet) — primeira versão fazia `message` duplicar o `code` (`"message": "PET_NAME_REQUIRED"`, ilegível para humano) e classificava QUALQUER `too_small` do Zod como campo ausente, incluindo `attack: -5` (valor presente, mas fora do mínimo) — gerava `PET_ATTACK_REQUIRED` para um valor negativo, o que é enganoso.
**Causa:** assumi a forma do erro do Zod em vez de inspecionar. O objeto real de issue expõe `origin` ("string" vs "number"), que distingue "string vazia" (campo ausente de fato) de "número abaixo do mínimo" (valor inválido) — as duas colapsam no mesmo `code: "too_small"`, mas são causas diferentes.
**Solução:** derivar o código do erro a partir de `issue.code` **e** `issue.origin`, nunca do texto da mensagem; mensagem fica livre para ser legível a humano.
**Previne:** mesma disciplina de T9 no combate núcleo (não confiar em nome de API sem checar a fonte) — aqui aplicada a bibliotecas TypeScript, não só à Unreal. `bun -e` com o schema real, três cenários, foi o que expôs os dois bugs antes de virarem código "pronto".

### L-012: Comparação `updatedAfter` exata no limite ainda inclui o registro — precisão de timestamp, não bug

**Contexto:** T6 (backend de pets) — filtro `updatedAfter` usa `gt` (estritamente maior). Testado com o cutoff igual ao `updatedAt` exato de um registro: o registro apareceu na lista mesmo assim. Com 1 segundo de diferença real, o filtro excluiu corretamente.
**Causa:** o Postgres guarda mais precisão em `timestamp with time zone` do que sobrevive ao round-trip por uma string ISO 8601 de milissegundos processada pelo `Date` do JS — o valor armazenado é ligeiramente "maior" que o que o cliente recebeu de volta.
**Por que não é bug:** o uso real (sync incremental) é idempotente — reenviar o mesmo registro verificado de novo não causa dano, só uma leitura redundante no pior caso. Corrigir isso exigiria precisão extra ou cursor sequencial, engenharia que a spec não pede.
**Previne:** ao testar filtro de "maior que" sobre timestamp, sempre testar o caso de fronteira exata separado do caso com folga real — são achados diferentes, e só o segundo prova que o operador funciona.

### L-013: `ANSI_TO_TCHAR` presume string terminada em `\0` — não use em buffer de bytes crus

**Contexto:** teste de interoperabilidade AES-256-GCM (T15/T16) — a decriptação em si passou (GCM autenticou corretamente), mas a comparação de string falhou com um caractere extra no fim (`...PLAINTEXTt` em vez de `...PLAINTEXT`).
**Causa:** `ANSI_TO_TCHAR` é uma macro que assume C-string terminada em `\0` e usa `strlen` internamente — aplicada a um `TArray<uint8>` de bytes decifrados (sem terminador), ela lê memória além do fim do buffer.
**Solução:** `FUTF8ToTCHAR` com comprimento explícito (`Converter(Data, Length)`), que não depende de terminador.
**Previne:** mesma disciplina de L-004 (verificar em vez de afirmar) — o resultado "quase certo, com um caractere a mais" é o tipo de falha que parece sutil mas aponta para uma suposição errada sobre o formato dos dados, não para lógica de negócio quebrada. A criptografia estava certa desde o início; o teste que a media é que estava errado.

### L-014: Automação da Unreal falha o teste automaticamente em qualquer `UE_LOG(Error)`, mesmo intencional

**Contexto:** T17 — dois testes de caminho negativo (espelho ausente, chave errada) fizeram exatamente o esperado (`bLoaded=false`, log de erro correto), mas o framework de automação reportou `Result={Fail}` mesmo assim.
**Causa:** a Unreal conta qualquer `UE_LOG(..., Error, ...)` emitido durante um teste como falha implícita, independente das asserções `TestTrue`/`TestFalse` passarem. Erro de log intencional precisa ser declarado com `AddExpectedError(padrão, MatchType::Contains, ocorrências)` antes do código que o emite.
**Também nesta rodada:** um teste comparava contra "Fluffy" com `attack: 10` — valor que eu mesmo tinha mudado para 15 num `PUT` de verificação manual, fases atrás. O carregador leu o dado real corretamente; o teste é que dependia de estado externo mutável. Corrigido criando um pet de fixture dedicado (`UnrealTestFixturePet`), nunca reusando dado de teste manual anterior.
**Previne:** ao testar caminho de erro EM Unreal, sempre declarar `AddExpectedError` antes de rodar; ao fixar dado esperado, nunca depender de um registro que outra parte do teste (ou sessão) pode ter mutado.

### L-015: `sqlite3.h` é um header de SISTEMA no macOS — sonda de isolação deu falso positivo

**Contexto:** T19 — sonda de isolação estendida testando se `BattleSim` conseguia referenciar SQLite/OpenSSL. A metade OpenSSL falhou corretamente; a metade SQLite **compilou**, parecendo indicar fronteira furada.
**Investigação (não aceitei o resultado sem checar a causa):** compilação manual com `clang -H` (rastreio de resolução de header) mostrou que `#include "sqlite3.h"` resolvia para `/usr/include/sqlite3.h` — **o SQLite do próprio SDK do macOS**, não o vendorizado do projeto. Apple embute SQLite como biblioteca de sistema há anos; qualquer programa C/C++ no Mac encontra esse header e linka essa lib, independente de qualquer dependência de módulo do projeto.
**Causa raiz do design da sonda:** testar "o header X existe" é um proxy falho para "o módulo Y depende de Z" quando X coincide com algo que o sistema operacional já fornece globalmente.
**Correção:** trocado o alvo da sonda de `sqlite3.h` (ambíguo) para os próprios headers do `BattleSquare` (`Data/PetDataLoader.h`) — nunca pode colidir com nada do sistema, e testa exatamente a direção de dependência que importa (núcleo nunca alcança a feature construída em cima dele).
**Previne:** ao desenhar uma sonda negativa contra biblioteca de terceiro, checar antes se o nome do header/símbolo é genérico o bastante para existir em bibliotecas de sistema do SO. Rastrear a resolução real (`clang -H` ou equivalente) antes de aceitar um resultado de sonda como prova — mesma disciplina de L-004/L-007, aplicada ao desenho do próprio teste de verificação, não só ao código sendo verificado.

### L-016: `FBattleRandom` nunca tinha sido exportado entre módulos — ninguém tinha chamado seus métodos de fora do `BattleSim` até agora

**Contexto:** T5 (IA burra, Apresentação do Combate) — primeira vez que `BattleSquare` chama `FBattleRandom::NextRange` diretamente. Link falhou: `Undefined symbols... FBattleRandom::NextRange`.
**Causa:** `struct FBattleRandom` nunca teve `BATTLESIM_API`. Isso nunca deu problema porque todo consumo anterior do `BattleSquare` tocava campos de `FBattleState`/`FPetState` (dados, sem chamar método), nunca um método de `FBattleRandom` em si — a lacuna de exportação estava latente, sem sintoma.
**Solução:** `struct BATTLESIM_API FBattleRandom` — só anotação de visibilidade entre módulos, não adiciona dependência nenhuma; a fronteira do núcleo (AD-011/AD-012) continua intacta.
**Previne:** ao expor uma struct nova do núcleo para consumo por outro módulo, checar se ela (e não só o módulo) tem `_API` — a ausência não aparece até o primeiro consumidor de fora tentar chamar um método dela, exatamente como aconteceu aqui, meses (nesta sessão, mensagens) depois da struct ter sido criada.

---

### L-017: helper de teste em `namespace { }` colide sob unity build — e o *adaptive unity* esconde isso até o conjunto de arquivos mudar

**Contexto:** ao plantar uma sonda de RPC (arquivo novo em `BattleSquare`) durante o Design do Combate Online, a build quebrou com `redefinition of 'MakeDuelState'` e `redefinition of 'HexStringToBytes'` — em arquivos de teste que **não foram tocados** e que vinham compilando havia várias fases.
**Causa:** `PetViewTest.cpp` e `BattleTracePlayerGroupingTest.cpp` definiam `MakeDuelState` cada um no seu `namespace { }` anônimo; `PetDataLoaderTest.cpp` e `PetDataLoaderTamperTest.cpp` faziam o mesmo com `HexStringToBytes`. Namespace anônimo é local à *translation unit*, não ao arquivo — e o unity build concatena vários `.cpp` numa TU só, o que faz os dois virarem o mesmo escopo. O defeito estava plantado desde que o segundo arquivo de cada par foi escrito; só não aparecia porque o **adaptive unity** da UBT vinha excluindo os arquivos recém-editados do blob de unity (`[Adaptive Build] Excluded from BattleSquare unity file: ...`), mantendo-os em TUs separadas por acidente.
**Solução:** nomes únicos por arquivo (`MakePetViewDuelState`, `TamperTestHexStringToBytes`). Build limpa e 30/30 testes confirmados depois.
**Previne:** helper de teste em namespace anônimo precisa de nome único no módulo inteiro, não só no arquivo. E, mais geral: **build verde sob adaptive unity não é prova de que a build limpa passa** — o conjunto de arquivos excluídos do unity muda a cada edição, então um mesmo código pode compilar hoje e quebrar amanhã sem ninguém ter mexido nele. Uma build limpa periódica (ou de CI) é o que pega isso.

---

### L-018: `BeginPlay` não é confiável num `UWorld` de teste montado à mão — SIGSEGV real, não falha de teste

**Contexto:** T9 (Sala e Pareamento Simples) — primeiro teste que dependia de um `AActor` ter rodado `BeginPlay` antes do corpo do teste (`ABattleSquareGameMode::RoomRegistry`, criado ali). Resultado: `SIGSEGV: invalid attempt to access memory at address 0x50` dentro de `TMulticastDelegateBase::Broadcast`, ao chamar `RoomRegistry->OnRoomAbandoned.Broadcast(...)` — não uma falha de asserção, um crash de verdade do processo do Editor, com stack trace do sistema operacional.
**Causa:** o padrão já usado em `BattleArenaTest.cpp`/`BattleSquareGameModeTest.cpp` (`UWorld::CreateWorld(EWorldType::Game, false)` + `GEngine->CreateNewWorldContext` + `World->InitializeActorsForPlay(FURL())`) monta um mundo jogável o bastante para `SpawnActor` funcionar, mas **não garante** que `BeginPlay` dispare de forma confiável em atores spawnados depois — ao contrário do que um nível real carregado via `OpenLevel`/PIE garante. Toda lógica anterior (T1–T8) só usava métodos chamados explicitamente pelo teste, nunca dependeu de `BeginPlay` ter rodado — esta foi a primeira vez que essa suposição implícita foi testada, e ela era falsa.
**Solução:** `ABattleSquareGameMode::RoomRegistry` passou a ser criado por inicialização preguiçosa (`EnsureRoomRegistry()`, guardado por `if (!RoomRegistry)`), o mesmo padrão que `ABattleArena::TracePlayer` já usava desde a Apresentação do Combate — só que aqui a lição ficou explícita: `BeginPlay` chama `EnsureRoomRegistry()` para o caminho de jogo real, e o método é público para que testes headless chamem explicitamente também, sem confiar em `BeginPlay` ter rodado no mundo de teste.
**Previne:** qualquer estado de ator inicializado em `BeginPlay` (ou `PostInitializeComponents`) precisa continuar acessível/inicializável mesmo se `BeginPlay` não disparar — inicialização preguiçosa com guarda `if (!X)`, chamável de fora, é o padrão a seguir neste projeto para qualquer ator que precise ser testado num `UWorld` de teste montado à mão (não um nível real). E, mais geral: um SIGSEGV do processo (não um `TestFalse` falhando) é sempre root-cause investigado até o fim — nunca contornado silenciando o teste.

---

### L-019: `BatalhaEncerrada` nunca disparava em produção — `ResolveTurn` não decide vitória por design, e nada chamava `BattleOutcome::EvaluateOutcome` fora dos testes do próprio `BattleSim`

**Contexto:** T5 (Escala de Pets e Skills) — a primeira ferramenta que roda um combate **até o fim de verdade** (`RunBatchSimulation`, em vez de um único turno como todo teste anterior). Com `SafetyTurnCap = 10000` como rede de segurança, toda simulação bateu o teto sem nunca terminar — `AverageTurns: 10000.0`, `0.0% vitórias` dos dois lados.
**Causa raiz:** `FBattleResolver::ResolveTurn` (`BattleResolver.cpp`) **nunca** chama `BattleOutcome::EvaluateOutcome` — decisão arquitetural deliberada e documentada desde o núcleo (`BattleOutcome.h`: *"resolvido DEPOIS de `ResolveTurn`, não dentro dele... separação deliberada: T9 não sabe de condição de vitória, e nunca vai precisar saber"*). Isso está certo — o problema é que **a responsabilidade de chamar `EvaluateOutcome` depois de cada turno nunca foi implementada em nenhum consumidor real**: nem `ABattleArena::HandlePlayerCommitted` (Apresentação do Combate, M1) nem `UBattleTurnCoordinator::ResolveWithCommits` (Combate Online, M2) chamavam essa função. `BatalhaEncerrada` só era emitido dentro de `BattleOutcomeTest.cpp`, que testa `BattleOutcome::EvaluateOutcome` isoladamente — nunca no jogo de verdade. **Toda partida jogada em M1 e M2 até agora, sem exceção, nunca terminava por vitória/derrota/empate** — `bBattleEnded` ficava `false` para sempre, `UBattleResultWidget` nunca tinha dado real para mostrar. Passou despercebido porque nenhum teste anterior jogava uma partida até o fim: `BattleArenaTest.FullTurnEndToEnd` resolve 1 turno só; `BattleTurnCoordinatorTest` idem; nenhum teste de integração de M1/M2 jogava os até-10 turnos necessários para o limite de `MaxTurns` forçar um resultado.
**Solução:** `BattleOutcome::EvaluateOutcome(Result.NextState, Result.Trace)` chamado logo após `ResolveTurn` nos três lugares que resolvem turno em produção: `ABattleArena::HandlePlayerCommitted`, `UBattleTurnCoordinator::ResolveWithCommits`, e o novo `FBattleBalanceSimulator::RunBatchSimulation`. Descoberto no processo: `BattleOutcome::EvaluateOutcome` **também** nunca tinha `BATTLESIM_API` (mesmo defeito de L-016, agora numa segunda função do núcleo) — corrigido junto.
**Previne:** uma função pública do núcleo sem NENHUM consumidor real fora dos próprios testes é um sinal de alerta, não uma prova de correção — `BattleOutcome::EvaluateOutcome` "funcionava" havia semanas (nesta sessão) porque `BattleOutcomeTest.cpp` sempre passou, mas "os testes da função passam" e "a função é chamada em produção" são afirmações completamente diferentes, e só a segunda garante que a feature existe de verdade. A ferramenta que primeiro jogou uma partida **até o fim de verdade** (não um turno isolado) foi o que revelou isto — reforça o valor de pelo menos um teste de integração longa por sistema, além dos testes de unidade por turno.

---

### L-020: `Tools/probe_isolation.sh` deixa o `.dylib` de `BattleSim` num estado quebrado após rodar — nunca recompila de volta com o código bom

**Contexto:** Arenas Variadas (M3), rodando a bateria completa depois de `probe_isolation.sh` (T8, regressão). Três tentativas seguidas de `Automation RunTests BattleSquare` falharam com uma caixa de diálogo do próprio Editor: *"O módulo do jogo 'BattleSim' não pôde ser encontrado. Verifique se este módulo existe e que foi compilado."* — nem chegava a rodar nenhum teste, e o exit code do processo não indicava teste algum (nem Success nem Fail, só a mensagem de módulo ausente).
**Causa raiz:** `probe_isolation.sh` (Combate Núcleo) planta um `.cpp` inválido de propósito em `BattleSim`, roda `Build.sh`, **espera que falhe** (prova de que a fronteira do núcleo está intacta), e remove a sonda via `trap cleanup EXIT`. O script nunca recompila de novo depois de remover a sonda — ele só confirma que a build falhou como esperado e sai com `exit 0`. Isso deixa `Binaries/Mac/libUnrealEditor-BattleSim.dylib` no estado da última tentativa de link malsucedida (o UBT invalida/substitui o artefato de saída antes de o link falhar), até que ALGUÉM rode uma build de verdade depois. Rodar testes nesse intervalo — sem uma build explícita entre `probe_isolation.sh` e `Automation RunTests` — sempre falha da mesma forma.
**Como foi diagnosticado:** o erro sempre citava "BattleSim" mesmo ao filtrar só testes de "BattleSquare" (que também depende do módulo BattleSim, carregado no boot do processo independente do filtro de teste) — o que por si só já apontava para módulo ausente, não teste real falhando. Confirmado comparando timestamp do `.dylib` com o horário da última build bem-sucedida, e reproduzindo o padrão de forma determinística: sempre falha logo depois de `probe_isolation.sh`, sempre passa logo depois de uma build normal.
**Solução:** disciplina de sessão, não mudança de script (o script está correto para o que ele promete — provar que a sonda falha; só o *efeito colateral* no `.dylib` não estava documentado): **sempre rodar `Build.sh` de verdade depois de `probe_isolation.sh`/`audit_no_recalculation.sh`/`audit_no_commit_replication.sh` (qualquer sonda que planta código e espera falha), antes de rodar `Automation RunTests` de novo.** Um `Build.sh` extra depois da sonda custa poucos segundos (é só um `Link`, o resto já está em cache) — muito mais barato que repetir o ciclo de diagnóstico que este achado custou.
**Previne:** qualquer sonda no padrão "planta código que deve falhar a build, depois remove" (`probe_isolation.sh` e as sondas por grep como `audit_determinism.sh`/`audit_no_recalculation.sh` são diferentes — essas nunca chegam a compilar nada, só fazem grep de texto, e por isso NÃO têm este problema) precisa de uma build de recuperação explícita depois, sempre, antes de qualquer teste real ser considerado válido.

---

### L-021: `Tools/audit_no_recalculation.sh` tinha escopo grande demais — sinalizou como violação um cálculo legítimo de montagem de partida

**Contexto:** Coleção e Captura (M4) — regressão final (T6) rodou `audit_no_recalculation.sh` e ele FALHOU pela primeira vez, apontando `FBattleDataTranslator::TranslateMatchup` (`OutLeftState.Attack = (OutLeftState.Attack * LeftEffectivenessPercent) / 100;`). Essa linha existe desde Escala de Pets e Skills e é a decisão arquitetural CENTRAL daquela feature (DP-escala-01) — pré-multiplicar `Attack` pela efetividade de tipo, precisamente para NUNCA precisar tocar `BattleSim`.
**Causa:** a sonda varria `Source/BattleSquare` inteiro, sem distinguir **código de montagem** (`Data/`, `Balance/` — roda ANTES da batalha começar, tem permissão de computar atributos derivados) de **código de apresentação** (`Battle/`, `UI/` — consome o trace DEPOIS que o núcleo já resolveu, e é isso que BTL-22 proíbe recalcular). O padrão de regex (`Attack *`, `Defense -`) não sabe a diferença — só o caminho do arquivo sabe.
**Por que só apareceu agora:** é a primeira vez que a bateria de regressão completa desta sessão rodou `audit_no_recalculation.sh` DEPOIS de `TranslateMatchup` existir e o script ter sido escrito com esse escopo — plausível que tenha passado despercebido em alguma verificação anterior desta sessão sem eu confirmar a saída com o mesmo rigor, ou que uma diferença sutil de ambiente tenha mudado o resultado. De qualquer forma, o que importa é a causa raiz, não reconstituir exatamente quando começou.
**Solução:** escopo da sonda restrito a excluir `*/Data/*` e `*/Balance/*` — documentado explicitamente no cabeçalho do script POR QUÊ esses diretórios são isentos (constrói o estado inicial, não recalcula um resultado já decidido). Ciclo plantar→falhar→remover→limpo reconfirmado, incluindo um teste específico de que uma violação real em `Battle/` (fora da isenção) continua sendo pega.
**Previne:** sondas por grep de caminho de arquivo precisam do MESMO rigor de manutenção que o código de produção — um escopo bom demais na hora de escrever pode ficar ruim demais depois que a arquitetura ganha uma nova camada legítima (aqui, "montagem de partida" como conceito só nasceu com Escala de Pets e Skills, depois da sonda já existir). Quando uma sonda falha apontando código que uma decisão de design já revisou e aprovou, a pergunta certa não é "como escondo isto", é "o escopo da sonda ainda reflete a intenção original dela".

### L-022: mesma classe de falso positivo de L-021 reapareceu num diretório novo (`Meta/`) assim que ele passou a fazer montagem de partida

**Contexto:** Níveis, Experiência e Evolução (M4) — regressão final (T6) rodou `audit_no_recalculation.sh` e ele FALHOU apontando `FPetProgressionService::ApplyLevelBonus` (`State.Attack = (State.Attack * BonusPercent) / 100;`), chamado por `ABattleSquareGameMode::ApplyOwnedPetProgressionBonus` para escalar atributos pelo nível do pet ANTES de montar `InitialState` — exatamente a mesma categoria de código de montagem que L-021 já tinha isentado, só que em `Meta/` em vez de `Data/`/`Balance/`.
**Causa:** a exclusão de L-021 cobria os dois diretórios que existiam na hora — não previu que uma feature futura criasse um TERCEIRO diretório de montagem legítima. `Meta/` já existia (Coleção e Captura), mas só passou a fazer cálculo de atributo nesta feature.
**Solução:** adicionado `-not -path "*/Meta/*"` à mesma exclusão, com o comentário do cabeçalho atualizado para nomear os três diretórios e a razão comum (roda antes do `BattleSim` ver o pet). Ciclo plantar→falhar→remover→limpo reconfirmado.
**Previne:** a exclusão por diretório é uma lista que cresce toda vez que uma nova camada de montagem-antes-da-batalha nasce — não é uma correção pontual e definitiva. Ao criar qualquer diretório novo com responsabilidade de "montar estado inicial" (não só `Data/`/`Balance/`/`Meta/`), checar se ele precisa entrar nesta lista antes que a sonda gere outro falso positivo evitável.

### L-023: propriedade da engine que aceita a escrita e depois a descarta — sucesso do `set_properties` não é confirmação

**Contexto:** Streaming de Mundo (M5), T2 — o design pedia célula de World Partition de 512uu (DP-streaming-02). `ObjectTools.set_properties` com `cellSize: 512` retornou `true`; a leitura seguinte devolveu **1600**.
**Causa:** `URuntimePartitionLHGrid::PostEditChangeProperty` (`Engine/Source/Runtime/Engine/Private/WorldPartition/RuntimeHashSet/RuntimePartitionLHGrid.cpp:128`) faz `CellSize = FMath::Max<int32>(CellSize, 1600)`. A escrita acontece, o `PostEditChange` a sobrescreve, e a API de MCP reporta sucesso porque a escrita de fato ocorreu.
**Solução:** DP-streaming-02a emendou o design para o piso da engine (célula 1600uu, raio 3200uu), preservando a intenção original (raio = 2 células, vizinhança 5x5) em vez do número absoluto.
**Previne:** toda escrita de propriedade via `unreal-mcp` que carrega um valor que o design escolheu deliberadamente precisa de um `get_properties` de confirmação depois. `PostEditChange` com clamp é comum na engine e é invisível pelo retorno da ferramenta. Quando a leitura discordar da escrita, o caminho é ler o `.cpp` da classe — não repetir a escrita.

### L-024: `set_properties` em asset de HLOD travou o game thread do Editor por tempo indefinido

**Contexto:** Streaming de Mundo (M5), T5 — `set_properties` num `HLODLayer` com `{cellSize, loadingRange, editorLoadingBehavior: "ForceLoaded"}` nunca retornou. O log do Editor parou de avançar de frame na linha do `Dispatching toolset tool`, com um `LogMetal: Creating NativeLayer ... in Metal Viewport` logo em seguida — sinal de janela nova. 18 minutos de silêncio total do game thread, com ~26% de CPU em threads de fundo.
**Causa:** não isolada com certeza; o suspeito é `editorLoadingBehavior: ForceLoaded`, que força carregar atores de HLOD e pode abrir um diálogo modal — e diálogo modal bloqueia o game thread, que é exatamente onde toda chamada de MCP roda. Sem alguém no teclado, o bloqueio é permanente.
**Solução:** matar e reabrir o Editor (tudo que já tinha sido salvo sobreviveu: nível, 225 atores externos, 25 materiais; o `HLODLayer` não-salvo simplesmente não existia), e refazer T5 **sem tocar nas propriedades opcionais** — os defaults do layer duplicado (Instancing, sempre carregado, 25600/51200) já serviam. O build seguiu limpo: 36 atores de HLOD.
**Previne:** duas coisas. (1) **Salvar antes de cada passo de MCP que muda asset**, não só no fim do lote — foi o que tornou o kill barato aqui. (2) Preferir o default do asset a escrever propriedade que não é exigida pelo design; cada propriedade escrita é uma chance de `PostEditChange` abrir modal. E ao suspeitar de travamento, o diagnóstico é o **contador de frame do log do Editor**, não o uso de CPU: threads de fundo continuam ocupadas com o game thread parado.

### L-025: `UnrealEditor.modules` pode apontar para um dylib mais velho que o recém-buildado — teste "passa" contra código que não existe mais

**Contexto:** Encontros e Transição para Batalha (M5), T1/T2 — depois de escrever os testes e rodar `Build.sh` com `Result: Succeeded`, `Automation RunTests BattleSquare.World.EncounterDetector` respondeu **`No automation tests matched`**, e depois o suite inteiro devolveu 85 quando deveria devolver 88. Nenhum erro em lugar nenhum.
**Causa:** `Binaries/Mac/` tinha acumulado dylibs numerados de hot-reload (`libUnrealEditor-BattleSquare-0002…0005.dylib`), sobra de builds feitos **com o Editor aberto** durante a feature anterior. `Binaries/Mac/UnrealEditor.modules` continuava apontando para `-0004` enquanto o build novo escrevia `-0005`. O `UnrealEditor-Cmd` carrega o que o manifesto manda — ou seja, o binário anterior. O build compila, o link tem sucesso, o teste roda, e nada indica que o código testado é de duas versões atrás.
**Solução:** apagar **todos** os dylibs dos módulos do projeto, incluindo o sem número — `rm -f Binaries/Mac/libUnrealEditor-BattleSquare*.dylib Binaries/Mac/libUnrealEditor-BattleSim*.dylib` — e rebuildar. Apagar só os numerados **não basta**: o build seguinte escreve `-000N+1` e o manifesto continua citando `-000N`, então a defasagem de um build se perpetua (reincidiu exatamente assim em T3: manifesto em `0006`, dylib novo em `0007`, suite travado em 88 quando deveria dar 93). Depois da limpeza completa, `Binaries/Mac/` fica com **um** dylib por módulo e o manifesto cita exatamente ele — foi aí que a contagem subiu para 93.

**A receita definitiva, achada depois de reincidir mais duas vezes:** `rm` dos dylibs **seguido de exatamente UM build bem-sucedido**. Cada build extra depois do `rm` (inclusive um que falha a compilar) incrementa o sufixo sem o manifesto acompanhar, e a defasagem volta — foi assim que o manifesto chegou a citar `0011`/`0012` com `0013` em disco. Com um único build limpo, o UBT volta a escrever os dylibs **sem número** (`libUnrealEditor-BattleSquare.dylib`) e o manifesto cita esses — que é o estado saudável, e o sinal de que deu certo. Uma variante do sintoma é ainda mais barulhenta e vale reconhecer: se o `rm` apagar um módulo que o build seguinte não precisa relinkar, o Editor abre um diálogo `"O módulo do jogo BattleSim não pôde ser encontrado"` e o run devolve 0 teste — exatamente o mesmo susto de L-020, por uma causa vizinha.
**Previne:** este é o mesmo gênero de falha de L-020 (sonda deixa o `.dylib` quebrado) e merece a mesma disciplina: **teste novo que não aparece na contagem é problema de binário até prova em contrário, não de nome de teste.** O diagnóstico é uma linha — comparar o dylib citado em `Binaries/Mac/UnrealEditor.modules` com o mais recente em `ls -lt Binaries/Mac/`. E a raiz é evitável: buildar com o Editor aberto é o que gera os numerados; fechar o Editor antes de `Build.sh` mantém `Binaries/` limpo. **A conferência confiável é `ls Binaries/Mac/ | grep <Módulo>` devolver uma linha só e ela bater com o manifesto** — comparar com "o mais recente" engana, porque o mais recente pode ser justamente o que o manifesto ainda não adotou.

### L-026: um teste inválido que crasha o processo se disfarça de "os testes novos não apareceram"

**Contexto:** Traversal e Câmera de Mundo (M5), T3 — depois de escrever 10 testes novos, a bateria devolveu **104** em vez de 112, e só 2 dos 10 novos apareceram na lista. O reflexo, treinado por L-025, foi culpar o manifesto de módulo de novo.
**Causa:** era outra coisa. O teste `MissingInputAssetsDoNotCrash` chamava `Explorer->SetupPlayerInputComponent(nullptr)`, e o `Super::` da engine desreferencia o ponteiro — SIGSEGV dentro do `AutomationWorker`. O processo morreu no meio da bateria, e **todo teste que viria depois em ordem alfabética simplesmente não rodou** (`WorldTraversalMotion` vem depois de `WorldExplorerCharacter`). O `Automation RunTests` não reporta isso como falha: ele reporta os que conseguiram terminar, e o resto some.
**Solução:** passar um `UEnhancedInputComponent` real com os *assets* nulos — que é o estado que o teste queria exercitar de verdade (ninguém autorou os `UInputAction` ainda), não um `UInputComponent` nulo, que é um estado que a engine nunca produz. O código de produção estava correto o tempo todo.
**Previne:** "faltam testes na contagem" tem pelo menos duas causas, e elas se confundem: binário velho (L-025) e **processo morto no meio da bateria**. O que as separa é uma linha — `grep -a StaticShutdownAfterError` no log; se aparecer, houve crash e o problema é um teste, não o build. E a lição de fundo: ao escrever um teste de robustez, exercitar o estado inválido que o **sistema real produz**, não o mais nulo que o compilador aceita — `nullptr` num parâmetro que a engine sempre preenche testa a engine, não o nosso código.

### L-027: bug que só existe no build empacotado — o Editor é modular, o pacote é monolítico

**Contexto:** M6, ao empacotar para Mac pela primeira vez (`RunUAT BuildCookRun`). O link falhou com **270 símbolos duplicados** de `sqlite3` entre o módulo `SQLiteLibrary` do projeto (que compilava a própria cópia de `sqlite3.c`) e o plugin `SQLiteCore` da engine.
**Por que nunca apareceu antes:** o alvo **Editor é modular** — cada módulo vira um `.dylib` e o símbolo duplicado se resolve por precedência de link dinâmico, em silêncio. O alvo de **jogo é monolítico**: tudo entra num binário só, e aí o duplicado é erro de link. **As 118 verdes do Editor não diziam nada sobre isso**, e não tinham como dizer.
**Causa raiz:** `AD-019` escolheu um módulo barreira para o SQLite e, no mesmo texto, registrou o princípio certo ao falar do OpenSSL — *"consumir o que a engine já tem, não vendorizar de novo"*. O princípio simplesmente não foi aplicado ao SQLite: `sqlite3.c` foi vendorizado em `Source/SQLiteLibrary/Public/`.
**Solução:** o projeto passou a depender do plugin `SQLiteCore` da engine, que expõe a API C **de propósito** (`SQLiteCore.Build.cs`: `SQLITE_API=SQLITECORE_API`, com o comentário *"Allow direct access to the SQLite API from outside this module"*). O módulo barreira foi removido: sem nada vendorizado para isolar, ele não tinha mais função — e, esvaziado, uma `PublicDependencyModuleNames` dele nem propagava (módulo sem fonte alguma não repassa a dependência, o que gerou um segundo erro de link, agora de símbolo **indefinido**, antes de a causa ficar clara). Os 118 testes continuam verdes, incluindo os do `FPetDataLoader`, que de fato abrem o espelho SQLite — prova de que a implementação da engine serve igual.
**Shipping também foi validado (2026-08-26):** `BuildCookRun -clientconfig=Shipping` passou **de primeira**, sem nenhuma correção adicional — as duas correções acima já bastaram para os dois configs. O binário de Shipping tem 272 MB contra os ~420 MB do de Development, e o pacote inteiro 744 MB. Duas coisas a saber sobre verificar Shipping, porque enganam: ele **ignora `-ExecCmds`** (o console é removido em Shipping, então `Quit` não encerra) e **não escreve log nenhum** (a saída sai com 0 bytes). A verificação que resta é a de fora: subir o processo, confirmar que ele **sobrevive ao boot** por dezenas de segundos e não gera crash report. Foi o que se fez.

**Previne:** **empacotar cedo, mesmo sem precisar do pacote.** Uma classe inteira de defeito — símbolo duplicado, dependência de módulo faltando, plugin não habilitado, config malformada — é invisível para `Automation RunTests` no Editor e só aparece no primeiro `BuildCookRun`. Este projeto passou de M1 a M7 sem empacotar uma vez, e acumulou dois defeitos silenciosos até aqui (este e o `ProjectID` de L-028). **A boa notícia da mesma medida:** uma vez corrigidos, Shipping passou sem nada novo — o custo de empacotar cedo é pago uma vez, não a cada config.

### L-028: `FGuid` em `.ini` com hífens faz o cook falhar por "1 error"

**Contexto:** logo depois de L-027, o cook falhou com `Failure - 1 error(s)` e a única linha de erro era `LogObj: Error: LoadConfig (/Script/EngineSettings.Default__GeneralProjectSettings): import failed for ProjectID in: 59423EED-39EA-47AB-A1F5-090E34A682C6`.
**Causa:** `ProjectID` é um `FGuid`, e o importador de `FGuid` da Unreal espera os 32 dígitos hexadecimais **sem hífens**. Com hífens o import falha; o cook conta como erro e aborta o empacotamento inteiro.
**Solução:** `ProjectID=59423EED39EA47ABA1F5090E34A682C6`.
**Previne:** um erro de *parse de config* aborta o cook com a mesma severidade de um asset quebrado, e a mensagem não diz "corrija o formato" — diz só "import failed". Ao ver um cook falhar com 1 erro e nenhum problema de conteúdo aparente, procurar `LoadConfig .* import failed` antes de suspeitar de asset.

### L-029: em World Partition, os atores do nível não existem no `BeginPlay` do GameMode

**Contexto:** ao ligar o bootstrap de encontros de mundo (M5), `ABattleSquareGameMode::BeginPlay` varria o mundo por um pawn com `UEncounterDetectionComponent` e não achava nada — `"nenhum pawn com UEncounterDetectionComponent no nível"` — apesar de o nível ter **dois**.
**Causa:** em nível com World Partition os atores vivem em pacotes externos e chegam por **streaming**, depois do `BeginPlay` do GameMode. Varrer o mundo naquele instante encontra um mundo praticamente vazio. É a mesma família de L-018 (`BeginPlay` não é o gancho confiável que parece), agora por streaming em vez de ordem de spawn.
**Solução:** a tentativa passou para o `Tick` do GameMode (que já roda 1x/s), repetindo até conseguir, e o motivo da falha é logado **uma vez só** — no Tick, um log por tentativa viraria enxurrada.
**Segundo achado, no mesmo lugar:** a varredura escolhia "o primeiro pawn que o iterador achar", e o nível de verificação tem dois pawns com detecção (o de rota e o de exploração). Isso deixaria a decisão para a ordem de registro do mundo — não-determinismo silencioso, do tipo que AD-004 recusa. A regra virou explícita: **vence o pawn possuído pelo jogador**; só na ausência dele o primeiro serve.
**Previne:** em World Partition, qualquer código que precise ENCONTRAR atores do nível tem de tolerar que eles ainda não estejam lá. E toda varredura que escolhe "o primeiro" precisa de uma regra de desempate escrita, ou a ordem do mundo decide por você.

### L-030: verificar runtime por `grep` em log de binário empacotado é método ruim

**Contexto:** para provar que a corrente "andar → encontrar → batalhar" funcionava, tentei rodar o jogo (`-game` sem cook, depois o pacote) e procurar as linhas de log do bootstrap. `-game` sem cook **não monta o streaming do World Partition**; o pacote monta, mas não emitiu nenhuma das nossas linhas de `LogTemp`, e cada tentativa custava um ciclo de empacotar/rodar/matar/procurar. Um `kill -9` ainda comeu o log inteiro por matar antes do flush.
**Solução:** parar e escrever o teste headless que faltava — `BattleSquare.World.WorldEncounterBootstrap`, três casos (fia com pawn e espelho; sem pawn devolve motivo; sem espelho devolve motivo). Ele carrega o espelho criptografado de verdade e prova a fiação em segundos, de forma repetível.
**Previne:** quando a verificação vira arqueologia de log, o sinal não é "melhorar o grep" — é que falta um teste. O que o pacote prova, e só ele, é a parte de **engine** (streaming real, posse, colisão); isso continua em roteiro manual, por DP-enc-05. A parte que é **decisão nossa** sempre cabe num teste, e cabia neste.

### L-031: exceção não tratada no `Bun.serve` devolveu e-mail e hash da senha AO CLIENTE

**Contexto:** primeiro item do roteiro de conta rodado contra Postgres real (M7). Registrar um e-mail duplicado devolveu **HTTP 500** com a página de diagnóstico do Bun — e dentro dela, em base64, a query e os **parâmetros**: `params: jogador...@exemplo.com,$argon2id$v=19$m=65536,...`. Ou seja, o e-mail (PII) e o hash da senha viajaram para o cliente.
**Causa, em duas camadas:** (1) `isUniqueViolation` checava `error.code === '23505'` no topo, mas o Drizzle embrulha o erro do driver num `DrizzleQueryError` e o código do Postgres fica na **causa** — a checagem dava `false`, o 409 nunca acontecia e a exceção subia; (2) o `Bun.serve` não tinha handler `error`, e o padrão dele é uma página de diagnóstico que inclui os parâmetros da query.
**Solução:** `isUniqueViolation` passou a caminhar a cadeia de causas (com profundidade limitada, para cadeia circular não virar laço) e virou `src/db/postgres-error.ts` com 6 testes próprios; e o `Bun.serve` ganhou handler `error` devolvendo `500 INTERNAL_ERROR` genérico, logando só o **nome** do erro — nunca params, nem no log.
**A segunda camada é a que importa:** a primeira era um bug de mapeamento; a segunda era a ausência de uma barreira que **qualquer** rota precisava. O módulo de pets tinha o mesmo buraco desde M4, e ninguém tinha esbarrado nele.
**Previne:** todo servidor HTTP precisa de barreira de erro global **antes** da primeira rota, não depois do primeiro vazamento. E um defeito de mapeamento de erro (`409` virando `500`) é barato; o que ele revela — que o caminho de exceção não tratada expõe dado — é o que custa caro.

### L-032: dois validadores para a mesma regra, e o de fora escondia o de dentro

**Contexto:** ainda no roteiro de conta, `CTA-03` (senha fraca) devolvia **um** motivo, não todos. `validatePasswordPolicy` devolve todos de uma vez, tem teste próprio provando isso, e mesmo assim o usuário via um.
**Causa:** `registerAccountSchema` tinha `password: z.string().min(PASSWORD_MINIMUM_LENGTH)`. O Zod recusava antes, e a política — escrita justamente para não devolver um motivo por vez — nunca era alcançada. **O teste unitário da política passava e continuava passando**: ele testava a função, e o problema era que ninguém a chamava naquele caminho.
**Solução:** o schema valida **forma** (é string? cabe no teto?); as **regras** ficam na política. O `.max()` permaneceu no schema de propósito: é proteção contra Argon2id sobre entrada gigante, e precisa recusar antes de hashear.
**Previne:** quando a mesma regra existe em duas camadas, a de fora decide e a de baixo vira código morto que continua verde. Vale perguntar, a cada validação nova, **qual camada é dona daquela regra** — e deixar só ela com a regra.

### L-033: a regra estava certa, testada e chamada — em UM dos dois caminhos de entrada

**Contexto:** `MOD-04` do roteiro de moderação, rodado contra Postgres real. Uma conta **banida** conseguia renovar a sessão e receber um `accessToken` novinho, com HTTP 200. E como a renovação emite um refresh novo a cada vez, ela renovaria para sempre: o banimento nunca chegaria a valer para quem já tinha entrado antes de ser banido.
**Causa:** `resolveBanState` estava correta e tinha 10 testes puros. `authenticateAccount` (login) a chamava. **`refreshSession` não.** Existem dois caminhos que emitem sessão, e eu guardei um.
**Solução:** a checagem entrou no `refreshSession` também, e o token é **revogado** ao ser recusado — recusar sem consumir deixaria o mesmo token disponível para tentar de novo.
**O que isto tem de diferente de L-032:** lá eram duas camadas validando a mesma regra e a de fora escondendo a de baixo. Aqui é a mesma regra guardando **duas portas**, e só uma tendo fechadura. O sintoma é idêntico — teste unitário verde, comportamento errado em produção — e a causa raiz é a mesma família: **o teste prova que a função funciona, nunca que ela é chamada em todo lugar onde precisa ser.**
**Previne:** para toda regra que **guarda acesso**, enumerar explicitamente os pontos de entrada antes de dar por pronta — aqui eram login e renovação; num sistema maior seriam também troca de senha, emissão de token de API, e qualquer rota que crie sessão. A pergunta que faltou: *"que outros caminhos emitem sessão?"*
**E o mais importante:** este defeito só apareceu porque o roteiro tinha um item escrito para procurá-lo, com o motivo anotado de antemão (*"é o que separa banimento real de anotação"*). Roteiro manual escrito com hipótese de falha vale muito mais que roteiro que só descreve o caminho feliz.

### L-034: a batalha era de UM turno só, e um teste AFIRMAVA o defeito

**Contexto:** primeiro turno jogado por um humano, com a interface finalmente na mão (2026-08-26). O commit funcionou, a rodada resolveu — e a tela travou em "Turno fechado — aguardando o oponente", para sempre.
**Causa:** `UBattleActionQueueComponent` não tinha como recomeçar. Nenhum `Reset`, nenhum `BeginNewTurn`. Commitada uma vez, ficava commitada. **A batalha acabava na primeira rodada**, e o jogo inteiro de M1–M4 nunca tinha jogado duas.
**O agravante:** `BattleSquare.BattleArena.FullTurnEndToEnd` afirmava, com todas as letras, `TestTrue("Fila travada após Commit")`. O teste não só deixou passar o defeito — ele o **fixou como contrato**. Quem tentasse consertar veria um teste vermelho e concluiria que estava errado.
**Solução:** `BeginNewTurn()` (esvazia, volta ao passo de tipo, destrava, com UM broadcast no fim), chamado por `ABattleArena` depois de resolver e **só se a batalha não acabou** — fila travada é o que impede escolher ações para uma partida encerrada. O teste foi atualizado com o porquê escrito no comentário, para ninguém "consertar" de volta. `BattleSim` não foi tocado: o núcleo nunca soube que existe turno seguinte, e continua sem saber.
**Previne:** duas coisas, e a segunda é a maior.
1. Testar **um** ciclo de um sistema cíclico não testa o sistema. A pergunta que faltava era "e a segunda rodada?" — a mesma família de L-032/L-033 (o teste prova a peça, não o uso).
2. **Um teste que afirma o comportamento observado, sem perguntar se ele é o desejado, transforma defeito em contrato.** "Fila travada após Commit" era uma descrição fiel do que o código fazia, e uma descrição errada do que o jogo precisava. Ao escrever asserção sobre estado FINAL de um fluxo, vale perguntar: isto é o que deve acontecer, ou só o que acontece hoje?

### L-035: consertar por hipótese antes de medir custou mais que o defeito

**Contexto:** ao dar interface ao combate (26–27/08/2026), o usuário relatou "não anda". Corrigi, em sequência: a câmera que não olhava a arena, o registro do Enhanced Input em `NotifyControllerChanged`, e o `DefaultKeyMappings` deprecado. **As três correções estavam certas** — e nenhuma era comprovadamente *a* causa do que ele via. A causa real era eu abrir o Editor por shell em segundo plano, o que impede o teclado de chegar ao PIE.
**O padrão do erro:** cada correção era plausível, barata e verdadeira. Isso é exatamente o que a torna perigosa: ela dá a sensação de progresso e consome um ciclo inteiro de "compila, reabre, joga, relata" do usuário. Três ciclos assim custaram mais que o defeito.
**O que finalmente funcionou:** instrumentar. Uma sonda de tecla crua, ao lado do Enhanced Input, respondeu em uma execução que a tecla **não chegava ao jogo por caminho nenhum** — e isso descartou meu código inteiro de uma vez.
**Previne:** diante de um defeito relatado, a primeira ação é **reproduzir e medir**, não consertar. Ordem que ficou registrada em `CLAUDE.md`: (1) teste headless que reproduz, (2) inspeção do estado vivo, (3) instrumentação dirigida no log, (4) o usuário olhando a tela. Corrigir sem ter medido é apostar o tempo de outra pessoa numa hipótese.
**Corolário sobre ferramenta:** o modo como eu *opero* o ambiente é parte da superfície de defeito. `open -a` em vez de shell em segundo plano não é detalhe de conveniência — foi a causa raiz.

### L-036: teste que mede o sintoma reprova o comportamento certo

**Contexto:** `OpponentMovesAtLeastOnceOverTenTurns` existia para responder "o inimigo está inerte?" — pergunta nascida do defeito da semente, que fazia toda partida repetir a mesma sequência. Ele media isso por **movimento**, porque a IA de então sorteava e mover era o sinal mais fácil de observar. Ao trocar por uma IA que decide, o teste reprovou: com os pets começando adjacentes, o oponente **ataca em vez de andar**, que é jogar melhor.
**O erro não foi o teste falhar — foi ele medir o sintoma.** "Sai do lugar" era um proxy de "age", válido só enquanto a IA era aleatória. Proxy amarrado a uma implementação vira armadilha quando a implementação melhora: o teste passa a defender o comportamento antigo.
**Como distinguir de uma regressão real:** perguntar o que o teste queria saber, não o que ele afirma. Aqui a pergunta era inércia; a resposta certa passou a ser "andou OU feriu". Corrigi a asserção para a pergunta original e adicionei um teste separado para o novo comportamento.
**Parentesco com L-034:** lá um teste afirmava um defeito como contrato (`TestTrue("Fila travada após Commit")`). Aqui não havia defeito — havia um proxy que envelheceu. Os dois entram pela mesma porta: o teste deixou de descrever a **intenção** e passou a descrever a **mecânica**.
**Previne:** ao escrever asserção sobre comportamento emergente, escrever a INTENÇÃO ("o inimigo age") e não o sinal observado ("o inimigo anda"). Quando um teste reprovar após uma melhoria deliberada, ler a intenção antes de mexer — enfraquecer a asserção até passar teria escondido justamente o que mudou.

### L-037: decisão de produto invertida deixa testes obsoletos, não errados

**Contexto:** ao inverter DP-02, `BattleSim.Phase.Movement.OpposingSidesCanCoexist` reprovou. Ele afirmava, corretamente, a regra anterior: lados opostos coabitam.
**A distinção que importa:** este teste não estava errado nem envelhecido por proxy (L-036) nem afirmando um defeito (L-034). Ele descrevia com precisão uma decisão de produto que deixou de valer. As três situações reprovam igual e pedem respostas diferentes — e é por isso que "o teste ficou vermelho" nunca basta como diagnóstico.
**O que fiz:** converti o teste no lugar, mantendo o nome do arquivo e a posição no arquivo, com a asserção invertida e um comentário dizendo que a regra foi invertida e quando. Apagar teria sido mais rápido e teria transformado a inversão numa **ausência silenciosa**: quem lesse o histórico não acharia a regra nova onde a antiga morava.
**Corolário sobre código morto:** a inversão tornou inalcançável a busca por oponente coabitando em `ResolveTarget`. Removi junto. Deixar teria criado uma segunda verdade sobre coabitação — e L-032, L-033 e o defeito de direção mostraram que duas cópias concordam só até a primeira edição.
**Previne:** ao inverter uma decisão de produto, procurar os testes que a afirmavam e CONVERTÊ-LOS, e procurar o código que só existia para ela e REMOVÊ-LO. Os dois passos, não só o primeiro.

### L-038: caminho que falha três vezes é dependência errada, não detalhe errado

**Contexto:** a tecla de copiar o painel (F9) falhou três rodadas seguidas. Cada vez consertei um detalhe diferente e cada vez estava certo sobre o detalhe: a classe de controlador não estava declarada no GameMode da tela; o foco do teclado era pedido antes de o widget entrar na viewport; F8 é a tecla de Eject do PIE. Três causas reais, três correções corretas, e nas três o usuário voltou dizendo que não funcionou.
**O erro de método:** eu tratava cada falha como um defeito isolado. A terceira deveria ter mudado a pergunta de "qual detalhe quebrou?" para "por que dependo disto?".
**O que resolveu:** remover a dependência. O painel passou a se gravar sozinho em arquivo a cada mudança — sem tecla, sem foco, sem modo de input, sem área de transferência — e as ações ganharam BOTÕES, que era o caminho comprovadamente funcional o tempo todo, já que o usuário clicava nos botões de ação sem problema nenhum. **A evidência estava disponível desde a primeira rodada** e eu não a usei.
**Corolário que me custou outra rodada:** ao compilar o alvo Shipping descobri que `OnApplicationPreInputKeyDownListener` não existe fora do editor — a solução da rodada anterior nunca funcionaria empacotada. Compilar o Shipping levava 20 segundos.
**Previne:** contar as tentativas. Na terceira falha do mesmo caminho, parar de consertar e trocar o caminho por um que já se sabe que funciona naquele contexto.

### L-039: etapa a mais é chance a mais de parecer destrutivo

**Contexto:** o usuário pediu para escolher as ações do jogador 2 na tela. Entreguei três versões: modo para ligar, troca de lado que zerava as escolhas, e confirmação em duas fases. Ele travou nas três, e a última travou por rótulo — "confirmar turno" na vez do jogador 1 parecia que ia RESOLVER a batalha, então ele não clicava, e sem clicar o jogador 2 era inalcançável.
**O padrão:** cada etapa que eu acrescentava era um ponto onde a pessoa tinha que adivinhar se a ação era segura. Consertei os rótulos duas vezes; o defeito eram as etapas.
**O que resolveu:** um painel sempre visível, ações escolhidas direto, e o turno fechando pelo botão que já existia. Zero modos, zero fases.
**Previne:** quando a correção de um fluxo é explicar melhor o que cada passo faz, considerar remover o passo. Rótulo bom não conserta fluxo com etapa desnecessária — só a torna compreensível.

### L-040: caminho que nunca foi percorrido até o fim não tem defeito — tem ausência

**Contexto:** o usuário caminhou pelo mundo, achou o inimigo, foi para a arena, derrotou o inimigo — e **nada aconteceu**. Ele ficou preso numa arena de uma partida já terminada.
**A causa:** captura, XP e o anúncio de fim de batalha eram chamados **só no caminho de REDE** (`HandleCoordinatorTurnResolved`). A batalha local resolvia o turno, avaliava o desfecho, e não contava a ninguém. Não havia código errado — havia código que nunca fora escrito.
**Por que passou por M1–M4 e pelos testes:** nenhuma partida jamais tinha sido jogada **até o fim por uma tela**. Os testes de resolução paravam no turno; os de rede exercitavam o outro caminho. O caminho local existia, era testado em pedaços, e nunca tinha chegado ao seu último passo.
**O parentesco com defeitos anteriores:** é a mesma família de "a batalha era de um turno só" (L-034) e "ninguém criava encontros". Em todos, cada peça funcionava isolada e ninguém tinha percorrido a sequência inteira. **Teste de unidade não descobre etapa que não existe** — só o percurso completo descobre.
**Previne:** para todo fluxo com começo, meio e fim, escrever um teste que percorra **até o fim**, não só cada etapa. E quando o usuário disser "nada aconteceu", suspeitar primeiro de ausência, não de erro: erro produz comportamento errado; ausência produz silêncio.

### L-041: feature testada não é feature alcançável — varra os caminhos, não as peças

**Contexto:** em 2026-08-27/28 encontrei QUATRO recursos completos, testados e **inalcançáveis em produção**, todos pela mesma razão: ninguém os chamava.

| Recurso | Como estava | Consequência para quem joga |
|---|---|---|
| Encontros no mundo | ninguém criava `AWorldEncounterActor` | caminhar nunca disparava batalha |
| Fim da batalha local | captura/XP/anúncio só no caminho de rede | derrotar o inimigo não devolvia ao mundo |
| Layouts de arena | `CellLayout` nascia neutro e ninguém o preenchia | casa de dano, bônus e água jamais apareciam |
| Efetividade de tipo | tabela nunca carregada; produção usava `TranslatePet` em vez de `TranslateMatchup` | **Fogo contra Planta batia igual a Fogo contra Água** |

**O que os quatro têm em comum:** cada peça tinha teste, cada teste passava, e nenhum teste percorria o caminho do jogador de ponta a ponta. Bateria verde e recurso morto convivem sem atrito.

**A varredura que os encontrou** — e que vale repetir: para cada classe pública, contar em quantos arquivos de PRODUÇÃO (fora de `Tests/`) ela aparece. Um só é o próprio `.cpp` — ou seja, ninguém a usa. Foi assim que a efetividade de tipo apareceu, e ela é o coração de um jogo de coleção.

**Previne:** ao terminar uma feature, perguntar **quem chama isto no jogo** e escrever o teste que percorre o caminho inteiro. E rodar a varredura de "classe sem uso em produção" periodicamente — ela custa um comando e achou quatro recursos mortos numa tarde.

**A varredura completa (2026-08-28)** achou 9 classes sem uso em produção, de 64 declaradas. Sete são legítimas: GameModes e pawn referenciados pelo NÍVEL e por config (não por C++), o simulador de balanceamento que é ferramenta de teste, a sonda de rota determinística, e structs internas de um único `.cpp`. **Duas eram reais:** `UBattleResultWidget` (corrigida) e `FTouchMovementInput` (entrada por toque nunca ligada ao explorador — mobile segue bloqueado por B-006/B-007, e ligar sem poder verificar seria entregar trabalho não medido).

### L-042: helper de teste homônimo vira SOBRECARGA em unity build

**Contexto:** ao acrescentar um arquivo de teste em 2026-08-29, a bateria do `BattleSim` caiu de 68 testes para 37, com `Array index out of bounds: 0 into an array of size 0` derrubando o processo no meio. O arquivo novo passava sozinho.
**A causa:** três arquivos definiam `MakePet` em namespace anônimo, com assinaturas diferentes — `(PetId, Side, Column, Row)`, `(PetId, Side, Health, MaxHealth)` e `(PetId, Side, Health)`. O Unreal compila em **unity build**: vários `.cpp` viram uma unidade de tradução, os namespaces anônimos se fundem, e os homônimos viram **sobrecargas**. `MakePet(1, 0, 0, 0)` no teste de movimento passou a criar um pet com **vida zero** em vez de posição.
**O que torna isso perigoso:** não há erro de compilação. O defeito era **pré-existente e latente**, e só apareceu quando um arquivo novo mudou o agrupamento do unity build — ou seja, ele podia ter aparecido em qualquer commit futuro, sem relação com a mudança que o revelasse. E o sintoma (índice inválido num teste de movimento) não aponta para a causa (helper de outro arquivo).
**O que fiz:** dei nome próprio do arquivo a todos os helpers homônimos, e escrevi `Tools/audit_test_helper_names.sh`. A sonda achou na hora uma colisão que eu tinha deixado passar no mesmo trabalho (`Cena`, em dois arquivos) — o que confirma que revisão manual não bastava.
**Previne:** helper de teste leva nome do que ele monta, não nome genérico. A sonda entrou na lista de verificação do `CLAUDE.md`.

---

## Ideias Adiadas

- [ ] `apps/worker-pet-sync` usa o prefixo `worker-` (code-standart.md §2), mas as rules globais de Worker (`backend/worker.md`) descrevem especificamente consumidor RabbitMQ com ack manual/DLQ/prefetch — o sidecar de sync é um poller de intervalo (`setInterval`), não consome fila nenhuma. Nenhuma das três categorias documentadas (api/worker/cron) descreve exatamente esse padrão ("processo de fundo que se auto-agenda via poll HTTP periódico, sem fila e sem gatilho externo"). Não bloqueia nada — só fica registrado para quando as rules globais de backend forem revisadas, se fizer sentido nomear essa categoria explicitamente. Capturado em: 2026-08-25, Fase 5 do Backend de Dados de Pet.

- [ ] Criar um arquivo de **rules de desenvolvimento de jogos** (`~/.claude/rules/rules/games/unreal.md` ou similar), no mesmo espírito de `code-standart.md`, `frontend/web.md`, `apis.md` — nomenclatura C++/Blueprint, convenção de módulo, padrão de fronteira núcleo-vs-engine (o que este projeto já praticou em AD-011/AD-012), convenção de teste headless, checklist de auditoria de determinismo. Motivo: o usuário já tem rules globais para outros tipos de app (Bun/API, frontend web, banco de dados); faltam para jogos, e este projeto acumulou padrões reais que valeriam virar rule reaproveitável em outros projetos Unreal. Capturado em: 2026-08-25, durante o Backend de Dados de Pet.

- [ ] Espectador e replay a partir do trace de eventos — sai quase de graça do AD-005. Capturado em: Specify do Combate Núcleo.
- [ ] Simulação headless em lote para balanceamento (rodar 10.000 combates e medir taxa de vitória). Capturado em: Specify do Combate Núcleo.
- [ ] Modo batalha avulso em navegador, se a batalha for separável do mundo. Capturado em: decisão de motor.

---

## Todos

- [ ] Decidir as áreas cinzentas marcadas como PROPOSTA na spec do Combate Núcleo (ver seção "Decisões Pendentes")
- [x] Definir orçamento de performance de mobile — **resolvido em 2026-08-26**: `docs/performance/orcamento-mobile.md` (aparelho de referência declarado, 1,5 GB de processo / 384 MB de streaming / 30 FPS / até 720p). Chegou TARDE: M5 inteiro foi decidido contra o número provisório de DP-streaming-01, exatamente o risco que este Todo alertava. Todos os números são **alvo**, nenhum medido — medir depende de B-006/B-007.
- [x] Avaliar MCPs de controle do Unreal Editor — **resolvido em 2026-08-24**: existe plugin **oficial da Epic** (`EpicGames/unreal-engine-skills-for-claude-code-plugin`, v3.0.4, MIT, autor Thomas Mansencal/Epic), publicado no marketplace oficial da Anthropic. Descarta a necessidade de MCP comunitário. **Instalado e habilitado em 2026-08-24** (escopo de usuário, v3.0.4, 3 skills: `unreal-mcp`, `create-toolset`, `unreal-skill`; custo fixo ~1.3k tokens por sessão). Fica inerte até o editor subir.
      Exige Unreal Editor rodando com os plugins `ModelContextProtocol` e `AllToolsets`, e `ModelContextProtocol.StartServer` no console.
      **Cuidado registrado pelo próprio README da Epic:** `ProgrammaticToolset.execute_tool_script` executa Python arbitrário dentro do editor, com acesso total ao projeto em disco. Nunca usar com `--dangerously-skip-permissions`, e commitar antes de sessão longa dirigida por MCP.
- [ ] Reavaliar GAS em M3 — ver AD-007

---

## Preferences

**Model Guidance Shown:** 2026-08-25 (troca de modelo por fase confirmada pelo usuário via /model sonnet)

### L-043: o mapa tem um chão, e ele fica na frente do que você acabou de construir

**Contexto:** em 2026-08-29, ao dar geometria 3D à arena (chão, moldura e uma laje por casa), tudo compilou, os testes de asset atribuído passaram, e o tabuleiro teria nascido **invisível** — enterrado.

**Medido, não suposto:** `Content/Maps/BattleScreen.umap` carrega `Floor_0`, um plano do template em **Z=-0.5**. A laje de andar foi desenhada com topo em **-4**, porque o pet fica em Z=0 e a laje precisa ficar logo abaixo do pé dele. Quatro unidades abaixo do chão do mapa é quatro unidades de invisibilidade.

**Por que nenhum teste pegaria:** o defeito não é asset faltando (esse já tem teste desde os pets invisíveis) nem lógica errada. É **profundidade** — duas geometrias corretas, uma na frente da outra. A bateria inteira fica verde.

**Como medi sem abrir o editor:** teste de automação descartável que faz `LoadPackage` do mapa, `UWorld::FindWorldInPackage`, e percorre `PersistentLevel->Actors` imprimindo nome, classe, localização e escala. Só Engine/CoreUObject — `UEditorLoadingAndSavingUtils` mora em `UnrealEd`, que o módulo de jogo não linka (e não deve linkar). Custou um ciclo de build e respondeu a pergunta exata.

**Correção:** `ABattleArena::BoardElevation` (120), aplicado com `AddActorWorldOffset` no `BeginPlay`. Erguer o ATOR, não as lajes: `GetCellWorldLocation` deriva da posição dele, então pets, grade desenhada e câmera sobem juntos. E a elevação mora na arena, não nos **três** pontos de spawn (`BattleScreenGameMode`, `BattleSquareGameMode`, `WorldBattleTransitionService`) — três cópias do mesmo número discordam na primeira edição, que é L-032 e L-033 de novo.

**Previne:** ao pôr geometria nova num nível existente, **medir o que já está lá** antes de escolher alturas. O teste que ficou (`Arena.SitsAboveLevelGround`) prende a folga: a casa mais funda, a água, tem que terminar acima de zero.


### L-044: a convenção de sinal do motor se MEDE, não se lê na tela

**Contexto:** em 2026-08-29 os adornos de tipo (orelha, chama, barbatana, folha) tombavam para o lado errado. Olhei a captura, concluí "estão deitando para dentro" e troquei o sinal do Roll para o que parecia óbvio. O teste caiu nos oito casos — e o número dizia o contrário do que eu tinha lido: base a 8.7 do meio, ponta a 0.7. O código original já estava certo.

**Medido:** `FRotator(0,0,Roll).RotateVector(FVector(0,0,h))` rende **`Y = +h·sen(Roll)`**. Como a tabela de tipos traz Roll negativo, abrir para fora exige `-LateralSign * Roll`. O sinal que "fica mais óbvio de ler" era o que deitava os dois para dentro.

**Por que a leitura visual falhou:** dois cones simétricos vistos em perspectiva de diorama são ambíguos — o de trás aparece menor e mais alto, e isso lê como inclinação. A tela mostra QUE está errado; ela não mostra PARA QUE LADO.

**Previne:** convenção de eixo, sinal e ordem de rotação sai de um teste que imprime o vetor, não de uma captura. É o mesmo L-035 (consertar por hipótese) num lugar novo: aqui a hipótese veio de um sentido que eu confiei demais.

### L-045: ramo que o DADO nunca exercita é ramo que nunca apareceu na tela

**Contexto:** `FPetAppearance::ForType` tinha uma tabela com cinco tipos (Fogo, Água, Planta, Cachorro, neutro), testada e chamada em produção. Em jogo, nunca se viu nenhuma diferença de tipo — e não por defeito de código.

**A causa:** o espelho de fixture tem seis pets, e só dois tipos: `Cat` (cinco) e `Dog` (um). `Dog` caía no ramo neutro, idêntico a gato. E a tela de batalha abria com os ids de catálogo VAZIOS, o que punha gato contra gato. Resultado: a tabela inteira era código sem uma única prova visual, com bateria verde o tempo todo.

**O que isso é:** o mesmo modo de falhar do "ator com componente mas sem malha atribuída" que o `CLAUDE.md` já registra três vezes — só que a peça que falta não é o asset, é o **dado que faria o ramo ser tomado**. Teste de unidade prova o ramo; ele não prova que alguém chega nele.

**O que fiz:** dei ao cachorro orelha caída e acento próprio, mapeei a forma nova para a malha, escrevi `TellsDogFromCat`, e fixei `PlayerCatalogId`/`OpponentCatalogId` no `DefaultGame.ini` para a tela abrir **gato contra cachorro**. Os outros três tipos seguem sem prova visual: não há pet deles no espelho.

**Previne:** ao fechar uma tabela de variação, perguntar **qual dado de produção cai em cada ramo**. Ramo sem dado é ramo que só existe no teste — e é assim que se descobre, um mês depois, que ele nunca funcionou.

### L-046: teste que some não faz a contagem cair

**Contexto:** ao fechar a fatia 1 dos atributos, a bateria deu **299 testes, 299 sucessos** — e os dois testes que eu tinha acabado de escrever não existiam nela. A contagem era **idêntica** à da rodada anterior.

**A causa:** o manifesto de módulo estava defasado (L-025), então o editor carregou o `.dylib` antigo. Nenhum teste falhou; os novos simplesmente nunca se registraram.

**Por que quase passou:** eu esperava que um teste faltando fizesse a contagem *cair*. Ela não cai — o conjunto que roda é o do binário velho, coerente consigo mesmo. Contagem estável é o que se vê quando o build novo não chegou, e é indistinguível de "nada mudou".

**O que fiz:** procurei os testes **pelo nome** em vez de conferir o total; sincronizei o manifesto; a bateria passou a 301.

**Previne:** depois de escrever teste novo, verificar que **o nome dele aparece no log** — o total não responde essa pergunta. E rodar `sync_module_manifest.sh` **antes** da bateria, não depois de suspeitar.

### L-047: o padrão vale para a camada errada, não só para o valor errado

**Contexto:** na fatia 4 (acaso no combate), pus o padrão de ±20% de variação de dano **dentro do `BattleSim`**, derivando a faixa da agressividade do atacante. Três testes caíram: `DirectionalAttackHits`, `MagicIgnoresDodge`, `DefendReducesMagicDamageToo`.

**O que eu quase fiz:** afrouxar os três para aceitar uma faixa em vez de um valor exato. Eles são justamente os que **fixam a fórmula de dano** — a coisa que `audit_no_recalculation.sh` existe para proteger.

**A causa real:** o padrão estava na camada errada. Todo `FPetState` construído à mão — ou seja, **todo estado de teste** — passou a ter dano imprevisível, porque o núcleo aplicava a variação a quem nunca pediu.

**O que fiz:** o núcleo passou a receber **porcentagens prontas** (`ReflexDodgePercent`, `DamageVariancePercent`), ambas nascendo em ZERO, que é ausência de acaso. O ±20% padrão mora na tradução do pet, junto de `MovePowers` e pelo mesmo princípio. Os tetos continuam recortados **dentro** do núcleo: amarra de jogo não é acordo entre camadas, e um estado vindo da rede não pode passar por cima dela.

**Previne:** quando um teste antigo cai por causa de um padrão novo, perguntar **de quem é esse padrão** antes de mudar o teste. Padrão que atinge quem não pediu está na camada errada — e enfraquecer o teste esconde isso em vez de resolver.

### L-048: gravação que monta o save do zero apaga a metade que não é dela

**Contexto:** o perfil do treinador (especialidades) foi para o MESMO `USaveGame` da coleção de pets — decisão certa, porque dois arquivos dessincronizam e "treinador sem coleção" é um estado que nada no jogo sabe interpretar.

**O defeito que isso quase criou:** `FPetCollectionService::SaveCollection` monta um `UPetCollectionSaveGame` **novo** e grava por cima do slot. Ela nunca precisou reler nada, porque o save só tinha pets. Com o treinador dentro, **cada ganho de experiência apagaria as especialidades** — uma escolha que não se refaz, sumindo sem nada indicar quando nem por quê.

**Por que nenhum teste pegaria:** os testes de treino gravam o treinador e releem o treinador; os de coleção gravam pets e releem pets. Cada metade passa sozinha. Só **cruzando** as duas gravações o defeito aparece.

**O que fiz:** as duas funções passaram a reler a metade que não é delas, e `SavingOneHalfKeepsTheOther` grava especialidade → grava coleção → confere que a especialidade sobreviveu, e o contrário.

**Previne:** ao acrescentar um campo a um `USaveGame` já existente, procurar **toda** função que monta esse save do zero. E ao juntar dois dados num arquivo, o teste que importa é o que **cruza** os caminhos de escrita — nunca os dois testes separados, que continuam verdes enquanto um apaga o outro.


### L-049: constante em unidades absolutas envelhece quando a ESCALA do mundo cresce

**Contexto:** o jogador disse que a floresta "desapareceu". Nada tinha sido apagado — a mata continuava sendo gerada, com a mesma densidade de plantas de sempre.

**O defeito:** `IslandGeography::HomeRadiusUnits()` devolvia **2600 unidades fixas**, um número escolhido quando a ilha tinha 6000 de raio. A ilha passou para 20000 e o número ficou. Como os setores de bioma são fatias de 72° que **se encontram no centro**, tudo que está fora do miolo de mata é outro bioma — e um pedaço de mundo tem 6400 de lado, ou seja, **nem um pedaço inteiro de mata cabia dentro do miolo**. O vizinho imediato de quem nasce era deserto.

**Por que passou:** todo teste do miolo media o miolo contra ele mesmo ("o centro é mata", "a praia fica fora"), e continuou verdinho. O raio nunca foi medido contra **o tamanho do pedaço de mundo**, que é a unidade em que o jogador anda.

**O que fiz:** o raio virou `max(piso, 35% do raio da ilha)`, preso pela praia — ele acompanha a ilha em vez de esperar alguém lembrar. E o teste passou a medir o que se vê: o pedaço em que se nasce **e os quatro vizinhos de lado**, que são o que está carregado no primeiro instante.

**Previne:** raio, distância ou clearance escrito em unidades absolutas se mede contra a constante de ESCALA a que ele pertence (o pedaço de mundo, o raio da ilha), nunca contra um número lembrado. E quando uma constante de escala do mundo mudar, varrer quem mais depende dela — foi a mesma causa dos anéis de relevo amontoados no miolo do mapa.


### L-050: EDITOR ABERTO faz a compilação mentir — o manifesto aponta para o .dylib velho

**Contexto:** um teste novo foi escrito, compilou, o build disse `Result: Succeeded`, e a suíte rodou **duas vezes** sem que o teste existisse. A contagem ficou exatamente na mesma de antes, o que parecia "nada mudou" em vez de "nada rodou".

**O mecanismo:** `Binaries/Mac/UnrealEditor.modules` é um JSON que diz **qual .dylib numerado** cada módulo carrega. Com o Editor aberto, ele segura o dylib atual; a UBT então **liga um número mais alto** (`-0016.dylib`) e deixa o manifesto apontando para o antigo (`-0015.dylib`), para não quebrar o Editor em execução. Todo teste roda contra código velho, e nada avisa.

**Como medir:** `cat Binaries/Mac/UnrealEditor.modules` e comparar com o `Link [Apple] libUnrealEditor-…-00NN.dylib` do log de build. Para conferir se um literal está no binário, `strings` **não serve**: `TEXT()` é UTF-16, e `strings` só lê ASCII. Procurar os bytes com `python3` e `.encode('utf-16-le')`.

**Previne:** fechar o Editor antes de compilar (a regra do `CLAUDE.md` já dizia, e é ESTE o preço de ignorá-la). E o gatilho: **nome de teste novo com zero ocorrência no log da suíte é binário velho até prova em contrário** — nunca "o teste não foi registrado".


### DP-mapa-01: quando a ilha crescer demais, o mapa separa por ÁREA

**Decidido em 30/08/2026, não construído.**

O mapa desenha um pedaço por casa da ilha inteira, e a contagem cresce com o
QUADRADO do raio: 6.400 pedaços a 20.000 de raio, 14.400 a 30.000, 57.600 a
60.000. O pedaço não pode crescer junto para compensar — ele está preso ao
tamanho da região de descoberta, e passar disso faz a fronteira do que se andou
mentir nos dois sentidos (ver o teste `PedacoCabeNaRegiaoDescoberta`).

**A saída é mapa por ÁREA**: o mapa passa a ter o tamanho do SETOR onde o
jogador está, e não do planeta. Isso limita a contagem por construção, em vez de
por ajuste de número — e é como jogos de mundo grande sempre resolveram.

A costura já existe: `IslandGeography::BiomeOfSector` divide a ilha. O dia da
separação não pede arquitetura nova, só o mapa parar de olhar o mundo inteiro de
uma vez.

**Por que não foi construído:** com a ilha em 20.000 não é preciso, e feature
especulativa envelhece mal — ela é escrita contra um problema imaginado e
descobre-se errada quando o problema chega de verdade.

**O que FOI construído é o gatilho.** O teste
`BattleSquare.World.Map.QuandoAIlhaCrescerDemaisSepararPorAreas` falha quando a
ilha configurada passa de 10.000 pedaços, e a mensagem dele diz que não é
defeito — é a hora. Verificado que dispara: com a ilha em 30.000 ele acusa
14.400 pedaços e reprova.

Plano que depende de alguém lembrar é plano que ninguém executa, e este projeto
já pagou por isso mais de uma vez.

### B-010: os cubos do teste de streaming — RESOLVIDO em 30/08/2026

Os 216 cubos e os 36 atores de HLOD gerados a partir deles foram apagados do
`WorldStreamingTest`. Sobraram 41 atores: os 25 pisos, a esfera de céu, o
PlayerStart, o explorador, os quatro encontros, as luzes e o landscape.

O caminho foi pelos DESCRITORES do World Partition, e não pelo World Outliner.
Num commandlet as células não carregam, então `get_all_level_actors` devolve
dez atores e destruir por ali não apaga nada; e como cada ator é um arquivo
(OFPA), remover de verdade é remover o pacote dele. Os descritores dão rótulo,
classe e caminho de pacote de todos os 293 — precisão em vez de palpite.

**E o paliativo era NOCIVO, não só morto.** `RemoveStreamingTestCubes` varria
`AStaticMeshActor` com a malha `/Engine/BasicShapes/Cube`, e eu documentei essa
assinatura como "estreita de propósito". Ela não era: **os 25 pisos usam essa
mesma malha**. O paliativo apagava o chão a cada Play, e o guarda-queda do
jogador escondia o sintoma. Só apareceu porque o usuário perguntou por que os
blocos continuavam na tela ANTES do Play — a pergunta era sobre o editor, e a
resposta estava no runtime.

A lição não é sobre cubos: **"assinatura estreita" foi afirmação minha, sem
medição.** Bastava listar o que ela casava antes de confiar nela.

