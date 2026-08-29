# Mobile — Especificação

**Status:** Entregue o que não depende de aparelho; compilar e medir está BLOQUEADO por B-006/B-007 (componentes de engine ausentes). Status corrigido em 29/08/2026
**Depende de:** M5 concluído (o mundo aberto é o maior consumidor de recurso do jogo, e é dele que sai o teto de hardware). Primeira feature de M6.

---

## Problem Statement

O jogo roda em PC e nunca rodou em outro lugar. M6 existe para mudar isso, e a ordem do roadmap não é arbitrária: portar antes de M5 seria portar um jogo que ainda não tinha o seu maior consumidor de recurso — o mundo aberto contínuo, com World Partition, streaming e HLOD.

Existe também uma dívida explícita que este marco cobra: **`STATE.md` carrega, desde antes de M5, o Todo "Definir orçamento de performance de mobile antes de M5 — decisões de mundo tomadas sem ele são irreversíveis".** Esse Todo não foi cumprido. M5 foi inteiro decidido com um orçamento declaradamente provisório (DP-streaming-01: 512 MB e 60 FPS, com a nota honesta de que são "decisão de partida, não medição"). Esta feature é onde essa conta chega.

**E aqui a spec precisa ser honesta sobre uma coisa antes de prometer qualquer outra:** *validar* mobile exige um aparelho, e um aparelho exige coisas que não estão nesta máquina. Ver "Bloqueios Conhecidos" abaixo — eles não são desculpa, são o desenho do escopo.

## Goals

- [ ] Existe um **orçamento de performance de mobile nomeado e escrito**, que substitui o número provisório de DP-streaming-01 por um alvo declarado com hardware de referência — pagando a dívida que `STATE.md` cobra desde antes de M5
- [ ] O traversal de M5 aceita **entrada por toque**, reusando a mesma conversão input → direção que já existe e já é testada — sem um segundo caminho de movimento
- [ ] Existem **perfis de dispositivo e escalabilidade** declarados para mobile, em vez de o jogo herdar os padrões de PC
- [ ] Está **escrito, com precisão, o que falta na máquina** para compilar, empacotar e validar em aparelho — cada item com o remédio exato e de quem é a ação

## Out of Scope

| Item | Razão |
|---|---|
| Empacotar e rodar num aparelho físico | **Bloqueado**, não descartado — ver B-006/B-007. Nenhuma linha de código resolve |
| Medir o orçamento em hardware real | Mesmo bloqueio. O orçamento desta feature é um **alvo declarado**, e a spec diz isso na cara em vez de fingir medição |
| Console | Segunda feature de M6, e bloqueada por licença e devkit (B-008) |
| UI de toque (botões virtuais, HUD adaptado) | Depende de autoria visual/UMG, que este projeto adiou desde `apresentacao-combate` (DP-08). Toque aqui é **entrada de traversal**, não interface |
| Otimização de verdade (reduzir draw calls, atlas, LOD autoral) | Otimizar sem medir é chute. Só faz sentido depois de B-006/B-007 caírem e existir uma medição |

---

## Bloqueios Conhecidos

Registrados como blockers de infraestrutura, no mesmo espírito de B-004 (que já registrou que esta engine não compila `TargetType.Server`). **Nenhum é resolvível por código**, e todos precisam de uma ação do usuário.

| Id | O quê | Evidência | Remédio, e de quem é |
|---|---|---|---|
| **B-006** | A engine não tem o componente **IOS** instalado | `Build.sh BattleSquare IOS Development` → `Missing files required to build IOS targets. Enable IOS as an optional download component in the Epic Games Launcher.` | Usuário: instalar o componente IOS pelo Epic Games Launcher |
| **B-006b** | Nenhuma identidade de assinatura nem perfil de provisionamento | `security find-identity -v -p codesigning` → `0 valid identities found`; `~/Library/MobileDevice/Provisioning Profiles/` vazio | Usuário: conta Apple Developer + certificado + perfil |
| **B-007** | SDK Android ausente | Validação de plataforma da engine → `Android INVALID r27c` | Usuário: instalar NDK r27c / Android Studio pelo `SetupAndroid` da engine |

**O que isso muda no escopo, com franqueza:** tudo que precisa de aparelho sai. Sobra o que é verificável nesta máquina — o orçamento escrito, a entrada de toque (testável headless, porque a parte que é nossa é aritmética pura desde DP-trav-02), e a configuração declarada. Isso não é "mobile pronto"; é **mobile preparado até a fronteira do que a máquina permite**, com a fronteira nomeada.

---

## User Stories

### P1: Orçamento de performance nomeado ⭐ MVP

**User Story:** Como quem toma decisões de mundo, quero um orçamento de performance de mobile escrito e justificado, para que decisões irreversíveis de escala parem de ser tomadas contra um número provisório.

**Acceptance Criteria:**
1. WHEN uma decisão de escala de mundo é tomada THEN o projeto SHALL ter um documento que declara: aparelho de referência, memória alvo, frame time alvo e resolução alvo
2. WHEN o documento declara um número THEN ele SHALL dizer se é **medição** ou **alvo declarado**, sem ambiguidade — o erro que DP-streaming-01 evitou por pouco e nomeou
3. WHEN o orçamento for medido de verdade (depois de B-006/B-007) THEN o documento SHALL ter um lugar já preparado para o número real entrar ao lado do alvo

**Independent Test:** o documento existe, e o Todo correspondente em `STATE.md` sai de aberto.

---

### P1: Traversal aceita toque ⭐ MVP

**User Story:** Como jogador em celular, quero andar pelo mundo com o dedo, para que o traversal de M5 exista no aparelho e não só no teclado.

**Acceptance Criteria:**
1. WHEN a entrada vem de um toque em vez de teclado THEN o sistema SHALL produzir a **mesma** direção de movimento, pela mesma função pura de DP-trav-02 — nunca um segundo caminho de movimento
2. WHEN o toque é um arrasto a partir de um ponto de origem THEN o sistema SHALL converter o deslocamento em eixo 2D normalizado, com uma **zona morta nomeada** para o dedo parado não gerar deriva
3. WHEN o arrasto passa de um raio máximo nomeado THEN o sistema SHALL saturar em 1, para o movimento não ficar mais rápido quanto mais longe o dedo vai

**Independent Test:** headless e puro — arrasto para cima dá frente; arrasto dentro da zona morta dá vetor nulo; arrasto além do raio máximo tem comprimento exatamente 1; arrasto na diagonal é normalizado.

---

### P2: Perfis de dispositivo e escalabilidade declarados

**User Story:** Como quem for empacotar quando o bloqueio cair, quero que mobile já tenha perfis próprios, para que o primeiro build no aparelho não herde os padrões de PC.

**Acceptance Criteria:**
1. WHEN o projeto é empacotado para mobile THEN ele SHALL usar configuração de escalabilidade declarada no projeto, não os padrões de PC
2. WHEN a configuração é lida THEN ela SHALL ser coerente com o orçamento de P1 — os dois não podem discordar

**Independent Test:** os arquivos de configuração existem e declaram os valores; **verificação real só depois de B-006/B-007** — e o roteiro manual diz isso.
