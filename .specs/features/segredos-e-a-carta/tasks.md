# Segredos e a carta — tarefas

Quatro tarefas. A primeira é o **gabarito**, e não o segredo: enquanto
`ChartConformance` afirmar só a contagem do que aparece, esconder qualquer coisa
reprova, e a feature não tem por onde nascer.

Ordem obrigatória. SC2 antes de SC1 é assar um mundo que a bateria recusa.

---

## SC1 — O gabarito aprende a dizer "escondido"
> 🤖 Modelo: `opus` 🧠 — muda o gabarito de aceite

`ChartConformanceTest.cpp` afirma hoje `Templos == 5`, `Ruinas == 4`,
`Cemiterios == 7`, `PocosQueDaoAgua == 2`. Um templo escondido é
`Templos == 4`, e o teste reprova. **Não é defeito do teste** — é o teste
fazendo o trabalho dele contra um gabarito que promete completude.

O gabarito passa a ter **três** afirmações por categoria, e as três juntas são o
mecanismo:

| afirmação | o que ela impede |
|---|---|
| quantos **aparecem** na carta | que a carta e o traçado divirjam |
| quantos estão **escondidos** | que "escondido" seja indistinguível de "apagado" |
| a **SOMA** | que esconder um e apagar outro se cancelem na conta |

**Por que as três, e não só a primeira:** afirmar apenas o mostrado deixa
passar, com a mesma cara, esconder um templo e **apagar** um templo. O teste já
usa esse argumento por escrito nos `0 pontes` — *"o zero é uma medição, não uma
ausência de medição"*. A contagem de escondidos é o mesmo raciocínio numa
dimensão nova.

**Esta é a task onde o teste velho VIRA o teste novo.** `GroundUseMatches`
deixa de afirmar `Templos == 5` e passa a afirmar
`TemplosNaCarta == N && TemplosEscondidos == M && N + M == 5`. Não convive com
uma segunda versão: duas afirmações sobre a mesma contagem concordam até a
primeira edição, e é exatamente o que L-032 e L-033 cobraram.

⚠️ **Três contagens da carta não estão em teste nenhum hoje:** `PomarSelvagem`
(a carta diz **3 selvagens**), `CemiterioEsquecido` (**1 esquecido**) e
`Caves` (**16 cavernas**). Elas entram nesta task, com a forma nova de saída —
enquanto não estiverem afirmadas, apagá-las passa em toda a bateria, que é o
mesmo buraco por outro nome.

*Aceite:* **esconder passa, APAGAR reprova.** Com um templo marcado como
escondido, a bateria fica verde; com um templo removido do gerador, ela reprova
e a mensagem diz qual soma não fechou. Se as duas passarem, a task não foi
feita.

