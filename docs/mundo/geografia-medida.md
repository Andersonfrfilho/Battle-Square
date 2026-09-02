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

---

## 8. Adendo de 01/09/2026 — por que a água desenhava círculos

A pergunta foi feita três vezes e eu respondi errado duas. A causa nunca esteve
onde eu procurei.

### Os ATRATORES é que eram circulares

Espalhei os pontos de atração em coordenada polar — sorteia ângulo, sorteia raio,
dentro de uma coroa. É o jeito natural de encher um anel, a fórmula parece certa,
e **o viés não existe nela: ele existe só no resultado.** Uma rede que cresce
para dentro de atratores em coroa herda a coroa, porque os galhos param onde os
atratores acabam — e onde eles acabam é um círculo.

A correção é distribuição em **grade sacudida**: uma casa, um ponto no centro
dela, mais um empurrão de até meia casa. Uniforme por área, sem estrutura
angular nenhuma para copiar. Sorteio puro faz grumo; grade pura faz fileira; a
sacudida não faz nem um nem outro.

| | antes | depois |
|---|---|---|
| troncos chegando ao mar | 3 | **14** |
| cursos que são arco de círculo | — | 61 de 371 |
| galerias subterrâneas | 3 riscos retos | **333** |

### E o agravante foi meu: eu olhei em vez de medir

Formei impressão olhando o desenho três vezes seguidas. A conta que resolveu —
*"quantos cursos quase não mudam de raio"* — tem dez linhas e levou dez segundos.

Ela está no arquivo de regras junto com as outras duas que faltavam:
circularidade, sinuosidade e cobertura.

### O parâmetro por RAIO era a outra metade

`ponto = f(raio)` obriga todo traço a correr do centro para fora, e limita a
ramificação a abrir em ângulo — desenha espinha de peixe por mais galhos que se
acrescente. O curso virou polilinha, parametrizada por progresso medido em
comprimento andado.

E a pergunta que o mundo realmente faz é **"qual ponto do traço está mais perto
daqui"**, nunca "que ponto está no raio X". Toda a mata, as grutas, as trilhas e
o uso do solo perguntavam a segunda porque a primeira não existia.

### A emenda reta desfez o gerador

Depois de gerar a rede orgânica, costurei as cavernas com **segmentos de dois
pontos**. Consertei o gerador e recriei o defeito na costura — e ela chama mais
atenção que tudo, por ser a única coisa reta na tela.

Emenda virou caminhada com ruído coerente: cada passo mira o destino e é
empurrado pelo ruído da POSIÇÃO. Sorteio por passo daria tremor, e tremor não é
curva.

> ⚠️ **Corrigido em 02/09/2026 — a caminhada com mira também não curvava.**
> Ver §11. Este parágrafo ficou aqui porque ele foi a conclusão da época, e
> apagá-lo esconderia que ela estava errada.

### E duas redes não se fundem sozinhas

Em colonização espacial cada nó tem UM pai. As galerias de duas cavernas chegam
a se encostar no desenho e continuam sendo dois sistemas separados — quem
entrasse numa não sairia na outra. A ligação é explícita, e é pergunta de grafo
conexo, que se afirma em teste.

**As regras gerais disto viraram `~/.claude/rules/rules/geracao-procedural-de-mapas.md`.**

---

## 9. O gerador é UMA função parametrizada — `WorldBudget`

**Decidido em 01/09/2026**, e é o que permite fazer o deserto sem reescrever
nada.

Cada número morava no módulo que o usava: quantos bosques em `LandUseLayout`,
quanta água em `FreshWater`, quantas cavernas em `IslandFeatureLayout`. Fazer um
segundo bioma significaria caçar constante por constante em seis arquivos.

Agora é uma tabela só, por bioma:

| | mata | pântano | deserto | geleira | vulcão |
|---|---|---|---|---|---|
| Água sobre a terra | 6% | 14% | 1,2% | 3,5% | 2% |
| Densidade da mata | 1,0 | 1,35 | **0** | **0** | 0,35 |
| Bosques | 9 | 9 | 0 | 0 | 2 |
| Clareiras fechadas | 6 | 6 | 2 | 3 | 4 |
| Criadouros de pet | 4 | 4 | 2 | 1 | 2 |
| Fazendas por vila | 2 | 1 | 1 | **0** | 3 |
| Pomares cuidados | 3 | 1 | 1 | 0 | 0 |
| Pomares selvagens | 5 | 7 | 0 | 0 | 1 |
| Lojas de cruzamento | 4 | 4 | 4 | 2 | 4 |
| Acampamentos | 6 | 3 | 9 | 7 | 6 |

