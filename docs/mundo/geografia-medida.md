# Geografia medida — os números que vieram do mundo real

**Escrito em 31/08/2026**, depois de comparar o nosso traçado com a literatura
de rios, trilhas e cachoeiras.

Este documento existe por um motivo só: **as próximas gerações vão mexer nestes
números.** Sem saber de onde eles vieram, a mexida vira gosto — e gosto não
sobrevive a uma revisão. Cada linha abaixo diz o valor, a fonte e, quando
houve, o que a nossa medição achou de errado.

---

## 1. Rios

### Comprimento de onda do meandro = 11 larguras de calha

Leopold mediu rios reais e achou onda de **10 a 14 larguras de canal**. Um rio
largo faz curva aberta; um estreito faz curva apertada. A razão é a mesma em
qualquer escala, e é por isso que ela vale para uma ilha inventada.

**O que estava errado:** o nosso rio tinha um número FIXO de curvas (1,15 a
1,85) do início ao fim, independente de comprimento e de largura. Alargar o rio
não abria as curvas — ele passava a curvar apertado, que é o contrário do que a
água faz.

```
ondas = comprimento_do_percurso / (largura_cheia * 11)
```

### Sinuosidade entre 1,5 e 2,0

Sinuosidade é o comprimento do curso dividido pela distância em linha reta
entre as pontas. **Abaixo de 1,5 o rio é considerado reto; acima, meândrico.**

**O que a medição achou, duas vezes:**

| momento | sinuosidade | veredito |
|---|---|---|
| antes | 1,26 – 1,56 | reto, pela literatura |
| depois da onda de Leopold | 2,00 – 2,65 | tortuoso demais |
| com a amplitude corrigida | 1,31 – 1,85 | dentro da faixa |

A lição que fica: **comprimento de onda e amplitude são dois botões para o
mesmo efeito.** Consertei um e o outro ficou sobrando — troquei um exagero por
outro sem perceber, porque não remedi. Mexer num obriga a remedir o outro.

### Raio de curva = 2 a 3 larguras

Ainda NÃO aplicado. Fica registrado porque é a próxima checagem: com o
comprimento de onda certo, o raio de curva é consequência da amplitude, e vale
medir antes de mexer.

### A calha é fração do raio da ilha

Não são unidades escritas à mão. Com 170 fixos numa ilha de 1,4 km, o rio tinha
**1,7 m** — um córrego que o passo do traçado de trilha nem enxergava, o que
escondeu o defeito das pontes por várias voltas.

---

## 2. Trilhas

### Declive sustentável máximo = 10%

É o limite de quem constrói trilha de verdade: acima disso a água desce pelo
caminho e o caminho vira valeta.

**O que a medição achou: 607%.** A trilha subia o barranco da cidade de frente,
como ninguém sobe um morro.

### A regra da metade

**A trilha nunca passa de metade do declive da encosta que ela contorna.** Se o
morro tem 16%, a trilha tem no máximo 8%. É isto que faz o caminho abraçar a
curva de nível em vez de cortá-la, e é o segundo motivo pelo qual trilha de
montanha serpenteia.

### O limite ENCARECE, nunca proíbe

Proibir criaria lugar inalcançável a pé, e a regra da região é que todo destino
se alcança andando. Encarecer faz o traçado achar o ziguezague sozinho.

### O que a trilha deixou de ser

Com o declive sustentável, a trilha **não é mais o caminho mais barato** entre
duas pontas: ela é mais longa e mais cansativa que a linha reta, de propósito.
Trilha de montanha real faz essa troca — quem sobe pelo ziguezague anda três
vezes mais que quem sobe de frente.

Isso quebrou um teste nosso que afirmava "a trilha custa menos que a reta". A
asserção certa é a SINUOSIDADE. Manter a antiga teria me feito desfazer a
melhoria para o teste voltar ao verde.

### 🔴 O defeito que continua aberto

**A grade do traçado é mais grossa que o barranco.**

