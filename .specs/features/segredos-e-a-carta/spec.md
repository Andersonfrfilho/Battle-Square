# Segredos e a carta — por que o gabarito vem antes do segredo

Vem de **P10** em `.specs/features/pendencias/BLOQUEIO-P7-P8-P9-P10.md`.

A promessa é curta: **existe coisa no mundo que a carta não mostra, e achar é
mérito de quem andou.** Hoje o mundo não tem segredo nenhum — e o que impede não
é falta de código, é o **gabarito de aceite**.

---

## O gabarito é a carta, e a carta hoje promete COMPLETUDE

`Source/BattleSquare/Private/Tests/ChartConformanceTest.cpp` espelha
`docs/mundo/carta-ilha-de-mata.html` número por número, em três testes:

| teste | o que afirma |
|---|---|
| `GroundUseMatches` | 9 bosques, 6 clareiras, 8 fazendas, 4 criadouros, 4 lojas, 6 acampamentos, 3 pomares, 7 decks, **5 templos**, **4 ruínas**, **7 cemitérios**, e **2 poços que dão água** |
| `CrossingsMatch` | 30 vaus, 25 balsas, **0 pontes**, 56 travessias no total |
| `WaterAndTrailsMatch` | 137 cursos, 5 riachos, 5 nascentes, 23 trilhas, **158 galerias**, 2 aquedutos |

O cabeçalho do arquivo diz por que ele é assim: *"Quando um número mudar de
verdade, muda-se a carta e muda-se esta lista — nas duas, de propósito, porque a
divergência é a informação."*

**Esconder qualquer coisa REPROVA** — e isso não é defeito do teste, é o teste
fazendo o trabalho dele. Um templo escondido é `Templos == 4`, e o teste afirma
5. O gabarito de hoje não tem como dizer "cinco, dos quais um não aparece na
carta": ele só sabe contar o que existe, e presume que tudo o que existe está
desenhado.

**Para haver segredo, a carta precisa dizer o que ela NÃO mostra** — uma
contagem de "coisas fora da carta", que o teste passa a cobrar também. Sem isso,
o teste que protege a feature acaba proibindo o desenho.

---

## ⚠️ O que eu medi, e que muda a leitura de P10

**O teste NÃO afirma tudo o que a carta promete.** Três contagens estão na carta
e não estão em teste nenhum:

| a carta diz | o teste afirma? |
|---|---|
| Uso do solo: **3 selvagens** (`EGroundUse::PomarSelvagem`) | ❌ não |
| Cemitérios: 7 de vila · **1 esquecido** (`CemiterioEsquecido`) | ❌ não |
| **Cavernas: 16**, retangulares, 2 a 3 bocas | ❌ não |

E o `PomarSelvagem` **já é** conceitualmente um segredo: pomar que ninguém
plantou, e a carta o desenha. As ruínas idem — a carta diz que elas *"ficam longe
de trilha e de vila, e achá-las é acidente"*.

Isto agrava P10 em vez de aliviar. Hoje, **"escondido" e "não afirmado" são
indistinguíveis** — e a indistinção é silenciosa: apagar as três selvagens do
gerador passa em toda a bateria, exatamente como escondê-las passaria. O
gabarito não está apenas incapaz de expressar segredo; ele tem três buracos onde
o segredo caberia sem ninguém saber.

**É esse o mecanismo da feature, e a razão de a primeira task ser o gabarito:**
afirmar só a contagem do que aparece deixa passar **esconder** e **apagar** com
a mesma cara. Afirmar `mostrado + escondido + a SOMA` separa os dois.

E o teste já usa esse argumento em outro lugar, por escrito: os `0 pontes` vêm
com o comentário *"o zero é uma medição, não uma ausência de medição"*. A
contagem de escondidos é o mesmo raciocínio aplicado a uma dimensão nova.

---

## Onde a marca de "escondido" mora

**Na mancha, não numa lista ao lado.** `FBakedGroundUse` já tem `bYieldsWater`
— um booleano por mancha, que é o precedente. Uma lista paralela de "o que está
escondido" seria uma segunda verdade sobre o mesmo traçado, e cópias concordam
até a primeira edição (L-032, L-033).

E `UPROPERTY` ausente desserializa como `false` = **mostrado** = o
comportamento de hoje. Assado velho continua válido, e nenhum mundo já assado
ganha segredo de carona.

---

## Como o jogador acha

`FWorldDiscovery` já existe e é exatamente este cano: guarda **o que o jogador
já viu**, em grade grossa (`RegionSizeUnits()` = raio / 25, hoje cerca de 800, o
que mantém 6 400 regiões em qualquer tamanho de ilha), com
`SightRadiusInRegions` de 1. É pura, não conhece `UWorld`, e é onde "revelado
por andar" mora sem inventar sistema novo.

`FWorldMapPins` marca o que interessa; `FWorldStatusReadout` e
`FBattleDebugScreen::Show` põem na tela. Nada aqui é sistema novo — é ligar
peças que já têm teste.

---

## Aceite

**A MESMA ruína, antes e depois de andar até ela:** a carta não a menciona, o
painel do jogador não a menciona, e depois de o jogador passar perto ela aparece
no painel dele — enquanto a carta continua calada. Sem a feature, tudo o que
existe está na carta, e chegar perto não muda nada na tela.

E o gabarito prova a diferença que importa: **esconder passa, apagar reprova.**
Se as duas coisas continuarem passando, a feature não foi feita — só ficou sem
teste.
