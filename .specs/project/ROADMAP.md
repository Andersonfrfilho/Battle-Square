# Roadmap

**Marco atual:** roadmap percorrido até onde esta máquina permite (2026-08-26). M1–M5 e M7 concluídos; **M6 é o único incompleto**, e por bloqueio de infraestrutura, não de código — ver B-006/B-006b/B-007 (nenhuma plataforma móvel compila aqui) e B-008 (console exige licença e devkit).
**Status:** M1, M2, M3 e M4 concluídos
**Objetivo de fechamento do roadmap inteiro (M3→M7):** `.specs/project/OBJECTIVE.md` — ordem, dependências entre marcos, gates (B-001, B-004) e critério de "pronto" do projeto completo.

---

## M1 — Fatia Vertical do Combate ✅ CONCLUÍDO

**Objetivo:** um combate 1v1 offline, jogável do início ao fim, com as três ações e a resolução simultânea funcionando. É o que prova que o jogo é divertido antes de qualquer investimento em rede, conteúdo ou mundo.

**Critério de conclusão:** dá para jogar uma partida completa contra uma IA burra e sentir a tensão do commit às cegas. ✅ Atingido — `ABattleArena` local (Standalone) resolve turnos completos contra `FDumbOpponentAI`.

### Features

**Núcleo de Combate** — ✅ CONCLUÍDO (`.specs/features/combate-nucleo/`)
- Estado de batalha, grade 3x3, ocupação de casas
- Fila de 3 ações por pet, commit e revelação
- Resolução simultânea por slot, em fases
- Movimento, ataque, magia, defesa, esquiva
- Condição de vitória, derrota e empate

**Apresentação do Combate** — ✅ CONCLUÍDO (`.specs/features/apresentacao-combate/`)
- Consumo do trace de eventos para animar o resultado
- Câmera de arena, seleção de ações, indicação de limite (3/3)
- Feedback de vida, dano e ação inválida
- Layout visual final (UMG) e verificação estética ficam para autoria posterior — ver `docs/verification/apresentacao-combate-visual.md`

**Backend de Dados de Pet** — ✅ CONCLUÍDO (`.specs/features/backend-dados-pet/`)
- Backend próprio (Bun + PostgreSQL + Drizzle), API REST `/v1/pets`, CRUD
- Espelho local sincronizado no servidor de combate — batalha nunca lê a rede diretamente (AD-014)
- 6 a 10 pets de teste cadastrados via API

---

## M2 — Combate Online ✅ CONCLUÍDO

**Objetivo:** o mesmo combate, autoritativo no servidor, entre dois jogadores reais.

### Features

**Servidor Autoritativo** — ✅ CONCLUÍDO (`.specs/features/combate-online/`)
- Simulação rodando atrás de `HasAuthority()` — mesmo código para `Standalone`/`ListenServer`/`DedicatedServer` (NetMode decide, não uma flag)
- Cliente envia apenas o commit das 3 ações (`FNetTurnCommit`, tipo de fio dedicado); servidor devolve estado + trace
- Timeout de commit e preenchimento automático (`UBattleTurnCoordinator`)
- Reconexão por estado completo, sem replay de trace (DP-online-03)
- **Ressalva registrada (B-004):** verificação com `DedicatedServer` real bloqueada — a engine instalada (Epic Games Launcher) não compila `TargetType.Server`. Não é dívida de código; é decisão de infraestrutura para quando M2 for a produção de verdade. `ListenServer` cobre desenvolvimento sem essa restrição.

**Sala e Pareamento Simples** — ✅ CONCLUÍDO (`.specs/features/sala-e-pareamento/`)
- Código de sala (5 caracteres, sem `0`/`O`/`1`/`I`), lobby de dois jogadores
- Tratamento de abandono (timeout injetável, vitória por `BatalhaEncerrada` já existente)
- Reconexão por segredo de sessão (`FGuid`), sem conta de jogador

---

## M3 — Conteúdo e Balanceamento ✅ CONCLUÍDO

**Objetivo:** provar que o sistema aguenta escala de conteúdo sem código novo por pet.

### Features

