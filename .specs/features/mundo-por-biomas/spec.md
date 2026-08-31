# Mundo por biomas — e o tamanho dele

**Escrito em 31/08/2026.**

## A decisão de fundo

O jogo é ONLINE, e o jogador **não nasce sempre no mesmo lugar**. Cada bioma
tem a sua vila inicial, e começar na praia, no deserto ou na montanha não é
começar em desvantagem — é começar em outro lugar.

Disso decorrem três coisas que não são óbvias:

1. **Toda vila inicial precisa do mesmo bolso de segurança.** Se "perigo cresce
   com a distância do centro da ilha", quem nasce perto da borda nasce em
   apuros. O modelo passa a ser **distância da SUA vila**, não do centro.
2. **A vila é um MOLDE, não uma obra única.** Os prédios são os mesmos em todo
   bioma — o que muda é material, paleta e o que a academia ensina. Por isso
   `VillageLayout::Plan()` é puro e parametrizável desde o primeiro dia.
3. **Os pets mudam de região.** É o que faz viajar valer, e é o que dá sentido
   a trocar pets com quem começou noutro bioma.

## Ordem: um bioma por vez

Começamos pela **floresta**, inteira: vila, mata em volta, trilhas, e o que
mais couber. Só então o próximo.

Mexer em cinco biomas ao mesmo tempo é não terminar nenhum — e a primeira
sessão jogada já mostrou o custo disso: o mundo inteiro estava mediano e a
arena, que teve atenção concentrada, ficou boa.

## O TAMANHO da ilha

Hoje o raio é **20.000 unidades = 200 metros**. É pequeno demais para o que
está desenhado: seis biomas, quatro cidades grandes, vilas iniciais e viagem
que signifique alguma coisa.

As contas, com o passo de 4 m/s do explorador:

| raio | travessia | terra | caminhada de ponta a ponta | pedaços de mapa |
|---|---|---|---|---|
| 20.000 (hoje) | 400 m | 0,13 km² | 1,7 min | 6.400 |
| 50.000 | 1 km | 0,79 km² | 4,2 min | 40.000 |
| **100.000** | **2 km** | **3,14 km²** | **8,3 min** | **160.000** |
| 200.000 | 4 km | 12,6 km² | 16,7 min | 640.000 |

**Decisão: 100.000 unidades — 1 km de raio, 2 km de travessia.**

Por que não 200.000: dezessete minutos a pé para atravessar é castigo, e o
problema deixaria de ser tamanho e passaria a ser DENSIDADE. Mundo grande e
vazio é pior que mundo pequeno e cheio — é o erro clássico do gênero.

Por que não 50.000: com seis biomas, cada setor fica com meio quilômetro de
profundidade, e ali já cabem uma vila (192 m de bolso), uma cidade e nada mais.

Com 100.000, cada setor de bioma ganha **1 km de profundidade** — vila, cidade
e mata entre elas, com folga. E crescer depois é uma linha de configuração; o
sistema de blocos não se importa com o tamanho.

## O que essa escolha DISPARA

**O mapa passa a ser por ÁREA, obrigatoriamente.** Com 160.000 pedaços, o teto
de 10.000 de `DP-mapa-01` estoura por dezesseis vezes — e o teste que avisa a
hora vai reprovar, que é exatamente o trabalho dele.

Não é defeito: é o gatilho funcionando. O mapa deixa de mostrar o planeta e
passa a mostrar o setor onde o jogador está, usando `BiomeOfSector`, que já
divide a ilha.

## Um risco a MEDIR antes de crescer

As peças da ilha hoje estão a 10.500–17.000 unidades do centro. **Se esses
números forem absolutos em vez de frações do raio**, crescer a ilha deixa
montanha, caverna e vulcão amontoados no miolo, com um quilômetro de nada em
volta.

Medir antes de mudar o raio. É barato, e o contrário é um mundo vazio que
parece defeito de geração.


## Uma ilha, uma aventura, um mapa

**Decidido em 31/08/2026.** O mundo não é uma ilha só que cresce — são ILHAS,
cada uma completa em si.

Com 1 km de raio cabem uns quinze pontos de parada: vila inicial, uma cidade
grande, cinco campos de treino, três rios com lago e cachoeira, três montanhas,
uma caverna grande, um vulcão e o anel de praia. Entre vizinhos, 200 a 400
metros — um a dois minutos de caminhada. Se cada ponto valer de 5 a 15 minutos,
**a ilha inteira dá de uma hora e meia a três horas.** Isso é aventura
completa.

