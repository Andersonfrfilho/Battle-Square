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

## Perguntas em aberto

- **Quantas vilas iniciais?** Uma por bioma, ou só nos que fazem sentido para
  começar (a praia é um bom começo; o vulcão talvez não)?
- **Os pets de região são exclusivos ou só mais comuns?** Exclusivo obriga a
  trocar; comum deixa o jogador solo completar sozinho, mais devagar.
