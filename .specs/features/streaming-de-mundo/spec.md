# Streaming de Mundo — Especificação

**Status:** Concluída e verificada por teste (status corrigido em 29/08/2026 — a linha dizia "Draft" enquanto a feature já rodava)
**Depende de:** M1–M4 concluídos, gate B-001 (go/no-go de M5) decidido pelo usuário. Primeira feature de M5 — ver `.specs/project/OBJECTIVE.md`.

---

## Problem Statement

Tudo que existe hoje (M1–M4) roda dentro de um `UWorld` pequeno, sempre presente por inteiro na memória: a arena de batalha (`ABattleArena`) é um ator spawnado sob demanda, sem nenhuma noção de "mundo maior ao redor". Não existe ainda nenhum nível persistente, nenhuma malha de streaming, nenhum conceito de "onde o jogador está" fora de uma partida.

**Isto é o salto de escopo que o roadmap já nomeia (AD-001):** a razão inteira de a Unreal ter sido escolhida em vez de Cocos Creator foi o mundo aberto contínuo sem loading — e até aqui, nenhuma linha de código usou essa capacidade. Esta feature é a primeira a de fato abrir um `UWorld` com **World Partition** habilitado e provar que ele carrega/descarrega região por região, sem tela de loading, num orçamento de memória e frame time que ainda precisa ser definido (ver Todos pendente em `STATE.md`, linha 418).

**Diferença estrutural em relação a tudo que veio antes:** M1–M4 são inteiramente sobre `BattleSim`/`BattleSquare` — lógica de jogo determinística e sua apresentação. Esta feature é a primeira sobre **o motor de mundo em si** (World Partition, streaming de nível, HLOD). Não há núcleo determinístico a proteger aqui — o mundo não precisa ser bit-exato entre cliente e servidor, porque (por ora) ele é só cenário: a batalha continua sendo a única coisa que precisa de determinismo (AD-004), e o mundo não participa da resolução de combate.

## Goals

- [ ] Um nível único, com World Partition habilitado, cobre uma área substancialmente maior do que cabe confortavelmente em memória de uma vez (área de referência: a decidir em design.md, ancorada num orçamento de memória explícito)
- [ ] O jogador atravessa esse nível sem nenhuma tela de carregamento — streaming de céu (cells) entra e sai com base na posição do jogador, de forma automática
- [ ] Existe um orçamento de performance nomeado (memória de streaming, frame time alvo) documentado ANTES de qualquer decisão de tamanho de mundo ou densidade de conteúdo — precondição já registrada como Todo pendente em `STATE.md`
- [ ] HLOD (ou equivalente) reduz o custo de renderizar regiões distantes, sem exigir autoria manual de LODs por asset

## Out of Scope

| Item | Razão |
|---|---|
| Conteúdo real do mundo (terreno autoral, vegetação, biomas) | Esta feature prova a INFRAESTRUTURA de streaming com geometria de placeholder — autoria de conteúdo é trabalho de arte/design separado, consumidor desta infraestrutura |
| Encontros de pets no mundo (spawn, gatilho de batalha) | Feature seguinte de M5 ("Encontros e Transição para Batalha") — depende desta, mas não é esta |
| Movimentação do personagem e câmera de mundo (traversal) | Terceira feature de M5 ("Traversal e Câmera de Mundo") — esta feature só precisa de um pawn mínimo de debug para provar o streaming, não do sistema de traversal final |
| Multiplayer no mundo aberto (replicação de posição entre jogadores fora de batalha) | M2 resolveu rede só para a batalha (`ABattleArena`/`UBattleTurnCoordinator`); replicar exploração de mundo aberto é escopo novo, não assumido aqui |
| Mobile real (device físico, orçamento validado em hardware) | M6 (Plataformas) — esta feature define o orçamento de PC como baseline; validar em mobile de verdade é marco separado |
| Dedicated Server hospedando o mundo aberto | B-004 já registra que a engine instalada não compila `TargetType.Server` — mundo aberto em servidor dedicado herda o mesmo blocker, não é resolvido aqui |

---

## User Stories

### P1: Nível com World Partition carrega sem tela de loading ⭐ MVP

**User Story:** Como jogador, quero atravessar uma área grande do mundo sem nunca ver uma tela de carregamento, para que a promessa de "mundo contínuo" seja real, não só uma frase no roadmap.

**Acceptance Criteria:**
1. WHEN o jogador se move de uma extremidade a outra da área de referência do nível THEN o sistema SHALL manter o jogo rodando sem nenhum `LoadStreamLevel`/transição de nível bloqueante — streaming acontece via World Partition, células entrando/saindo em segundo plano
2. WHEN uma célula de World Partition sai do raio de streaming do jogador THEN o sistema SHALL descarregá-la da memória, mensurável por uma queda de uso de memória correspondente
3. WHEN o jogador reverte o caminho (volta para uma célula já visitada e descarregada) THEN o sistema SHALL recarregá-la corretamente, sem estado corrompido ou atores duplicados

