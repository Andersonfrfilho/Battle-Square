# Mobile — Design

**Spec:** `.specs/features/mobile/spec.md`
**Status:** Draft — aguarda aprovação

---

## DP-mobile-01: O orçamento é um alvo declarado, e o documento diz isso

**Decisão:** `docs/performance/orcamento-mobile.md`, com aparelho de referência, memória, frame time e resolução — e uma coluna que marca cada número como **alvo** ou **medido**. Hoje todos são alvo. O documento nasce com a tabela de medição vazia e um passo escrito de como preenchê-la quando B-006/B-007 caírem.

**Aparelho de referência escolhido:** um aparelho de **gama média de ~4 anos atrás** (referência: iPhone 11 / Snapdragon 730 class, 4 GB de RAM). **Razão:** mirar no topo de linha é mirar em quem não precisa de otimização; mirar no mais fraco possível é desenhar para um mercado que já sumiu. Gama média de 4 anos é o aparelho que a maior parte de um público real de fato tem na mão.

**Números, e de onde saem:**

| Número | Alvo | De onde vem |
|---|---|---|
| Memória do processo | **1,5 GB** | 4 GB de RAM total, com o SO e o resto do sistema comendo boa parte; 1,5 GB é o teto realista antes do risco de o SO matar o app |
| Streaming (subconjunto do acima) | **384 MB** | Os 512 MB de DP-streaming-01 eram um alvo de PC. Em mobile ele **desce**, e é este o primeiro número que M5 vai ter de rever |
| Frame time | **33,3 ms (30 FPS)** | 60 FPS em mundo aberto num aparelho de gama média de 4 anos é promessa que não se cumpre. 30 é o piso honesto; o alvo de 60 continua valendo em PC |
| Resolução de render | **até 1280x720, com escala dinâmica** | Renderizar na resolução nativa de tela de celular é gastar em pixels que ninguém distingue a essa distância |

**Nota honesta, no mesmo espírito de DP-streaming-01:** nada disto foi medido. São alvos derivados de um aparelho de referência declarado, e existem para que decisões de escala parem de ser tomadas contra o vácuo. **O primeiro que provavelmente cai quando houver medição é o de streaming**, porque 384 MB num mundo com HLOD é apertado.

## DP-mobile-02: Toque reusa a função pura, e o que é novo é só a conversão do gesto

**Decisão:** `FTouchMovementInput::ComputeMovementAxis(const FTouchMovementParams&)` — estática e pura. Recebe origem do toque e posição atual (dois `FVector2D` de tela) e devolve o **eixo 2D** no mesmo formato que o teclado produz. Esse eixo então entra em `FWorldTraversalMotion::ComputeMoveDirection`, que já existe e já é testada.

**Razão:** o traversal já foi resolvido em DP-trav-02 e não vai ser resolvido de novo. O que muda no celular é **como se colhe o eixo**, não o que se faz com ele. Escrever um caminho de movimento separado para toque seria criar o caminho não testado — o mesmo erro que `colecao-e-captura` evitou ao não reimplementar captura.

**Regras da conversão, todas testáveis:**
- **zona morta** (`TouchDeadZoneScreenUnits`): arrasto menor que ela devolve vetor nulo. Sem isso, um dedo parado tremendo gera deriva — o análogo de P1/critério 3 do traversal.
- **raio máximo** (`TouchMaxRadiusScreenUnits`): além dele, satura em comprimento 1. Sem isso, quanto mais longe o dedo, mais rápido o personagem — o que ninguém espera de um analógico virtual.
- **eixo de tela → eixo de jogo:** arrastar para **cima** na tela é **frente**. Em coordenadas de tela o Y cresce para baixo, então o sinal inverte. Isto é exatamente o tipo de detalhe que se erra em silêncio e que um teste com número exato pega.

**O que NÃO se testa aqui:** que o toque real chega do sistema operacional. Isso é a engine e o aparelho — item de roteiro manual, atrás de B-006/B-007.

## DP-mobile-03: Escalabilidade declarada em `Config/`, coerente com o orçamento

**Decisão:** `Config/DefaultDeviceProfiles.ini` e `Config/DefaultScalability.ini` declarando, para o perfil mobile, os valores que decorrem de DP-mobile-01 (escala de resolução, sombras, pós-processamento, distância de view).

**Razão:** o padrão da engine para um perfil não declarado é herdar configuração pensada para PC. É o tipo de omissão que só aparece no primeiro build no aparelho — tarde, e atrás de um bloqueio que torna "tarde" ainda mais caro.

**Limite honesto:** estes arquivos **não são verificáveis** nesta máquina. Nenhum teste headless prova que uma escalabilidade de mobile faz o que promete — só um aparelho prova. O que se pode fazer, e se faz, é declará-los coerentes com o orçamento e deixar a verificação escrita no roteiro.

## DP-mobile-04: O que é automatizável, e o que não é

| Verificação | Automatizável? | Como |
|---|---|---|
| Conversão gesto → eixo (zona morta, saturação, sinal do Y) | ✅ Sim | Teste headless puro sobre `FTouchMovementInput` |
| O eixo de toque produz a mesma direção que o de teclado | ✅ Sim | Teste headless compondo com `FWorldTraversalMotion` |
| Compilar para iOS/Android | ❌ **Bloqueado** | B-006 / B-007 — nem é questão de teste, o toolchain não existe na máquina |
| Frame time, memória, e o orçamento inteiro | ❌ **Bloqueado** | Precisa de aparelho. Roteiro manual, atrás dos mesmos blockers |
| Escalabilidade fazendo efeito | ❌ Não | Só no aparelho |

---

## O que muda

- **`docs/performance/orcamento-mobile.md`** (novo) — o orçamento, e a dívida de `STATE.md` paga
- **`FTouchMovementInput`** (novo, `BattleSquare/World/`) — gesto → eixo, puro
- **`Config/DefaultDeviceProfiles.ini`, `Config/DefaultScalability.ini`** (novos)
- **`STATE.md`** — Todo do orçamento fechado; B-006/B-006b/B-007 registrados

## O que NÃO muda

- **`BattleSim`:** nenhuma linha.
- **`FWorldTraversalMotion`, `AWorldExplorerCharacter`:** consumidos como estão. Toque entrega o mesmo eixo que o teclado; nada a jusante sabe a diferença.
- **As três sondas:** continuam limpas.