**Escala de Pets e Skills** — ✅ CONCLUÍDO (`.specs/features/escala-pets-skills/`)
- Tipos, fraquezas e resistências por dados (`Config/TypeEffectiveness.json`, sem recompilar para adicionar tipo novo)
- Ferramenta de balanceamento e simulação em lote (`FBattleBalanceSimulator`, determinística por seed, headless)
- **Achado relevante no processo:** `BatalhaEncerrada` nunca disparava em produção desde M1 (`BattleOutcome::EvaluateOutcome` nunca era chamada fora dos testes do próprio núcleo) — corrigido, ver L-019 em `STATE.md`

**Arenas Variadas** — ✅ CONCLUÍDO (`.specs/features/arenas-variadas/`)
- Casas com propriedades (bloqueio, dano, buff) — layout viaja dentro de `FBattleState`, entra no hash de dessincronia
- **Achados relevantes no processo:** um bug de lógica próprio (dano de casa não aplicava quando nenhum lado tentava mover) e uma lição de processo (`Tools/probe_isolation.sh` deixa o binário quebrado até rebuildar de novo — ver L-020 em `STATE.md`)

**M3 concluído — as duas features planejadas estão prontas.**

---

## M4 — Progressão e Meta ✅ CONCLUÍDO

**Objetivo:** razão para voltar amanhã.

### Features

**Coleção e Captura** — ✅ CONCLUÍDO (`.specs/features/colecao-e-captura/`)
- Vitória em batalha contra pet de catálogo não possuído captura ele (decisão de produto, dado que M5/Mundo Aberto ainda não existe)
- Coleção local (`USaveGame`), independente de conta (M7 ainda não existe)
- **Achado relevante:** `Tools/audit_no_recalculation.sh` tinha escopo grande demais, sinalizava cálculo legítimo de montagem de partida como violação — corrigido, ver L-021 em `STATE.md`

**Níveis, Experiência e Evolução** — ✅ CONCLUÍDO (`.specs/features/niveis-experiencia-evolucao/`)
- XP concedida ao pet do jogador ao fim da batalha (vitória/derrota/empate com valores distintos), só se ele já estiver na coleção — funciona sem seleção de time
- Nível sempre derivado de `Experience` (nunca campo próprio); bônus de atributo aplicado na montagem da partida, antes do `BattleSim` ver o pet
- "Evolução" = crescimento de atributo por nível no mesmo `CatalogId`, não troca de forma (catálogo não tem cadeia evolutiva — fora de escopo)
- **Achado relevante:** mesmo falso positivo de L-021 reapareceu em `Meta/` assim que ele passou a fazer montagem de partida — corrigido, ver L-022 em `STATE.md`

**M4 concluído — as duas features planejadas estão prontas. Bateria final: 127/127 testes (75 BattleSquare + 52 BattleSim), três sondas de arquitetura limpas.**

---

## M5 — Mundo Aberto Contínuo — ✅ CONCLUÍDO (2026-08-26)

**Objetivo:** o salto de escopo. Mundo contínuo com World Partition, exploração, encontros e batalhas instanciadas a partir do mundo.

**Pré-requisito inegociável:** M1 a M3 concluídos e o combate validado como divertido. Começar o mundo antes disso é o cenário de não entregar nenhum dos dois.

### Features

**Streaming de Mundo** — ✅ CONCLUÍDO (2026-08-26)
**Encontros e Transição para Batalha** — ✅ CONCLUÍDO (2026-08-26)
**Traversal e Câmera de Mundo** — ✅ CONCLUÍDO (2026-08-26)

---

## Fora de marco — Interface de Batalha ✅ CONCLUÍDO (2026-08-26)

Paga a dívida de DP-08 (`apresentacao-combate`): o combate estava pronto e
testado desde M1 e ninguém conseguia jogá-lo, porque o layout UMG tinha sido
adiado para uma autoria visual que nunca aconteceu. Entrega a tela de ações e o
nível `BattleScreen`, que abre uma batalha sem depender do mundo aberto.

---

## Fora de marco — dívida de M1 paga em 2026-08-27

**Interface de Batalha** — ✅ CONCLUÍDO. O combate existia e era testado desde
M1, mas era INJOGÁVEL: DP-08 adiou o layout para uma autoria visual que nunca
veio. Agora se joga por tela, sem depender do mundo aberto.

---

## M6 — Plataformas — ⚠️ INCOMPLETO (bloqueado por infraestrutura)

**Objetivo:** sair do PC.

### Features

