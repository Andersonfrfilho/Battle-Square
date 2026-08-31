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

## Perguntas em aberto

- **Quantas vilas iniciais?** Uma por bioma, ou só nos que fazem sentido para
  começar (a praia é um bom começo; o vulcão talvez não)?
- **Os pets de região são exclusivos ou só mais comuns?** Exclusivo obriga a
  trocar; comum deixa o jogador solo completar sozinho, mais devagar.