E vale dizer o que a conta não diz: **o que faz aventura é DENSIDADE, não
área.** Kanto inteiro cabe num quarteirão em metros reais e ninguém o achou
pequeno. Mundo grande e vazio é o erro clássico do gênero — 3,14 km² cheios
valem mais que 12 km² com nada entre os marcos.

### O que a estrutura de ilhas resolve de graça

1. **O mapa por área.** A ilha É a área. Cada uma tem o seu mapa inteiro, e a
   divisão que `DP-mapa-01` mandava fazer deixa de ser problema — não porque o
   limite sumiu, mas porque a unidade passou a ser natural.
2. **O spawn por bioma.** Uma ilha por família de bioma dá a cada jogador vila,
   cidade, ranking e espécies próprias, sem que começar num lugar seja começar
   pior.
3. **A progressão.** Sair da ilha é o marco de "terminei aqui" — coisa que um
   mundo contínuo tem dificuldade de expressar.
4. **O motivo de trocar pets.** Se as espécies são DA ILHA, quem começou noutra
   tem o que você não alcança sozinho.

### O custo, dito na cara

**Cada ilha é conteúdo.** Uma ilha excelente vale mais que quatro medianas, e a
tentação de multiplicar antes de a primeira ficar boa é forte. Construir UMA
inteira — a floresta — e só então a segunda, reusando o molde com paleta e
espécies diferentes.

### A correção que a conta expôs

Ilhas resolvem a divisão conceitual do mapa, mas **não resolviam a contagem de
pedaços** — e eu disse que resolviam antes de fazer a conta. Com raio de 1 km e
região de descoberta fixa em 800 unidades, o mapa desenharia 160 mil pedaços.

A causa é a mesma dos anéis das peças, e o mesmo tipo de decisão velha: **800
unidades é número absoluto**, escolhido quando a ilha só tinha 200 metros.

**A região de descoberta passa a ser o raio dividido por 25.** Isso mantém
tudo constante, cresça a ilha o quanto crescer:

| raio | região | de ponta a ponta | pedaços |
|---|---|---|---|
| 20.000 (hoje) | 800 u (8 m) | 80 | 6.400 |
| 100.000 | 4.000 u (40 m) | 80 | 6.400 |
| 200.000 | 8.000 u (80 m) | 80 | 6.400 |

A fração foi escolhida para dar **exatamente as 800 unidades de hoje** com o
raio atual — mesma disciplina dos anéis: converter absoluto em fração não pode
mudar o mundo de ninguém.

**Ressalva:** mudar o tamanho da região invalida a descoberta já salva, porque
a chave da região muda de significado. Fazer isso AGORA, enquanto a descoberta
tem um dia de vida, custa nada; depois custa o mapa de todo jogador.

### Perguntas em aberto

- **Como se viaja entre ilhas?** Barco, portal, ou o marco de retorno da vila?
  A resposta muda se as ilhas são vizinhas no mesmo mundo ou lugares separados
  que se carregam.
- **Quantas ilhas no começo?** Uma boa, e depois se decide.


## A primeira região: três vilas, uma cidade e as fronteiras

**Decidido em 31/08/2026.**

**Cada assentamento tem função PRÓPRIA.** Três vilas iguais é uma vila visitada
três vezes — a segunda não acrescenta nada, e a viagem até ela vira imposto.

| lugar | função | por que ali |
|---|---|---|
| **Vila inicial** (bloco 0,0) | Centro de Recuperação e Escola do treinador | É casa. Cura de graça, e é onde `bs.Especializar` deixa de ser console. **Sem academia**, de propósito: assim a primeira viagem tem motivo |
| **Vila da academia** | treino rápido e PAGO, de alguns atributos | O primeiro ralo da economia, e o primeiro motivo de ter dinheiro |
| **Vila do mercado** | troca de pets por raridade, e o quadro de trabalhos | Onde o repetido vira alguma coisa, e onde os poderes do pet rendem |
| **Cidade grande** | a ARENA e o ranking da região, mais tudo o que as vilas têm | O clímax. Ser o melhor daqui é o que abre a fronteira |

### As fronteiras exigem o ranking

Os postos de fronteira ficam na borda e **só abrem para quem venceu o ranking
da região**. Isso transforma a região de corredor em lugar que se conquista, e
dá ao "acabou aqui" um momento exato.

E casa com a economia: o ranking já era a FONTE de dinheiro e já parava de
pagar no primeiro lugar. Agora esse mesmo primeiro lugar é a chave da porta —
o incentivo aponta para a saída no instante em que a região se esgota.

