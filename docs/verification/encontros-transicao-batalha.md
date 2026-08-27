# Roteiro de Verificação Manual — Encontros e Transição para Batalha

**Feature:** `.specs/features/encontros-transicao-batalha/`
**Nível:** `Content/Maps/WorldStreamingTest` (`/Game/Maps/WorldStreamingTest`)
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado no editor.

Este documento cobre só o que `design.md` (DP-enc-05) marca como **não
automatizável de forma confiável**: a ausência de tela de carregamento na
transição, e a permanência do mundo carregado enquanto a batalha corre. Tudo o
que é regra — disparo do encontro, montagem com os `CatalogId` certos,
restauração exata da posição, não disparar um segundo encontro na volta — já
está coberto por `Automation RunTests BattleSquare.World` e **não** se repete
aqui.

Cada item tem um passo concreto e um número a ler. Marcar `[x]` só depois de
executar, anotando data e quem verificou.

---

## Preparação

1. Abrir o Editor no projeto `BattleSquare.uproject` e carregar
   `/Game/Maps/WorldStreamingTest`.
2. Confirmar no Outliner a pasta `StreamingEncounters/` com os
   `AWorldEncounterActor` posicionados sobre a rota do `DebugRoutePawn`
   (a rota vai de `(-3600,-3600)` a `(3600,-3600)`, daí a `(3600,3600)` e
   termina em `(-3600,3600)`).
3. Confirmar que o `DebugRoutePawn` tem, além do `DebugRouteMover`, um
   `UEncounterDetectionComponent`.
4. **Dados — já resolvidos.** Os três atores de encontro carregam `CatalogId`
   **reais** do espelho de pets (Whiskers, Spike e Fluffy), e o pet do jogador
   vem de `DefaultGame.ini` (`UnrealTestFixturePet`, o único que os testes
   tratam como estável). O espelho apontado é o de **fixture**, de propósito:
   `WorldStreamingTest` é nível de verificação, e o espelho de produção só
   existe depois que o worker de sync roda contra a API.
5. **Quem dispara os encontros:** o `DebugRoutePawn` é quem tem
   `AutoPossessPlayer`, e o bootstrap escolhe o pawn **possuído** — então dar
   Play já faz ele percorrer a rota e encostar nos encontros sozinho, sem
   ninguém no teclado. Para testar andando, trocar a posse para o
   `WorldExplorer` (e devolver depois).

---

## ENC-01 — A transição mundo → batalha não tem tela de carregamento

- [ ] **Não verificado**

**Passo:** dar Play in Editor e deixar o `DebugRoutePawn` percorrer a rota até
alcançar o primeiro encontro. No instante em que a batalha começa, observar a
tela.

**Critério:** nenhuma tela de carregamento, fade preto ou congelamento. A arena
aparece no mesmo frame ou no seguinte, porque ela nasce no **mesmo `UWorld`**,
deslocada de 1.000.000uu (DP-enc-03) — não há troca de nível.

**Se houver engasgo:** rodar `stat unit` junto e anotar o pico de `Frame` no
momento da transição: ______ ms.

---

## ENC-02 — O mundo continua carregado durante a batalha

- [ ] **Não verificado**

**Passo:** com a batalha ativa (depois de ENC-01), rodar `stat streaming` no
console e anotar o número de células carregadas: ______.

**Critério:** o número é **o mesmo** que estava carregado no mundo um instante
antes do encontro. É isto que torna a volta instantânea: nada foi descarregado,
então nada precisa recarregar. Se o número cair para perto de zero, a arena
provavelmente está dentro do raio de streaming e puxou a fonte de streaming
junto — o deslocamento precisaria ser revisto.

---

## ENC-03 — A volta é instantânea e no lugar certo

- [ ] **Não verificado**

**Passo:** deixar a batalha terminar (qualquer resultado). Observar a volta ao
mundo.

**Critério, em três partes:**
1. Nenhuma tela de carregamento na volta.
2. O pawn está **exatamente** onde estava — a mesma posição e a mesma direção.
   O teste headless já garante isso numericamente; o que se confere aqui é que
   a câmera não "salta" de um jeito desconfortável.
