# Fundura no traçado — a água ganha FUNDO, e é o gerador que diz quanto

Duas paradas num arquivo porque elas são a **mesma** parada, e a causa é uma só:
cada ponto de curso guarda `x`, `y` e meia-largura, e nada mais. O campo `fundo`
existe por **rio**, não por ponto — e vale `0` nos cinco primeiros.

Disso saem os dois sintomas que a construção do mundo mediu e parou:

| parada | a pergunta que ela faz | por que não há resposta hoje |
|---|---|---|
| **P7** — nadar quando a água passa da cintura | "que fundura tem a água AQUI?" | não existe fundura por ponto |
| **P8** — ler o poço da queda pela fundura | "esta parte do poço é mais funda que aquela?" | o poço tem UM número de fundura, e o relevo dele é liso |

Juntá-las não é conveniência: **P8 é o caso extremo de P7.** Se a fundura por
ponto entrar e o poço continuar sendo um prato, P7 fica de pé e P8 continua
respondendo sempre a mesma coisa.

---

## A tentação, e por que ela é proibida

Dá para inventar uma fundura: `fundura = f(largura)`. Fica barato, roda hoje, e
**é uma segunda verdade sobre a mesma água** — a do traçado e a da fórmula. Elas
concordam até a primeira vez que alguém alargar um rio.

É a invariante 4, e este projeto já pagou duas vezes por ela: **L-032** (dois
validadores para a mesma regra, e o de fora escondia o de dentro) e **L-033** (a
regra estava certa, testada e chamada — em UM dos dois caminhos de entrada).

**Quem decide a fundura é o gerador. O assado carrega. Todo mundo lê.**

### E a segunda verdade JÁ EXISTE — medido

Isto não é hipótese: `Source/BattleSquare/Private/World/TrailLayout.cpp` tem, em
namespace anônimo, exatamente a fórmula proibida.

```cpp
constexpr float FunduraSobreLargura = 0.065f;
float FunduraEm(const FVector2D& Onde);   // devolve meia-largura * 2 * 0,065
```

O comentário dela argumenta que um campo de profundidade separado seria "uma
segunda fonte da mesma verdade (L-032)" — e ela é justamente a segunda fonte,
porque é a **única** que existe. Ela decide as 56 travessias da carta.

Sendo função linear da meia-largura, ela **não carrega informação nenhuma** que
a largura já não carregue. Um rio largo e raso não existe para ela. E é
exatamente o rio largo e raso que P7 precisa enxergar.

**Portanto esta feature não acrescenta fundura: ela MUDA DE DONO.** Sai da
estimativa privada do traçado, entra no assado, e `FunduraSobreLargura` morre no
mesmo commit em que a leitura nasce — porque enquanto as duas existirem, existem
duas verdades, e a nova é a que ninguém está chamando.

---

## O que já está construído, e por isso não vai ser descoberto

Nenhuma das tarefas abre caminho novo. Todas ligam cano que já passa ali:

| cano | onde | prova de que serve |
|---|---|---|
| **vetor paralelo por ponto de rio** | `FBakedRiver` (`IslandBakedPlan.h`) | já tem **cinco**: `HalfWidthUnits`, `bIsRapids`, `FluidByPoint`, `FlowDirectionByPoint`, `FlowStrengthByPoint` |
| **o laço que preenche** | `BakeInto`, um `for` sobre `RiverSampleCount()` | uma linha por vetor, todas na mesma forma |
| **marca por ponto no JSON** | `IslandMapDumpTest.cpp`, bloco `"corredeira"` | o precedente está escrito e comentado: *"vai como marca por ponto do curso, e não como faixa"* |
| **fundura já viaja no JSON** | as travessias saem `{"tipo":"vau", …, "fundura":86}` | o campo já tem nome e já é lido |
| **consulta por progresso** | `FreshWater::*AtProgress` | é a forma da casa; parametrizar por raio é o defeito que §3 das regras de mapa proíbe |
| **quem pisa na água** | `WaterFooting::At` | **já consulta as travessias** para o vau, com a medição escrita no `.cpp` |
| **a linha na tela** | `BattleSquareGameMode.cpp`, chave **748** | já imprime `pisando: %s em %s (passo %.0f%%)` |
| **o assado valida sozinho** | `Tools/bake_island.sh` | já exige as seções e o `.uasset` |