| medida | valor |
|---|---|
| altura do planalto | ~2.800 unidades |
| largura da faixa do barranco | 2.520 unidades |
| percurso necessário para 10% de declive | **28.000 unidades** |
| passo da grade do traçado | 1.680 unidades |
| casas de grade dentro da faixa | **1,5** |

Não cabe ziguezague em uma casa e meia. O declive máximo continua em ~590%, e
**não é o custo que está errado: é a resolução.** É a mesma família do rio que
era mais estreito que o passo — quem amostra grosso não vê o que é fino.

Três saídas, e a escolha é de projeto:

1. **Grade mais fina** no barranco. Corrige de verdade, e custa muito mais
   Dijkstra.
2. **Barranco mais largo** (faixa de ~28.000 em vez de 2.520). Deixa de ser
   barranco e vira encosta.
3. **Aceitar que ali é ESCALADA, não trilha** — e dizer isso na tela. É o que
   um penhasco real é.

---

## 3. Cachoeiras

Da literatura de knickpoint e poço de queda:

- **A profundidade do poço é proporcional à altura da queda.** Queda alta cava
  poço fundo, e o jato bate mais longe da parede.
- **O poço aprofunda dez vezes mais rápido do que alarga.** Poço de cachoeira é
  furo, não bacia — se um dia ele for desenhado, é fundo e estreito.
- **Cachoeira alta costuma ser uma ESCADA de poços**, não um degrau só: a
  parede recua cavando poços sucessivos em alturas diferentes.

**O que temos:** a queda existe com posição calculada, e a gruta ao lado. Não
há poço, e a queda é um degrau único. Nada disso está errado hoje — está
simplesmente não modelado, e é o que se ganha modelando.

**O que já está certo, e por acidente feliz:** a queda vem sempre DEPOIS do
lago, por soma e não por sorteio independente. Sorteados à parte, um dia sairia
cachoeira no meio do lago — água caindo dentro de água parada.

---

## 4. Florestas e clareiras

Da ecologia de paisagem:

- **O efeito de borda depende do tamanho E da forma da mancha**, não só do
  tamanho. Mancha recortada tem até **50% mais borda** que uma retangular do
  mesmo tamanho.
- Borda é onde a estrutura muda — é o lugar mais legível de uma floresta, e o
  mais rico.

**O que isso diz para nós:** nossas manchas são QUADRADAS, e por isso têm o
mínimo de borda possível. Uma mancha recortada leria como floresta de verdade e
daria mais lugar reconhecível pelo mesmo custo de área.

Não mexi ainda, e é a próxima melhoria barata desta lista.

---

## 5. O princípio que atravessa tudo

**Número absoluto escolhido quando só existia um tamanho é a armadilha mais
cara deste projeto.** Ela apareceu, medida, nestes lugares:

| onde | o que aconteceu |
|---|---|
| anéis das peças da ilha | ficaram para trás quando o raio cresceu |
| cinta de praia | deixou de caber num pedaço do mundo |
| calha do rio | 1,7 m numa ilha de 1,4 km |
| busca da gruta | procurava dentro do lago depois que o rio alargou |
| comprimento do lago | virou bolha mais larga que comprida |
| campos de treino | os cinco cabiam num ponto |
| passo do traçado de trilha | mais grosso que o barranco (ABERTO) |

E o corolário, que custou duas voltas: **relativo não basta — tem de ser
relativo à coisa certa.** Escalei a busca da gruta pela largura do LAGO, e a
gruta é da CACHOEIRA, onde o rio já é estreito.

---

## 6. Como remedir

O despejo `Saved/IslandMap.json` sai do teste
`BattleSquare.IslandMap.Dump` e carrega tudo: alturas, rios com largura ponto a
ponto, trilhas com ALTURA em cada ponto, córregos, fontes, uso do solo,
passagens e as plantas das cavernas.

```bash
UnrealEditor BattleSquare.uproject -ExecCmds="Automation RunTests BattleSquare.IslandMap;Quit" -unattended -nullrhi
```

**A altura vai junto de cada ponto de trilha de propósito.** Remedir o declive
reamostrando a malha do mapa daria o declive da MALHA, não o da trilha — a
malha anda 1.555 unidades por casa e o barranco tem 2.520 de largura.

