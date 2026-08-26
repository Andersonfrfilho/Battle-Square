# Coleção e Captura — Especificação

**Status:** Draft — aguarda aprovação
**Depende de:** Combate Núcleo, Backend de Dados de Pet, Combate Online (todos concluídos). Primeira feature de M4 — ver `.specs/project/OBJECTIVE.md`.

---

## Problem Statement

Hoje "ter um pet" não existe como conceito — `FPetDataLoader::LoadVerifiedPets` lê o catálogo inteiro do backend, e qualquer partida usa pets dele diretamente. Não há distinção entre "o catálogo de pets que existem no jogo" e "os pets que UM jogador específico tem". Sem essa distinção, não existe coleção, não existe progressão pessoal, e "razão para voltar amanhã" (`OBJECTIVE.md`) não tem onde morar.

**A decisão de produto que já foi tomada (2026-08-26):** sem Mundo Aberto ainda (M5), captura é disparada por **vitória em batalha contra um pet que o jogador ainda não tem** — reaproveita o combate PvP já existente, sem inventar mecanismo novo. Isso é combustível para trocar a FONTE do encontro por exploração de mundo quando M5 chegar, sem tocar no sistema de coleção em si.

**A segunda decisão que esta spec precisa deixar clara, porque `Contas de Jogador` (M7) ainda não existe:** coleção é **local, não de conta**. Cada instalação do jogo guarda sua própria coleção num save local (SaveGame da Unreal) — não sincronizada entre dispositivos, não protegida contra perda de dados do dispositivo. Isso é consciente e temporário: o dia em que M7 existir, a coleção local vira o que sincroniza para a nuvem, não é reescrita do zero.

**Terceira decisão que precisa ficar explícita:** o catálogo do backend (`apps/api-battle-pets`) hoje modela **pets individuais** (cada linha tem nome próprio, ex.: "Fluffy"), não espécies abstratas com múltiplos indivíduos possíveis. "Capturar uma espécie que o jogador ainda não tem" nesta spec significa **capturar um registro de catálogo (por `id`) que o jogador ainda não possui uma instância dele** — não existe conceito de "espécie com vários indivíduos" no backend hoje, e esta feature não o inventa.

## Goals

- [ ] Vencer uma batalha contra um pet do catálogo que o jogador ainda não possui dá a ele uma instância própria daquele pet, adicionada à coleção local
- [ ] A coleção sobrevive ao fechamento do jogo — é um save local, não estado de sessão
- [ ] Uma instância capturada é distinta do registro de catálogo — tem progressão própria (nível/XP, feature seguinte), mesmo que hoje comece idêntica às estatísticas de catálogo
- [ ] O jogador pode ver sua coleção (lista de pets capturados) fora de uma partida
- [ ] Capturar o mesmo pet de catálogo (mesmo `id`) de novo não duplica a instância — o jogador já tem, vitória seguinte contra ele não faz nada de novo (ou dá outra recompensa, a decidir em design.md — mas nunca uma segunda instância do mesmo catálogo)

## Out of Scope

| Item | Razão |
|---|---|
| Sincronização entre dispositivos / nuvem | Depende de M7 (Contas de Jogador) — coleção local é o piso consciente até lá |
| Taxa de captura menor que 100% (chance de falhar) | Mecanismo de "captura garantida ao vencer" é o MVP; probabilidade/itens de captura são refinamento de produto, não bloqueiam a feature existir |
| Múltiplos indivíduos do mesmo registro de catálogo (ex.: dois "Fluffy" diferentes) | O backend não modela isso hoje (Problem Statement) — inventar essa camada é escopo de uma feature de conteúdo maior, não desta |
| Seleção de time para batalha (escolher quais pets da coleção levar) | Fora de escopo desde a spec da Apresentação do Combate — continua fora aqui; hoje a montagem de partida ainda usa os 2 primeiros pets do catálogo (Escala de Pets e Skills), não a coleção do jogador |
| UI final de visualização da coleção (grade visual, fontes, animação) | Mesma separação lógica/visual de sempre — esta spec cobre o C++ testável; UMG é autoria posterior |
| Troca ou comércio de pets entre jogadores | Não é mencionado no roadmap, e depende de contas (M7) para ter sentido (trocar com QUEM, de forma rastreável) |

---

## User Stories

### P1: Vencer contra um pet novo captura ele ⭐ MVP

**User Story:** Como jogador, quero que vencer uma batalha contra um pet que eu não tenho me dê aquele pet, para que jogar seja a forma natural de crescer minha coleção.

**Acceptance Criteria:**
1. WHEN o jogador vence uma batalha (evento `BatalhaEncerrada`, `WinningSide` igual ao lado do jogador) THEN o sistema SHALL verificar se o pet do oponente (identificado pelo `id` de catálogo) já está na coleção local
2. WHEN o pet do oponente NÃO está na coleção THEN o sistema SHALL adicionar uma instância dele à coleção local, persistida imediatamente (não esperar o jogador sair do jogo)
3. WHEN o pet do oponente JÁ está na coleção THEN o sistema SHALL não duplicar — nenhuma instância nova é criada
4. WHEN o jogador perde ou empata THEN o sistema SHALL não capturar nada, independente do pet do oponente

**Independent Test:** simular uma batalha completa contra um pet de catálogo específico, terminar em vitória do jogador, confirmar que a coleção local ganhou exatamente 1 instância daquele `id` — e que rodar a mesma vitória de novo não adiciona uma segunda.

---

### P1: Coleção sobrevive ao fechamento do jogo ⭐ MVP

**User Story:** Como jogador, quero que minha coleção continue lá na próxima vez que eu abrir o jogo, para que capturar valha a pena de verdade.