---

## Três coisas que a medição contradisse — e ficam registradas

O documento de bloqueio é de 02/09/2026 e envelheceu em três pontos. Quem
executar precisa saber, porque duas delas mudariam o desenho:

1. **`WaterFooting` já NÃO decide o vau pela largura.** O `.cpp` foi corrigido e
   guarda a medição: *"nenhum ponto de rio saía vau, porque a meia-largura
   mínima é 481 e o limiar do a-pé é 30% da calha"*. Ele lê as travessias. Só o
   comentário do **`.h`** ainda afirma que a largura decide — o cabeçalho está
   defasado, e cabeçalho defasado é o que faz a próxima pessoa reescrever o
   defeito.

2. **A fundura dos poços não é "entre 30 e 51".** No assado vivo os 13 poços
   têm `fundo` em **{16, 26, 28, 30, 32, 32, 32, 42, 46, 49, 51, 54, 64}** —
   mínimo 16, máximo 64.

3. **O prato tem causa aritmética, não artística.** A meia-largura do poço é
   `max(MeiaCalhaDoRio() * 1,15 ; fundo * 1,6)`. Com raio de terra 140 000 e
   `FracaoDaCalhaDoRio = 0,0055`, o piso vale `770 × 1,15 = 886`. O outro termo,
   para fundo de 16 a 64, dá 25,6 a 102,4 — **o piso ganha sempre**, e é por isso
   que todos os 13 poços medem exatamente 886.

   E `fundo = queda × 0,55` implica quedas de apenas **29 a 116** unidades, num
   relevo amostrado a `2 × MeiaQueda() = 493` unidades de distância. Ou seja: a
   rocha **não tem degrau** onde a cachoeira cai. O poço não pode ser fundo
   enquanto a queda não for alta.

   **Por isso P8 é relevo, não consulta.** Escrever `FunduraDoPocoEm(ponto)` hoje
   devolveria o mesmo número em todo o raio de 886 — uma função que só o teste
   chama, que é o mesmo diagnóstico que parou P9.

---

## ⚠️ A armadilha desta feature, e ela já cobrou uma vez

O "dez vezes" da literatura de poço de queda é razão de **VELOCIDADE DE EROSÃO**
— vertical sobre lateral. **Não é a forma do buraco.**

Neste projeto já se escreveu um teste afirmando "mais fundo que largo" lendo essa
fonte errado. **O teste reprovou código certo**, e a mensagem não dava pista
nenhuma de que o errado era o teste.

**Poço de cachoeira é mais LARGO que fundo, e isso está correto.** O defeito de
hoje não é a proporção — é a fundura ser constante. Todo teste desta feature
afirma **variação**, nunca "fundo maior que largo".

O bloco de documentação de `FreshWater.h` ainda diz *"Poço de cachoeira é FURO"*.
É a leitura errada, escrita no código. Ela some junto.

---

## §6 manda aqui: antes de culpar a regra, conferir a RESOLUÇÃO

Este defeito já se disfarçou de **quatro** outros neste mundo — "a trilha nunca
cruza rio", "a trilha sobe o barranco de frente", "não existe corredeira
nenhuma", "a cachoeira não tem poço". Sempre a mesma causa: quem amostra grosso
não vê o que é fino, e o sintoma nunca aponta para a grade.

O que já está medido, e é o ponto de partida:

