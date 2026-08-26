# State

**Última atualização:** 2026-08-25
**Trabalho atual:** **Arenas Variadas (M3)** — `spec.md`/`design.md`/`tasks.md` escritos, **feature completa (T1–T9)**. Diferente da feature anterior, esta TOCA `BattleSim` de verdade: `ECellProperty`/`CellLayout` (9 posições) viajam DENTRO de `FBattleState` (mesma razão de `FBattleRandom`, AD-004) e entram no `ComputeHash()`. Bloqueio reaproveita `MovimentoBloqueado` (zero evento novo); dano de casa soma em `PendingDamage` sem evento próprio (F5 já cobre — inclusive morte simultânea combinando dano de casa e combate, testada); buff é contextual (Attack ao atacar, Defense ao defender), nunca persiste em `FPetState`. `BattleSim` nunca importa `Json` — `FArenaLayoutCatalog` (nomeação/autoria) fica inteiramente em `BattleSquare`, mesmo padrão de `FTypeEffectivenessTable`. `ABattleArena::BeginBattle` agora retorna `bool`, rejeitando montagem com pet em casa bloqueada. **Dois bugs reais encontrados e corrigidos no processo:** (1) meu próprio bug de lógica — o early-return de `ApplyMovement` pulava o cálculo de dano de casa quando ninguém tentava mover; pego pelos próprios testes, corrigido reestruturando sem early-return. (2) **L-020**: `Tools/probe_isolation.sh` deixa o `.dylib` de `BattleSim` quebrado depois de rodar (planta sonda, espera falhar, remove, nunca recompila de volta) — 3 tentativas de teste falharam com "módulo não encontrado" até a causa ser isolada; disciplina nova: sempre rebuildar depois de qualquer sonda que planta código. Bateria final: **66/66 BattleSquare, 52/52 BattleSim, `audit_determinism`/`probe_isolation` limpos.** **M3 completo — as duas features planejadas (Escala de Pets e Skills, Arenas Variadas) estão concluídas.** Efetividade de tipo resolvida INTEIRAMENTE fora de `BattleSim` (DP-escala-01, design.md) — `Attack` chega pré-multiplicado ao núcleo, zero linhas mudadas em `BattleSim` pela feature em si. `FTypeEffectivenessTable` (JSON local, `Config/TypeEffectiveness.json`), `TranslateMatchup` (🧠, testado com caso assimétrico), `FBattleBalanceSimulator::RunBatchSimulation` (determinístico por seed, prova com 200 simulações que tipo super efetivo vence mais). **Bug real de produção encontrado e corrigido — ver L-019**: `BatalhaEncerrada` nunca disparava em M1/M2 porque nada chamava `BattleOutcome::EvaluateOutcome` fora dos testes isolados do núcleo; corrigido em `ABattleArena`, `UBattleTurnCoordinator` e no simulador novo. `BattleOutcome::EvaluateOutcome` também não tinha `BATTLESIM_API` (mesmo defeito de L-016, 2ª ocorrência). Bateria final: **64/64 BattleSquare, 44/44 BattleSim, `audit_determinism`/`audit_no_recalculation` limpos.** **M3 (primeira feature) encerrada.** Fases 1–2 (T1–T6): `UBattleRoomRegistry` — código único entre salas vivas, `CreateRoom`/`JoinRoom` distinguindo `NotFound`/`Full`, desconexão/reconexão por segredo (`FGuid`), timeout de abandono (🧠 T5), sala vazia destruída com código reciclável. Fase 3 (T7–T9): `ABattleSquareGameMode` (RoomRegistry, `Tick` chamando `CheckAbandonment`/`CheckEmptyRooms` 1x/s) + `ABattleSquarePlayerController` (RPCs finos) + `AssembleMatchForRoom` (monta `ABattleArena`+`UBattleTurnCoordinator` reais, testável sem tocar no espelho de pets) + `HandleRoomReady` (caminho de produção, carrega pets reais via `FPetDataLoader`). **Um SIGSEGV real foi encontrado e corrigido nesta fase — ver L-018**: `BeginPlay` não dispara de forma confiável num `UWorld` de teste montado à mão; `RoomRegistry` virou inicialização preguiçosa (`EnsureRoomRegistry()`, mesmo padrão de `ABattleArena::TracePlayer`). Fase 4 (T10–T11): regressão completa e extensão de `docs/verification/combate-online-rede.md` com os itens de sala que só rede real prova (Itens 4–5). Bateria final: **59/59 BattleSquare, 44/44 BattleSim, Fail: 0**. **Feature Sala e Pareamento Simples encerrada — M2 completo.**

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