**Mobile** — ⚠️ PARCIAL (2026-08-26) — entregue tudo que não depende de aparelho (orçamento de performance, entrada de toque, escalabilidade). Compilar/empacotar/medir está BLOQUEADO por B-006/B-006b/B-007.
**Console** — ⛔ BLOQUEADO (B-008) — exige licença de desenvolvedor e devkit; sem spec de propósito, escrever uma para um SDK que não se pode ler produziria ficção.

---

## M7 — Contas e Moderação — ✅ CONCLUÍDO (2026-08-26)

**Objetivo:** identidade de jogador persistente entre partidas, pré-requisito para qualquer consequência que precise sobreviver ao fim de uma sessão — banimento, histórico, reputação, progressão de conta.

**Dependência registrada:** AD-017 (backend de dados de pet) já prevê a trilha de auditoria de adulteração de cache no Nível 1 (sessão); banimento persistente (Nível 2) só é implementável quando este marco existir. Não é bloqueante para nenhum marco anterior — é consumidor do que eles já produzem.

### Features

**Conta de Jogador** — ✅ CONCLUÍDO (2026-08-26) — serviço e contrato entregues; cliente C++ e sincronização de coleção ficaram fora de escopo por decisão da spec.
**Moderação e Banimento** — ✅ CONCLUÍDO (2026-08-26) — o Nível 2 que AD-017 prometeu. Cliente C++ reportando adulteração ficou fora de escopo por decisão da spec.

---

## M8 — Atributos e Treino (2026-08-29)

Nasceu de uma correção do usuário: *"todos os pets podem evoluir skills e
personalidade e musculatura que ficam gravados como atributos, isso desbloqueia
a escolha de ataques e magias que dependem desses atributos"*.

**Tamanho de campo variável** — ✅ CONCLUÍDO (`55d4685`) — a grade sai do 3x3 fixo, entra no hash, e a casa inicial passa a ser DERIVADA (lado 0 na primeira coluna, lado 1 na última, ambos na linha do meio).
**Desbloqueio de golpes** — ✅ CONCLUÍDO, quatro fatias — atributos que crescem e aparecem; requisito assinado no golpe; golpe trancado com o requisito na tela; e atributo alto mudando a resolução (esquiva por reflexo, golpe constante).
**Mundo com cara de lugar** — ✅ CONCLUÍDO (`60834f3`) — sol e mata no mundo aberto, reusando o cenário da arena; painel do mundo com pet, atributos e distância do adversário mais próximo.
**Campos de treino** — ✅ CONCLUÍDO (`0e4ec04`) — cinco clareiras coloridas, uma por atributo, que dão DESTINO ao mapa.
**Estudo do dono** — ✅ CONCLUÍDO — o treinador tem especialidades, limitadas a duas de cinco, que multiplicam o rendimento do treino sem substituir o pet.

**Duas decisões seguem do usuário, e estão marcadas na spec:** se o treino
avança offline (DP-atr-10, hoje "só com o jogo aberto"), e se dá para trocar de
especialidade (DP-atr-11, hoje não).

---

## Considerações Futuras

- Pixel Streaming como demo jogável por link (não como canal de distribuição)
- Modo de batalha avulso em navegador, se a batalha for separável do mundo
- Espectador e replay a partir do trace de eventos — o formato já permite de graça

---

## Fila de ideias — levantadas pelo usuário, aguardando decisão

Não bloqueiam nada em andamento. Cada uma tem spec com as perguntas que
precisam de resposta antes de virar código.

| Ideia | Spec | Por que ainda não virou código |
|---|---|---|
| **Clima no campo** | `.specs/features/clima-no-campo/` | O núcleo não pode consultar clima durante a batalha — replay e rede divergiriam. Precisa entrar como parâmetro da montagem, e há 9 perguntas em aberto (fonte, offline, justiça em ranqueada) |
| ~~**Desbloqueio de golpes**~~ | `.specs/features/desbloqueio-de-golpes/` | ✅ **CONCLUÍDO em 2026-08-29**, quatro fatias (`57f2587`, `7e7c6a3`, `1dce9d9`, `5b8782e`). O impasse se resolveu pelo requisito viajar DENTRO da assinatura: quem decide o desbloqueio não decide mais o próprio dano |
| **Captura, roubo e treinador** | `.specs/features/captura-roubo-e-treinador/` | O jogo não tem o conceito de DONO. Sem ele, "sem dono", "selvagem" e "roubar" não têm como ser distinguidos |
