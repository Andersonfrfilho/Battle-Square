# As quatro que param — com a medição, não com a impressão

Aberto em 02/09/2026, ao chegar nelas executando `tudo-que-falta`.

O objetivo previa **três** paradas (P7, P8, P10). A medição acrescentou a
**P9**, que ele classificava como "parcial". Estava errado, e o número é o que
mostra isso.

---

## P9 — Cavernas por dentro das cachoeiras ❌ MEXE NO TRAÇADO

**Medido** (`Saved/IslandMap.json`, 16 cavernas × 13 quedas com poço):

| | unidades |
|---|---|
| caverna mais perto de uma queda | **2 710** |
| segunda mais perto | 2 711 |
| **mediana** | 3 943 |
| meia-largura do poço da queda | **886** |

Uma gruta "atrás da cachoeira" precisa cair **dentro** do poço — algo como 900
unidades. A mais próxima está a **três vezes** essa distância, e a mediana a
mais de quatro.

**Portanto não é uma consulta que falta, é a POSIÇÃO das cavernas.** Escrever
`CavernaAtrasDaQueda()` hoje devolveria "nenhuma" para as dezesseis, e um
recurso que não existe em lugar nenhum do mapa não é um recurso — é uma função
que só o teste chama.

O objetivo dizia "a gruta é peça separada, sem ligação com a queda", e eu li
isso como falta de ligação. **É falta de proximidade**, e proximidade é decisão
do gerador.

---

## P7 — Nadar quando a água passa da CINTURA ❌ MEXE NO TRAÇADO

**Medido:** cada ponto de curso guarda **três** números — `x`, `y` e
meia-largura. **Não há fundura por ponto.** O campo `fundo` existe por RIO, e
vale `0` nos cinco primeiros.

Hoje `WaterFooting` decide vau/fundo **pela largura**, atravessando
`FreshWater::NavigabilityForHalfWidth`. "Passar da cintura" é uma pergunta sobre
**fundura**, e a fundura não está assada.

Dá para inventar uma: fundura = f(largura). **E aí seriam duas verdades sobre a
mesma água** — a do traçado e a minha —, que concordariam até a primeira vez que
alguém alargasse um rio. É a invariante 4, e este projeto já pagou por ela
(L-032, L-033).

**O que precisa mudar:** o assado ganha fundura por ponto, e quem a decide é o
gerador.

---

## P8 — O poço se lê pela FUNDURA ❌ MEXE NO TRAÇADO

**Medido:** os 13 poços têm meia-largura **886** e fundura entre **30 e 51**.
Todos os 13 são mais largos que fundos — o que está certo, e é o que a
literatura diz.

Mas "ler o poço pela fundura" pede que a fundura **varie**, e ela varia de 30 a
51 num raio de 886: o poço é praticamente um prato. Ler por essa fundura daria
sempre a mesma resposta.

⚠️ **Foi aqui que eu já errei uma vez**, e o registro fica: escrevi um teste
afirmando "mais fundo que largo" lendo o "dez vezes" da literatura como forma do
buraco. Ele é razão de **velocidade de erosão**, vertical sobre lateral. O teste
reprovou código certo.

---

## P10 — Coisas ESCONDIDAS ❌ MUDA O GABARITO DE ACEITE

`ChartConformanceTest` afirma número por número o que a carta promete: 5
templos, 4 ruínas, 7 cemitérios, 2 poços que dão água, e assim por diante.
Esconder qualquer coisa **reprova**.

Isso não é um defeito do teste — é ele fazendo o trabalho dele. O gabarito é a
carta, e a carta hoje é uma promessa de **completude**.

**Para haver segredo, a carta precisa dizer o que ela NÃO mostra**: uma
contagem de "coisas fora da carta", que o teste passa a exigir também. Sem isso,
o teste que protege a feature passa a proibir o design.

Isso é decisão de PRODUTO antes de ser de código: quantos segredos, se a carta
os conta, e se contá-los ainda os deixa ser segredo.

---

## O que sobrou, e por que a parada não é fracasso

Todas as quatro esbarram numa das duas invariantes que este objetivo declarou
antes de começar. Forçá-las custaria mais do que deixá-las: as três primeiras
abririam uma segunda fonte de verdade sobre a água, e a quarta desativaria o
único teste que garante que o mundo é o que a carta promete.

**Cada uma vira feature própria**, com o traçado ou o gabarito no escopo — que é
justamente o que uma tarefa de construção não pode ter dentro.

## Modelo recomendado quando forem abertas

| feature | modelo |
|---|---|
| Fundura por ponto no traçado (P7 + P8) | `opus` 🧠 — mexe no gerador |
| Cavernas ligadas às quedas (P9) | `opus` 🧠 — mexe na colocação |
| Segredos e o que a carta não mostra (P10) | `opus` 🧠 — muda o gabarito |