### As três regras que a tabela protege

1. **Porcentagem e contagem, nunca posição.** Onde as coisas ficam é decisão
   dos geradores, olhando o terreno. Aqui só se diz QUANTO.
2. **Fração de área, nunca unidades.** Número absoluto escolhido quando só
   existia um tamanho apareceu, medido, em sete lugares diferentes.
3. **Todo valor tem de ser verificável no despejo.** Orçamento que ninguém mede
   é desejo — a água pedia 6% e entregava 35%, e só um número na tela mostrou.

E a leitura de coluna conta a história do bioma. Se uma linha não faz sentido
lida assim, o número está errado: foi assim que "bosque no deserto" apareceu
antes de existir código para plantá-lo.

## 10. O que o mundo tem hoje, medido

| | |
|---|---|
| Água sobre a terra | 5,63% (pedido 6%) |
| Cursos de água | 82 em 5 ordens de Strahler |
| Cachoeiras | com escada de patamares, poço, pedras e subida |
| Trechos termais | 4 — onde o calor do vulcão alcança a água |
| Galerias subterrâneas | rede dendrítica ligando cavernas |
| Trilhas | 26, **nenhuma reta**, ziguezague nas encostas |
| Travessias | 29 vaus · 18 balsas · 2 pontes |
| Uso do solo | 8 fazendas · 4 criadouros · 4 lojas · 5 acampamentos · 3 pomares · 5 pomares selvagens · 7 decks · 9 bosques · 6 clareiras |

### Cada um deles é uma regra, não um enfeite

- **Loja no CRUZAMENTO** — comércio nasce onde passa gente das duas direções.
- **Acampamento perto da trilha e longe da vila** — as duas ao mesmo tempo:
  perto da vila dorme-se na vila; longe da trilha ninguém acha.
- **Pomar cuidado colado na fazenda** — é a mesma pessoa que cuida dos dois, e
  pomar leva anos: ele só existe onde alguém ficou.
- **Pomar selvagem longe de tudo** — achá-lo é sorte, e é o que faz valer a
  pena sair da trilha.
- **Criadouro longe da vila** — criar pet perto de gente é criar pet manso.
- **Deck onde a água aceita barco grande** — sem lugar de atracar, a
  navegabilidade é tabela que ninguém usa.
- **Chinampa, não Veneza** — Veneza pôs palácio sobre a água; os astecas
  puseram roça. A cidade que planta no lago come do lago.
- **Balsa acima do vão máximo de ponte** — 25 m. Acima disso a obra vira
  engenharia, e engenharia é uma civilização que este mundo não tem.
- **Água termal derivada** — o cruzamento do alcance do calor do vulcão com a
  máscara de água. Nada escolhido; consequência.


---

## 11. Adendo de 02/09/2026 — a galeria reta, e por que a mira não curva

A pergunta foi curta: *existem galerias subterrâneas retas?* A conta respondeu
antes de qualquer opinião.

### A medição

| | antes | depois |
|---|---|---|
| Sinuosidade mediana das passagens | **1,000** | 1,091 |
| Passagens retas (sin < 1,02) | **123 de 158 (78%)** | 18 de 85 (21%) |
| Maior vão riscado em linha | **126.000 un.** (≈ o raio da ilha) | — |

### A causa: duas fontes da mesma verdade, de novo

A emenda **entre cavernas** cavava com ruído. A emenda **entre bacias** riscava
dois pontos. E o comentário que explica por que uma galeria se cava estava
escrito **quarenta linhas abaixo** do bloco que riscava.

É L-032 pela enésima vez, e a forma dela aqui é a mais traiçoeira: não são duas
tabelas divergindo, é uma regra documentada ao lado do código que não a segue.

### E o erro que vale mais que o conserto