3. O pawn está parado em cima (ou ao lado) do pet que acabou de derrotar e
   **nenhuma segunda batalha começa**. Se começar, o bug é a ordem de
   DP-enc-03, e há um teste dedicado que deveria ter pegado — reportar como
   regressão, não como comportamento esperado.

---

## ENC-04 — O resultado da batalha valeu

- [ ] **Não verificado**

**Passo:** antes de encostar no encontro, anotar o tamanho da coleção e o nível
do pet do jogador. Depois de vencer a batalha e voltar ao mundo, conferir de
novo.

| Momento | Pets na coleção | Nível do pet do jogador |
|---|---|---|
| Antes do encontro | ___ | ___ |
| Depois da batalha | ___ | ___ |

**Critério:** captura e XP já valem **na volta ao mundo**, sem nenhuma ação
extra. Isto não é código novo desta feature — é `FPetCollectionService` e
`FPetProgressionService` rodando por dentro de `ABattleArena` (DP-enc-04). O
que este item verifica é que a transição não os atropelou ao destruir a arena.

---

## Ponta ligada em 2026-08-27: a batalha do mundo tem interface

Até aqui a transição levava o jogador à arena e o deixava **sem botões de
ação** — ele caminhava, encontrava o oponente, a câmera ia para a arena, e ele
ficava olhando uma luta que não podia jogar. Isso é pior que não ter transição:
parece defeito do jogo, não recurso faltando.

Agora o fluxo avisa quando a batalha começa, e o GameMode do mundo monta a
**mesma** tela de ações da tela de batalha — uma tela, não duas.

### ENC-10 — Caminhar até a batalha e voltar

- [ ] Caminhar até um encontro leva à arena **com os botões de ação na tela**.
- [ ] Dá para jogar o turno inteiro: escolher, confirmar, ver a reprodução.
- [ ] A barra de depuração aparece (ações do jogador 2, copiar painel).
- [ ] Ao fim da batalha, a tela de ações **some** e o controle volta ao mundo.
- [ ] O explorador volta respondendo ao teclado — cursor escondido, input de
      jogo. Se ele nascer parado, a transição de volta não completou.
- [ ] Se o caminho do widget estiver errado no `.ini`, o painel diz isso em
      vermelho em vez de abrir uma batalha injogável em silêncio.

### ENC-11 — O mundo tem inimigos, e eles andam

Até 2026-08-27 **ninguém criava encontros**: eles só existiam se alguém os
tivesse colocado à mão no nível. Caminhar pelo mundo nunca disparava batalha, e
nada acusava — o recurso inteiro ficava inalcançável em silêncio.

- [ ] Ao entrar no mundo, o painel diz `N encontros povoaram o mundo, e eles ANDAM`.
- [ ] Você **VÊ** os inimigos (esferas laranja) — eles nasceram sem malha
      atribuída, invisíveis, e nenhum teste de lógica acusava.
- [ ] Os inimigos **se movem** e viram para onde andam.
- [ ] Eles não estão meio enterrados no chão.
- [ ] Eles não somem no horizonte: o passeio é ao redor de onde nasceram.
- [ ] Nenhum nasce em cima do jogador — dá para andar antes do primeiro encontro.
- [ ] Encostar em um leva à batalha, **com botões** (ENC-10).
- [ ] Se o nível já tiver encontros colocados à mão, o painel avisa e nenhum é
      criado por cima.

### ENC-12 — O mundo não acaba

Com um número fixo de encontros, seis batalhas esvaziavam o mapa e sobrava
caminhar por um lugar onde nada mais acontece. Pior: o pet derrotado continuava
passeando como fantasma, dando a entender que a batalha não valeu.

- [ ] Depois de vencer, o pet derrotado **some** do mundo.
- [ ] Em poucos segundos o painel diz `N encontro(s) reposto(s)`.
- [ ] O substituto nasce **longe**, não em cima de você ao voltar da batalha.
- [ ] Vencendo várias vezes seguidas, sempre há inimigos — o mundo não esvazia.
- [ ] E não vira multidão: a população fica no alvo, não acumula.