| grade | valor |
|---|---|
| pontos por curso (`RiverSampleCount()`) | **41** |
| passo entre pontos consecutivos | mínimo **65**, mediana **67**, máximo **1 198** |
| lado da grade de alturas (`HeightGridSide()`) | **180** |
| limiar do a-pé (`TrailLayout::WadableDepthUnits()`) | **100** — um metro |
| meia-altura da cápsula do jogador | **88** (padrão da engine; nenhum `InitCapsuleSize` no projeto) |

Um trecho raso mais curto que o passo local — e o passo chega a 1 198 no pior
curso — é **invisível** para uma amostra por ponto. A cintura nunca apareceria, e
o culpado pareceria ser a regra da cintura.

---

## A fundura da batalha já é uma faixa nomeada, e isso não é coincidência

A grade de combate já guarda fundura, e ela é **0 seco, 1 poça, 2 fundo**
(`BattleTypes.h`). O mundo entrega `EWorldFeatureKind::DeepWater` e
`ShallowWater`, que `ArenaFromWorld` traduz em `ECellProperty::Water` e
`ShallowWater`.

Então já existem **duas** representações de fundura neste jogo: número contínuo
no traçado (as travessias trazem `fundura: 86`) e faixa nomeada na batalha. A
feature precisa dizer qual delas o assado carrega — e essa é decisão do usuário,
não do executor.

---

## Aceite

**O MESMO rio, largo em toda a extensão, atravessa a pé num trecho e obriga a
nadar noutro** — porque a fundura mudou, e a largura não.

E o poço da queda **responde diferente no centro e na borda**: a fundura varia
dentro dos 886, o que é a única forma de "ler o poço pela fundura" ser uma
leitura e não uma constante.

Nas duas frases o que prova é a **diferença dentro da mesma água**. Um rio raso e
um rio fundo respondendo diferente não prova nada: a largura deles também
difere, e a fórmula antiga acertaria igual.

---

## Decisões que são do usuário

Nenhuma delas é escolhida aqui. Enquanto não houver resposta, a caixa da tarefa
que depende dela fica `[⛔]`.

1. **Onde fica a cintura.** As âncoras medidas são três, e elas discordam:
   `WadableDepthUnits() = 100` (um metro, o limiar do a-pé do traçado), a
   meia-altura da cápsula do jogador (**88**), e a fundura das 30 travessias de
   vau de hoje (**todas abaixo de 94**). Escolher 100 mantém as travessias;
   escolher 88 reclassifica algumas. **É número de produto.**

2. **A fundura é dado do gerador ou faixa nomeada?** Número contínuo por ponto
   (como as travessias já trazem) ou uma faixa `seco / vau / cintura / fundo`
   assada direto. O contínuo deixa a faixa ser calculada por quem lê e sobrevive
   a mudar o limiar; a faixa nomeada é a forma que a batalha já usa e não pede
   limiar em lugar nenhum. **Uma só das duas, nunca as duas.**

3. **A carta pode mudar de números?** Uma fundura de verdade reclassifica
   travessia: um rio largo e raso que hoje sai balsa pode virar vau.
   `ChartConformanceTest` afirma **30 vaus, 25 balsas, 0 pontes, 56 ao todo**, e
   o zero das pontes está comentado como *"uma medição, não uma ausência de
   medição"*. Se a carta pode mudar, os números novos são o gabarito novo — e
   isso é decisão de produto, igual à P10.

4. **A queda ganha degrau na rocha?** É o que tira o poço de prato, e mexe no
   relevo — que é a camada mais baixa de todas (§14: rocha → água → região →
   relevo → trilhas → solo). Um degrau na rocha muda a costa, as vilas e as
   trilhas de tabela. **Quanto de degrau, e se ele entra, é do dono.**

5. **Quem mais passa a ler fundura.** O traçado é obrigatório (é ele que tem a
   estimativa a matar). Batalha, pesca, navegação e o campo de treino podem
   passar a ler — ou não, e a fundura fica sendo só do andar. Cada leitor a mais
   é um contrapeso a mais.