Com as duas cavando, a medição continuou dando **25 retas**. Supus que fosse
resolução — poucos passos no vão curto —, subi o mínimo de 3 para 7 passos e
remedi: **0,294. Pior.** A hipótese estava errada e só a medição mostrou.

O motivo real é estrutural: **caminhada com mira não curva.** Cada passo mira o
destino, então a mira do passo seguinte desfaz o empurrão do anterior. Os
desvios se cancelam e sobra a reta que a mira sempre apontou — e nenhuma
quantidade de passos conserta o que não é falta de passo.

O que curva é andar ao longo do eixo e sair **de lado**:

```
ponto = lerp(A, B, t) + perpendicular * ruído(ponto) * sin(t·π) * amplitude
```

O meio seno prende as duas pontas nas bocas sem precisar de mira nenhuma, e nada
recorrige o desvio, então ele não pode se cancelar.

### Reto virou parâmetro

Fratura de calcário às vezes é reta mesmo, e uma rede em que nada é reto lê tão
fabricada quanto uma em que tudo é. O defeito era a proporção, não a existência.

| bioma | fração reta | por quê |
|---|---|---|
| mata | 0,15 | — |
| pântano | 0,28 | rocha fraturada abre reta com frequência |
| deserto | 0,20 | — |
| geleira | 0,06 | basalto quase não abre |

O sorteio sai da **geometria das duas pontas**, não do índice do laço. E o teste
cobra o **teto**, nunca zero: cobrar zero congelaria uma decisão de arte num
teste, e a decisão é do bioma.

---

## 12. Adendo de 02/09/2026 — templos, ruínas e cemitérios

Três peças que não são enfeite: cada uma existe porque **a posição dela carrega
uma informação que o jogo não precisa escrever**.

### O templo se ensina pelo lugar

São cinco deuses, e a posição é a identidade de cada um: o do monte na saia da
montanha, o da água na cachoeira, o do fogo na beira da rocha queimada, o do
fundo dentro de uma caverna, Mãe Natureza **dentro** do bosque mais fechado.
Quem vê um templo sabe de quem ele é sem ler nada.

Duas amarras que valem mais que a lista:

- **O do fogo fica do lado de FORA da rocha queimada.** Dentro seria templo que
  ninguém alcança — ali o chão queima e a trilha não atravessa. Templo é lugar
  de ir (L-041).
- **O de Mãe Natureza fica DENTRO do bosque, não no lugar dele.** No centro
  exato ele limpava o coração da mata, e o coração é justamente o que ela
  governa. Um templo na floresta é uma clareira dentro dela.

E ele simplesmente não nascia: eu o colocava **antes de os bosques existirem**,
então procurava num mundo sem bosque nenhum. Quatro templos apareceram em vez de
cinco, e só a contagem mostrou.

### A ruína é a única peça que dá PASSADO

Longe de trilha e de vila; achá-la é acidente. Um lugar que já foi importante e
deixou de ser conta uma história que ninguém precisou escrever.

### O cemitério tem uma regra sanitária real

**Fica na direção oposta à da água.** Ninguém enterra rio acima de onde bebe, e
vilas fazem isso desde muito antes de saberem por quê. Aqui isso vira uma amarra
que o gerador consegue verificar — é ela que dá ao cemitério um lugar em vez de
um sorteio. Fora da clareira e perto dela: dentro seria praça, longe seria
romaria.

O **esquecido** não é o mesmo em tamanho menor: é vestígio, não serviço. Nasce
encostado numa ruína — um templo caído com um cemitério ao lado conta que ali
houve gente. Encostado, nunca em cima: mesmo chão apagaria os dois.

### E a lição está no TESTE, que reprovou desenho certo duas vezes

Ver a regra global §12. A forma curta: teste que reconstrói a decisão do gerador
mede o **desempate**, não a regra. Afirme a propriedade, na forma existencial —
"existe vila para a qual isto vale" —, e não há desempate para errar.

---

## 13. Estado em 02/09/2026

| | |
|---|---|
| Testes | **679/679** |
| Cobertura de água pedida / medida | 6,00% / 6,24% |
| Sinuosidade mediana das galerias | 1,091 |
| Templos · ruínas | 5 · 4 |
| Cemitérios | 7 de vila · 1 esquecido |