### O que isso obriga, e é bom saber antes

**Toda região inicial precisa ser autossuficiente.** Se a fronteira exige o
ranking, quem nasce na região B não visita a A antes de vencer a B — então B
precisa ter cura, escola, treino, mercado e arena. Nenhuma região pode depender
de outra para o básico.

Isso reforça a estrutura de ilhas: cada uma completa, e nenhuma é um corredor
para a próxima.

### Distâncias, com o raio de 1 km

- **Vila inicial** no centro — bloco 0,0.
- **Vila da academia** e **vila do mercado** a cerca de 400 m, em direções
  diferentes: quatro minutos de caminhada da casa, e uma da outra.
- **Cidade grande** a cerca de 700 m: longe o bastante para a chegada
  significar alguma coisa.
- **Postos de fronteira** na borda, perto dos 950 m.

Assim nenhum trecho passa de uns dois minutos a pé, e a região inteira se
percorre sem que a caminhada vire espera.


## A ilha é a unidade de SERVIDOR

**Decidido em 31/08/2026.** Cresce-se acrescentando ilhas, não enchendo uma.

Cada ilha é uma instância. "Servidor" passa a significar "ilha", e duzentos mil
jogadores são duas mil ilhas — não uma ilha lotada.

Isso já estava implícito em tudo o que foi decidido: uma ilha, uma aventura, um
mapa; espécies próprias por região; fronteira que exige o ranking. **A ilha já
era a unidade de conteúdo; ela vira a unidade de servidor também.**

### Quantos jogadores por ilha

**Recomendação: 60 a 100, com 80 como alvo.**

O número saiu de comparar com jogos reais, e ele **corrigiu para baixo** um
palpite anterior de 100 que eu tinha feito olhando só a distância de visão:

| jogo | mapa | jogadores | m² por jogador |
|---|---|---|---|
| Palworld (servidor dedicado) | ~4 km² | 32 | 125.000 |
| ARK, mapa da ilha | ~36 km² | 50 | 720.000 |
| Rust, servidor médio | ~16 km² | 150 | 107.000 |
| Rust, servidor cheio | ~16 km² | 400 | 40.000 |
| **nossa ilha a 80** | **3,14 km²** | **80** | **39.000** |
| nossa ilha a 400 | 3,14 km² | 400 | 7.850 |

**A 80 já estamos na densidade de um Rust cheio.** A 400 seríamos CINCO VEZES
mais densos que ele — e Rust é um jogo desenhado para conflito constante, que
não é o que este é.

### Por que 400 não serve

Não é limite de máquina. O combate é por turnos com commit simultâneo e custa
quase nada; o custo é replicar mundo, e ele cresce linear.

**O que quebra antes é a COLETA.** Quatrocentos jogadores tirando madeira, pedra
e barro de 3,14 km² esgotam a mata mais rápido que qualquer rebrota razoável — e
aí volta a tragédia dos comuns que o desenho de bosques e pomares existe para
evitar.

### O que salva a densidade é a CONCENTRAÇÃO

As quatro cidades puxam gente. Com 80 jogadores, se um terço estiver nas
cidades, são ~7 por cidade (viva, não cheia) e os outros espalhados por 3 km² —
o mato continua sendo seu.

É esse desenho que permite densidade de Rust sem sensação de Rust: **cidade
cheia, mato vazio.**

### O que ainda NÃO foi medido

A capacidade de CPU e de rede. Os números acima são de DENSIDADE — calculados, e
confio neles. Carga real precisa de teste com servidor dedicado, que hoje
esbarra em B-004: a engine instalada não compila `TargetType.Server`.


## As quatro ilhas, e por que essas

**Decidido em 31/08/2026. Raio de 1,4 km, 100 jogadores por ilha.**

O leque de biomas possíveis é enorme — Minecraft tem 53 só no mundo comum. A
graça não é ter muitos: é cada um significar alguma coisa. E o filtro que
importa aqui não é "que biomas existem", é **onde cada ELEMENTO mora**.

A gente decidiu que as espécies são próprias de cada região. Então elemento sem
casa é elemento que não tem onde ser comum — e três dos seis estavam assim.