## Fontes

- Leopold & Langbein, *River Meanders* (1966) e *River meanders and channel
  size* — comprimento de onda, raio de curva, sinuosidade.
- American Trails, *Building Sustainable Trails* — declive sustentável, regra
  da metade, reversões de declive.
- Scheingross & Lamb, *A Mechanistic Model of Waterfall Plunge Pool Erosion*
  (2017); *Morphometry of plunge pools and retreat mechanism of waterfall*
  (2020).
- *Edge effects in forest patches* (2020); Franklin & Forman, *Creating
  landscape patterns by forest cutting* (1987).

---

## 7. Adendo de 01/09/2026 — a bacia, a corredeira e o poço

### A bacia precisa de TRÊS ordens para ler como raiz

Um tronco com dois galhos entrando no MESMO ponto desenha um **Y**, e Y não é
raiz. O que faz o desenho virar bacia são duas coisas juntas:

1. **Ordens sucessivas** — fiapo entra em galho, galho entra em tronco.
2. **Junções escalonadas** — cada galho encontra o tronco num raio diferente.
   Dois galhos no mesmo raio desenham uma flecha.

Hoje: 21 cursos, 7 por monte (1 tronco + 2 galhos + 4 fiapos), calha de
962 / 1.636 / 2.782 por ordem.

E o galho quase não serpenteia, o que está **certo**: cabeceira de rio real é
reta — o meandro nasce no curso baixo, onde a inclinação cai.

### A corredeira sai do RELEVO, não de uma faixa escolhida

Onde o leito desce mais de 4%, a água quebra. Perguntar ao terreno é o que faz
a corredeira aparecer no lugar certo mesmo quando alguém mexer no relevo.

**O defeito que isso expôs, e é o de sempre:** comparei a largura do trecho com
a calha do rio BASE para decidir "isto é lago". Um tronco é 2,9 vezes mais
largo que a base só por ser de ordem 3 — então todo tronco parecia lago, e a
ilha inteira ficou sem uma corredeira. A comparação certa é com a calha DAQUELE
curso.

Terceira vez que escalo pela coisa errada. **Relativo não basta.**

### A cachoeira precisava de um degrau no TERRENO

O poço saía com meio metro de fundo, e a causa era que a queda não descia nada:
ela existia no modelo de água e o relevo não sabia dela. **Rótulo em chão
plano** — mesma família do ator sem malha, que passa em todo teste de lógica e
não existe na tela.

`GroundHeightAt` agora soma um degrau local em cada queda de tronco.

### E eu li a fonte errado

Escrevi um teste afirmando que **o poço é mais fundo que largo**, citando o
"dez vezes" da literatura. O dez é uma razão de **velocidade de erosão** — a
incisão vertical supera a lateral — e não a forma do buraco. Poço é mais largo
que fundo, como qualquer poço.

**O teste reprovou o código, e quem estava errado era o teste.** Vale registrar
porque asserção mal lida é o defeito mais difícil de achar: ela nasce verde.

### E L-042 apareceu em código de PRODUÇÃO

`Tracar()` existia em `TrailLayout.cpp` e em `LandUseLayout.cpp`, os dois em
namespace anônimo. O unity build junta os arquivos e os dois viram sobrecarga
que difere apenas no retorno.

**A sonda `audit_test_helper_names.sh` não pega isto: ela olha testes.** Nome de
helper específico vale em todo lugar, não só em teste.

### O mapa é fixo, e isso é uma otimização que não estava sendo usada

`RegionLayout::Plan` e `FreshWater::Plan` eram remontados **uma vez por aresta**
do Dijkstra das trilhas: o custo do passo pergunta se o ponto está na rampa, a
rampa pergunta onde é a cidade, e a cidade recalculava a região inteira — com
os rios junto, por causa do Mercado do Lago.

Milhões de arestas, uma montagem de mundo em cada. Hoje são cálculo único.

**O que continua faltando:** "uma vez por processo" ainda leva minutos. Para um
mapa fixo, o certo é **assar** — calcular fora e embarcar o resultado.