**Independent Test:** um pawn de debug percorre uma rota fixa e determinística pelo nível (waypoints), e uma métrica de memória/contagem de atores carregados é registrada nos extremos da rota, provando que células fora do raio de streaming não permanecem carregadas.

---

### P1: Orçamento de performance nomeado antes de qualquer decisão de escala ⭐ MVP

**User Story:** Como equipe de desenvolvimento, quero um orçamento de memória e frame-time documentado antes de desenhar o tamanho do mundo, para que decisões de streaming não sejam refeitas depois de já serem caras de reverter.

**Acceptance Criteria:**
1. WHEN esta feature é especificada em design.md THEN o sistema SHALL registrar um orçamento explícito (memória de streaming em MB, frame time alvo em ms, plataforma de referência) como parte da decisão de design, não como nota solta
2. WHEN o orçamento é definido THEN o sistema SHALL derivar dele o raio de streaming e o tamanho de célula de World Partition usados na área de referência — não o contrário

**Independent Test:** o valor do orçamento aparece em design.md como decisão nomeada (mesmo padrão de AD-004, AD-005), com raio de streaming e tamanho de célula derivados dele de forma auditável.

---

### P2: HLOD reduz custo de regiões distantes

**User Story:** Como jogador, quero que regiões distantes do mundo apareçam sem exigir o custo total de renderização delas, para que a área visível ao mesmo tempo possa ser grande sem destruir o frame rate.

**Why P2:** o MVP (P1) já prova que o streaming funciona; HLOD é uma otimização sobre isso, necessária para densidade de conteúdo real, mas não bloqueia a prova de conceito do streaming em si.

**Acceptance Criteria:**
1. WHEN uma região do mundo está além de uma distância configurada THEN o sistema SHALL renderizá-la através de uma representação de custo reduzido (HLOD), gerada automaticamente a partir do conteúdo da região

---

## Edge Cases

- WHEN o jogador se move rápido o suficiente para ultrapassar o raio de pré-carregamento de uma célula THEN o sistema SHALL expor essa condição de forma mensurável (ex.: pop-in registrado em log/métrica) em vez de mascarar silenciosamente — decisão de como tratar isso (aumentar raio, limitar velocidade do pawn de debug) fica para design.md
- WHEN duas células adjacentes têm um ator que atravessa a borda entre elas THEN o sistema SHALL manter esse ator coerente (sem duplicação, sem desaparecimento) enquanto ele cruza a fronteira
- WHEN o orçamento de memória é excedido durante o teste da rota de referência THEN o sistema SHALL falhar a verificação explicitamente (métrica documentada, não "pareceu OK") — nunca reportar sucesso sem medir

---

## Decision Points (para o Design)

- **DP-streaming-01 (a mais pesada):** o orçamento de performance em si — memória de streaming (MB), frame time alvo (ms), plataforma de referência (PC primeiro, por AD-001/M6). Precisa ser um número, não uma intenção.
- **DP-streaming-02:** tamanho de célula de World Partition e raio de streaming, derivados de DP-streaming-01 e da área de referência escolhida.
- **DP-streaming-03:** geometria/conteúdo de placeholder usado para provar o streaming (grade de blocos, terreno mínimo, ou landscape básico) — decisão de custo vs. representatividade do teste.
- **DP-streaming-04:** como o pawn de debug se move pela rota de teste (script determinístico de waypoints vs. input manual) — impacta se o Independent Test de P1 pode rodar headless/automatizado ou exige verificação manual no editor.
- **DP-streaming-05:** critério exato de "sem tela de loading" verificável por teste automatizado vs. só por roteiro manual (`docs/verification/`) — Unreal não expõe métricas de streaming da mesma forma que expõe automação de gameplay puro; pode ser que parte desta feature só seja verificável manualmente, e isso precisa ficar dito com todas as letras, não fingido como testável.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| STREAM-01 | P1: Streaming sem tela de loading ao atravessar o nível | Specify | Pending |
| STREAM-02 | P1: Células fora do raio são descarregadas, mensurável | Specify | Pending |
| STREAM-03 | P1: Recarregamento correto ao revisitar célula | Specify | Pending |
| STREAM-04 | P1: Orçamento de performance nomeado e documentado | Specify | Pending |
| STREAM-05 | P1: Raio/tamanho de célula derivados do orçamento | Specify | Pending |
| STREAM-06 | P2: HLOD reduz custo de regiões distantes | Specify | Pending |

**Cobertura:** 6 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Um orçamento de performance nomeado (memória, frame time, plataforma de referência) existe em design.md antes de qualquer número de tamanho de mundo ser decidido
- [ ] Um pawn de debug atravessa a rota de referência do nível sem nenhuma tela de carregamento, com memória de streaming medida nos extremos provando descarregamento de células distantes
- [ ] Revisitar uma célula já descarregada não produz estado corrompido nem atores duplicados
- [ ] HLOD está habilitado e mensuravelmente reduz o custo de renderizar regiões distantes (P2)
- [ ] Nenhuma regressão nos módulos existentes — `BattleSim`/`BattleSquare` continuam 127/127, já que esta feature não toca neles (mundo e batalha permanecem sistemas separados, batalha ainda instanciada sob demanda)