**Acceptance Criteria:**
1. WHEN o jogo é fechado e reaberto THEN o sistema SHALL carregar a mesma coleção que existia antes de fechar
2. WHEN não existe save nenhum ainda (primeira vez jogando) THEN o sistema SHALL começar com coleção vazia, sem erro

**Independent Test:** capturar um pet, simular o equivalente a fechar/reabrir (salvar e recarregar o save local diretamente, sem depender de UI), confirmar que a coleção persistiu.

---

### P1: Jogador consegue ver a própria coleção ⭐ MVP

**User Story:** Como jogador, quero uma lista do que eu já capturei, para acompanhar meu progresso.

**Acceptance Criteria:**
1. WHEN o jogador consulta a coleção THEN o sistema SHALL retornar a lista completa de instâncias capturadas, com nome e tipo de cada uma (dado já existente via `FPetPresentationInfo`/`FLoadedPetRecord`)

**Independent Test:** capturar 3 pets distintos, consultar a coleção, confirmar que os 3 aparecem com nome/tipo corretos.

---

### P2: Vitória contra pet já capturado ainda dá alguma recompensa

**User Story:** Como jogador que já capturou tudo que apareceu até agora, quero que vencer não seja "nada acontece", para a repetição continuar valendo a pena.

**Why P2:** o MVP (P1) já entrega o loop principal (capturar o que falta); recompensa para vitória "redundante" é retenção de longo prazo, não bloqueio do conceito.

**Acceptance Criteria:**
1. WHEN o jogador vence contra um pet já capturado THEN o sistema SHALL conceder XP à instância correspondente na coleção (gancho para a próxima feature, Níveis/Experiência/Evolução — não implementado nesta spec, só o ponto de extensão registrado)

---

## Edge Cases

- WHEN a batalha termina em abandono (Sala e Pareamento Simples, `DeclareAbandonment`) THEN o sistema SHALL tratar como vitória válida para fins de captura — o evento `BatalhaEncerrada` é o mesmo, a causa não muda a regra
- WHEN o jogador vence uma batalha CONTRA SI MESMO (Standalone, sem oponente humano — o "oponente" é `FDumbOpponentAI` usando um pet de catálogo) THEN o sistema SHALL capturar normalmente — não há distinção entre "oponente humano" e "IA" para fins de captura, só entre "já tenho" e "não tenho"
- WHEN o save local está corrompido ou ilegível THEN o sistema SHALL falhar explicitamente ao carregar (log alto e claro) e começar com coleção vazia, nunca crashar o jogo nem inventar dados
- WHEN dois pets distintos do catálogo têm o mesmo `type` (ex.: dois pets "Fogo" diferentes, `id`s diferentes) THEN o sistema SHALL tratá-los como capturas INDEPENDENTES — a identidade de captura é o `id` do catálogo, nunca o `type`

---

## Decision Points (para o Design)

- **DP-colecao-01:** onde e como o save local vive — `USaveGame` da Unreal (`UGameplayStatics::SaveGameToSlot`) é o candidato natural; design.md confirma o formato dos dados salvos (lista de `id`s de catálogo capturados, mais o que a feature de Níveis precisar guardar depois).
- **DP-colecao-02:** onde o gancho de captura se conecta ao fluxo de batalha existente — o evento `BatalhaEncerrada` já é consumido por `UBattleResultWidget` (Apresentação do Combate); a lógica de captura precisa de um consumidor próprio do mesmo evento, sem duplicar a leitura do `WinningSide`.
- **DP-colecao-03:** identidade da "instância capturada" — reaproveitar `FLoadedPetRecord`/`FPetPresentationInfo` como estão, ou um tipo novo (`FOwnedPetInstance`) que referencia o `id` de catálogo e guarda o que é específico da instância (hoje: nada além da posse; a feature de Níveis adiciona XP/nível aqui).
- **DP-colecao-04:** qual das duas partes (Side 0 sempre é "o jogador local" hoje?) determina "o jogador venceu" para fins de captura — revisar a mesma convenção já usada em `UBattleResultWidget::ApplyBattleEndedEvent` (`LocalPlayerSide`), não inventar uma nova.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| COLECAO-01 | P1: Vitória contra pet novo captura | Specify | Pending |
| COLECAO-02 | P1: Captura persiste imediatamente | Specify | Pending |
| COLECAO-03 | P1: Pet já capturado não duplica | Specify | Pending |
| COLECAO-04 | P1: Derrota/empate não captura | Specify | Pending |
| COLECAO-05 | P1: Coleção sobrevive a fechar/reabrir | Specify | Pending |
| COLECAO-06 | P1: Início sem save é coleção vazia, sem erro | Specify | Pending |
| COLECAO-07 | P1: Jogador consulta a própria coleção | Specify | Pending |
| COLECAO-08 | P2: Vitória redundante concede XP (gancho) | Specify | Pending |

**Cobertura:** 8 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Vencer contra 3 pets de catálogo distintos, em 3 partidas separadas, resulta numa coleção local com exatamente 3 instâncias, cada uma corretamente identificada
- [ ] Fechar e recarregar o save local (sem UI, direto no teste) preserva a coleção exatamente como estava
- [ ] Vencer duas vezes contra o mesmo pet de catálogo nunca resulta em mais de 1 instância dele na coleção
- [ ] Perder ou empatar contra um pet novo nunca adiciona ele à coleção
- [ ] Regressão zero: os 66 testes de `BattleSquare` e 52 de `BattleSim` continuam passando; nenhuma mudança no fluxo de batalha em si, só um novo consumidor do evento já existente