*Contrapeso obrigatório:* a carta **e** o teste mudam na mesma task. A carta
passa a ter a linha de "fora da carta" com o número; o cabeçalho do teste
continua valendo (*"muda-se a carta e muda-se esta lista, nas duas, de
propósito"*) e o número zero é escrito de propósito enquanto ninguém esconder
nada.

*Verificação:*
```bash
./Tools/run_tests.sh BattleSquare.ChartConformance
./Tools/audit_no_recalculation.sh
```

---

## SC2 — "Escondido" é marca na MANCHA, não lista ao lado
> 🤖 Modelo: `sonnet`

`FBakedGroundUse` já tem `bYieldsWater` — um booleano por mancha. É o
precedente, e a marca de escondido segue o mesmo caminho.

**Por que na mancha:** uma lista paralela de "o que está escondido" seria uma
segunda verdade sobre o mesmo traçado. Bastaria mover uma mancha para as duas
discordarem, e nada acusaria — cópias concordam até a primeira edição.

`UPROPERTY` ausente desserializa como `false`, e `false` é **mostrado**, que é o
comportamento de hoje. **Assado velho continua válido**, e nenhum mundo já assado
ganha segredo de carona.

*Aceite:* assado antigo carrega e conta **zero** escondidos; assado novo com uma
mancha marcada conta um. Sem a primeira metade, esta task apagaria o mapa de
quem já jogava.

*Contrapeso obrigatório:* `IslandBakedPlanGuardTest` continua **nomeando** o
parâmetro que divergiu. Campo novo no assado sem isso produz "o plano mudou" sem
dizer o quê, que é o defeito que aquele teste existe para não ter.

*Verificação:*
```bash
./Tools/bake_island.sh
./Tools/run_tests.sh BattleSquare
./Tools/audit_determinism.sh
```

---

## SC3 — O mapa DO JOGADOR revela por andar; a carta continua calada
> 🤖 Modelo: `sonnet`

`FWorldDiscovery` já guarda o que o jogador viu, em grade grossa
(`RegionSizeUnits()` = raio / 25, `SightRadiusInRegions` = 1), pura e sem
`UWorld`. É o cano que já existe, e a revelação entra nele.

**Duas visões, e elas não são a mesma:**

| visão | o que mostra |
|---|---|
| **a carta** (`docs/mundo/`) | o mundo desenhado; o escondido **nunca** aparece |
| **o painel do jogador** | o que ELE já viu, escondido incluído depois de achar |

Misturar as duas destruiria a feature: se a carta revela ao andar, ela deixa de
ser documento do mundo e passa a ser estado de partida — e o
`ChartConformance` de SC1 pararia de ter gabarito estável para espelhar.

*Aceite:* o mesmo segredo, antes e depois. Antes de o jogador passar perto, o
painel não o menciona; depois, menciona. E a carta não menciona nem antes nem
depois.

*Contrapeso obrigatório:* revelar é **por andar perto**, e a distância sai de
`FWorldDiscovery` — não de uma constante nova. Duas réguas de "perto" neste
projeto é a mesma armadilha de sempre.

---

## SC4 — O achado APARECE na tela
> 🤖 Modelo: `sonnet`

`FWorldStatusReadout::Build` monta as linhas do painel do mundo (chaves 740+) e
`FBattleDebugScreen::Show` as põe na tela. O achado entra ali, em `LOCTEXT` com
argumentos **nomeados**.

**Segredo que ninguém vê achar não foi achado.** Em 26–27/08, oito defeitos
sérios existiram por semanas e sete só apareceram porque um humano olhou a tela.
Uma ruína revelada em silêncio é indistinguível de uma ruína que não existe — e
é o defeito mais caro desta feature justamente porque nada reprova.

*Aceite:* passar perto da ruína escondida escreve a linha no painel; passar
longe não escreve nada. Roteiro em `docs/verification/segredos-e-a-carta.md`
dizendo onde ir.

*Contrapeso obrigatório:* o texto é `LOCTEXT`, nunca `FString::Printf`, e as três
culturas coletam. O modo de falhar é silencioso — o português continua certo e o
idioma novo nasce faltando linha.

*Verificação:*
```bash
./Tools/audit_localizable_text.sh && ./Tools/gather_text.sh
./Tools/run_tests.sh BattleSquare
./Tools/sync_module_manifest.sh
```

---

## Decisões que são do usuário

Estas três são de **produto antes de serem de código**, e a primeira task não
começa sem elas. São listadas, não escolhidas.

1. **Quantos segredos, e de que tipo.** A carta já desenha coisas que se
   comportam como segredo — **3 selvagens**, **1 cemitério esquecido**, e ruínas
   que *"ficam longe de trilha e de vila, e achá-las é acidente"*. A escolha é
   entre **marcar como escondido o que já existe** (a carta perde três linhas e
   o mundo não muda) e **acrescentar coisas novas fora da carta** (o mundo muda,
   e as somas de SC1 sobem).

2. **A carta CONTA os segredos, ou nem isso?** Três formas, e cada uma é um jogo
   diferente:

   | forma na carta | o que o jogador sabe |
   |---|---|
   | "5 templos, 1 fora da carta" | que falta um, e sai atrás |
   | "5 templos" e o sexto sem menção nenhuma | nada; achar é acidente |
   | "há coisas fora desta carta" sem número | que existe algo, sem saber quanto |

   ⚠️ A segunda forma **não fecha o gabarito**: sem o número, o teste volta a não
   distinguir escondido de apagado, que é o buraco de hoje. Se ela for a
   escolhida, o número tem de existir em algum lugar que o teste leia — e esse
   lugar precisa ser nomeado antes de SC1.

3. **Contar ainda deixa ser segredo?** É a pergunta de fundo, e é do dono do
   jogo. Saber que existe um sexto templo é metade da caça; a outra metade é
   onde. Se contar estragar a graça, a saída é a contagem viver num arquivo de
   verificação e não na carta que o jogador lê — o que muda **quem** é o
   gabarito, e por isso não é decisão de código.

---

## O que estas tarefas NÃO fazem

- **Não fazem a carta revelar por andar.** A carta é documento do mundo; o
  painel do jogador é o que ele viu. Fundir as duas tira de SC1 o gabarito
  estável que ele espelha.
- **Não inventam sistema de descoberta.** `FWorldDiscovery`, `FWorldMapPins` e
  `FWorldStatusReadout` existem e têm teste. O que falta é ligar.
- **Não mexem no traçado.** Nenhuma mancha muda de lugar; o que muda é se ela
  aparece na carta. Se a decisão 1 pedir coisas novas, isso é traçado e é outra
  feature.
- **Não mexem nas cavernas.** As **16 cavernas** entram em SC1 só como contagem
  afirmada. Onde elas ficam é assunto de `cavernas-nas-quedas`.
- **Não autoram asset.** O achado aparece por texto no painel, como o item
  apareceu.