| ilha | dominante | secundário | reusa o que já existe |
|---|---|---|---|
| **Floresta e Ruínas** | Planta | **Fantasma** | mata, rios, cachoeiras, pântano |
| **Pântano e Mangue** | **Água** | Planta | água doce, brejo, praia, grutas |
| **Geleira** | **Luz** | Água | neve, aurora, montanha |
| **Planalto e Penhascos** | **Ar** | Luz | montanha caminhável, serra |
| **Vulcão e Cavernas** | Fogo | **Terra** | vulcão, cavernas, labirinto, lava |

Três escolhas têm razão além da estética:

**O fantasma não ganha ilha — ganha um LUGAR dentro de uma.** Ruínas cobertas de
névoa na floresta. É o que ele é: fantasma não tem terra natal, tem lugar
assombrado. E economiza uma ilha inteira.

**A geleira é a casa da Luz** porque a AURORA já está construída — é literalmente
luz no céu. Nenhum outro candidato reusa tanto trabalho pronto.

**A Terra fica no vulcão**, pela rocha e pelas cavernas — que já existem no
código como feições, sem serem bioma de ninguém.

### O AR entrou, e `voar` mudou de dono

Acrescentado em 31/08. Fogo voando sempre foi convenção de dragão, não
característica de fogo — o dono natural de `voar` é o ar.

E o fogo não ficou sem: ganhou **`incendiar`**, que queima a casa à frente. Ela
faz PAR com `escavar`: a terra CONSTRÓI uma barreira, o fogo cria um PERIGO. Os
dois mudam a casa da frente e nenhum causa dano direto — são negação de espaço,
e é isso que os separa de atacar.

A regra que forçou isso é de ontem: **nenhum elemento fica sem skill.** Ela
existia porque a Terra era o buraco; agora ela impediu que o Fogo virasse um.

O ar também é dono de **secar**, junto com o fogo: vento seca tanto quanto
calor, e a resposta ao campo alagado passa a ter dois donos.

**Escola não precisa de casa.** Psíquica, Física, Natural e Espiritual são COMO
a criatura luta, não do que ela é feita — e por isso atravessam as ilhas. Um
psíquico de água mora no pântano porque é de água. Não existe ilha dos
psíquicos, e "Mágico" é um nome antigo que hoje é `Psiquica/Fogo`.

### O que se perde

**O deserto sai dos quatro**, e ele está construído. Volta como quinta ilha, ou
como faixa seca dentro da ilha do vulcão — terra queimada ao lado de lava é
vizinhança natural.

### O leque completo, para quando houver mais ilhas

| família | biomas |
|---|---|
| vegetação | floresta temperada, selva, taiga, bosque claro, campos, savana, floresta morta |
| água | oceano/arquipélago, pântano, mangue, costa, recife, região de lagos |
| seco | deserto de areia, deserto de pedra, cânion, dunas, salinas |
| frio | tundra, geleira, montanha nevada, banquisa |
| vulcânico | vulcão, campos de lava, fontes termais, terra queimada |
| subterrâneo | cavernas, cavernas de cristal, minas, abismo |
| altitude | planalto, montanha rochosa, penhascos |
| sinistro | ruínas na névoa, cemitério, terra assombrada |
| humano | campo e fazendas, urbano, **cidade abandonada** |

## Cidades abandonadas

**São a casa do Fantasma, e a resposta a uma pergunta que o mundo vivo abriu.**

Uma cidade abandonada não é cenário decorativo: ela é o que **um desastre
deixou**. E os desastres já existem — terremoto, furacão e tsunami foram
construídos. A cidade abandonada é a cicatriz deles.

Isso dá três coisas de uma vez:

1. **Casa para o Fantasma** sem gastar uma ilha.
2. **Destino de exploração** com motivo próprio — o que sobrou de uma cidade é
   o que ninguém levou.
3. **História sem escrever história.** O jogador entende sozinho por que ela
   está vazia, porque ele já viu o mundo tremer.

**A amarra:** ela precisa ter sido uma cidade DE VERDADE — com as mesmas partes
que as vilas vivas têm, em ruína. Uma cidade abandonada montada com prédios que
não existem em nenhuma cidade viva é cenário fingindo ser lugar, e o jogador
percebe.

O molde já existe: `VillageLayout` é puro e parametrizável. A cidade abandonada
é o mesmo traçado, com material, colisão e povoamento diferentes.

## Perguntas em aberto

- **Quantas vilas iniciais?** Uma por bioma, ou só nos que fazem sentido para
  começar (a praia é um bom começo; o vulcão talvez não)?
- **Os pets de região são exclusivos ou só mais comuns?** Exclusivo obriga a
  trocar; comum deixa o jogador solo completar sozinho, mais devagar.