## Ideias Adiadas

- [ ] `apps/worker-pet-sync` usa o prefixo `worker-` (code-standart.md §2), mas as rules globais de Worker (`backend/worker.md`) descrevem especificamente consumidor RabbitMQ com ack manual/DLQ/prefetch — o sidecar de sync é um poller de intervalo (`setInterval`), não consome fila nenhuma. Nenhuma das três categorias documentadas (api/worker/cron) descreve exatamente esse padrão ("processo de fundo que se auto-agenda via poll HTTP periódico, sem fila e sem gatilho externo"). Não bloqueia nada — só fica registrado para quando as rules globais de backend forem revisadas, se fizer sentido nomear essa categoria explicitamente. Capturado em: 2026-08-25, Fase 5 do Backend de Dados de Pet.

- [ ] Criar um arquivo de **rules de desenvolvimento de jogos** (`~/.claude/rules/rules/games/unreal.md` ou similar), no mesmo espírito de `code-standart.md`, `frontend/web.md`, `apis.md` — nomenclatura C++/Blueprint, convenção de módulo, padrão de fronteira núcleo-vs-engine (o que este projeto já praticou em AD-011/AD-012), convenção de teste headless, checklist de auditoria de determinismo. Motivo: o usuário já tem rules globais para outros tipos de app (Bun/API, frontend web, banco de dados); faltam para jogos, e este projeto acumulou padrões reais que valeriam virar rule reaproveitável em outros projetos Unreal. Capturado em: 2026-08-25, durante o Backend de Dados de Pet.

- [ ] Espectador e replay a partir do trace de eventos — sai quase de graça do AD-005. Capturado em: Specify do Combate Núcleo.
- [ ] Simulação headless em lote para balanceamento (rodar 10.000 combates e medir taxa de vitória). Capturado em: Specify do Combate Núcleo.
- [ ] Modo batalha avulso em navegador, se a batalha for separável do mundo. Capturado em: decisão de motor.

---

## Todos

- [ ] Decidir as áreas cinzentas marcadas como PROPOSTA na spec do Combate Núcleo (ver seção "Decisões Pendentes")
- [ ] Definir orçamento de performance de mobile antes de M5 — decisões de mundo tomadas sem ele são irreversíveis
- [x] Avaliar MCPs de controle do Unreal Editor — **resolvido em 2026-08-24**: existe plugin **oficial da Epic** (`EpicGames/unreal-engine-skills-for-claude-code-plugin`, v3.0.4, MIT, autor Thomas Mansencal/Epic), publicado no marketplace oficial da Anthropic. Descarta a necessidade de MCP comunitário. **Instalado e habilitado em 2026-08-24** (escopo de usuário, v3.0.4, 3 skills: `unreal-mcp`, `create-toolset`, `unreal-skill`; custo fixo ~1.3k tokens por sessão). Fica inerte até o editor subir.
      Exige Unreal Editor rodando com os plugins `ModelContextProtocol` e `AllToolsets`, e `ModelContextProtocol.StartServer` no console.
      **Cuidado registrado pelo próprio README da Epic:** `ProgrammaticToolset.execute_tool_script` executa Python arbitrário dentro do editor, com acesso total ao projeto em disco. Nunca usar com `--dangerously-skip-permissions`, e commitar antes de sessão longa dirigida por MCP.
- [ ] Reavaliar GAS em M3 — ver AD-007

---

## Preferences

**Model Guidance Shown:** 2026-08-25 (troca de modelo por fase confirmada pelo usuário via /model sonnet)
