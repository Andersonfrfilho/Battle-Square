# Mãe Natureza — o mundo se equilibra, e tem rosto

**Escrito em 31/08/2026.**

## Por que um controlador

Os prazos de rebrota são **malha aberta**: eles supõem uma taxa de corte. Se
aparecerem 300 jogadores em vez de 100, ou se ninguém jogar por um mês, o mundo
sai do lugar e nada percebe.

A Mãe Natureza fecha a malha. Em vez de supor, ela **mede** o censo da região e
corrige.

## A forma: pura, e por isso testável

Recebe o CENSO (quanto de mata, pedra, água, quantos pets de cada espécie) mais
as FAIXAS-ALVO, e devolve as CORREÇÕES. Sem mundo, sem tempo, sem banco.

Assim "a mata caiu a 40% e a rebrota acelerou" vira teste headless, em vez de
algo que só se descobre jogando um mês.

## O que ela corrige

- **Mata e pedra** — acelera ou desacelera a rebrota.
- **Espécies de pet** — e este é o caso mais interessante: se todo mundo captura
  o mesmo bicho, ele fica raro, e ela ou repovoa ou faz a espécie MIGRAR para
  outra parte da ilha. Conteúdo emergente sem conteúdo escrito.
- **Água e lama** — o ciclo que a cadeia de secagem já tem.

## Três cuidados, e é onde esse tipo de sistema estraga

**Age devagar e FORA DE VISTA.** Árvore brotando na frente do jogador é mágica,
e mágica quebra o mundo que ela existe para sustentar.

**DELATA quando age.** Se ela sempre conserta, ninguém descobre que os números
de base estão errados — o sistema vira o tapete debaixo do qual o
desbalanceamento some. Cada intervenção é registro:
*"mata do norte a 38%, rebrota acelerada"*. É o instrumento que diz que o corte
livre está generoso demais.

**O jogador VÊ.** Regra deste projeto: o que existe e não aparece não existe. Se
a floresta sofre, o guarda diz e o poste da praça diz. A pressão da natureza
vira informação que dá para responder, em vez de mão invisível que dá e tira.

## O que ela NÃO faz

**Não pune o jogador.** Se a mata cai e ela responde deixando os pets fracos ou
o clima hostil, o jogador sente castigo sem entender a corrente.

A resposta dela é sobre o **mundo** — rebrota, migração, escassez. A resposta
sobre o **jogador** é do guarda florestal, que é uma pessoa e pode explicar.

## O panteão

A Mãe Natureza é a primeira de vários. **A regra que impede o panteão de virar
enfeite: um deus só existe se tiver um NÚMERO.** Sem domínio mensurável, é lore
fingindo ser sistema.

E a maioria já existe — falta o rosto:

| deus | domínio | o que já está construído |
|---|---|---|
| **Mãe Natureza** | mata, recursos, população de espécies | rebrota, censo |
| **do tempo e da fúria** | clima e eventos | terremoto, furacão, tsunami, chuva |
| **do comércio** | preço e demanda | a demanda finita do comerciante |
| **da justiça** | procurados, pressão policial | lista de procurados, recompensa |

Dar rosto a um controlador é o que o torna legível: o jogador não lê um gráfico
de estoque de madeira, mas entende que a Mãe Natureza está brava.


## O laço: ela amortece, nós equilibramos

**Decidido em 31/08/2026.** A Mãe Natureza não substitui o desenho — ela compra
tempo e **produz a evidência** de onde o desenho está errado.

> jogadores jogam → ela absorve o tranco e mantém o mundo jogável → cada
> intervenção dela vira REGISTRO → lemos o registro e ajustamos os números-base

**Ela é o amortecedor. O registro é o instrumento. Nós somos o equilíbrio.**

É por isso que "ela DELATA quando age" deixa de ser um cuidado e vira a peça
central: sem o registro, equilibrar depois dos jogadores é adivinhar depois dos
jogadores.

### Ajuste a TORNEIRA, nunca o balde

Mudar o quanto o mundo dá de agora em diante é ajuste. Mexer no que o jogador
já ganhou é confisco — e é sentido exatamente como o castigo que decidimos
evitar.

| torneira (pode mexer) | balde (não se toca) |
|---|---|
| prazo de rebrota | a coleção dele |
| demanda do comerciante | os atributos que ele treinou |
| preço da academia | as especialidades escolhidas |
| prêmio do ranking | o que ele já coletou |

Um jogador que investiu numa estratégia e a viu ser cortada sente punição, não
balanço — mesmo quando o corte é justo.

### O laço precisa ser CURTO

Se o reequilíbrio vem em três meses, os primeiros jogadores viveram três meses
num mundo torto. E são justamente eles que decidem se o jogo tem futuro.

Disso decorre uma exigência técnica: **todo número de balanço vive em
CONFIGURAÇÃO, não em código.** Mudar rebrota, demanda ou preço tem de ser editar
um arquivo — não recompilar e publicar.

### E é assim que os números-chute são resolvidos

O roteiro de verificação lista os números que são palpite meu: a proporção dos
três desfechos da lama, os prazos de congelamento, a esquiva por reflexo, os
segundos por ponto de treino, o 200% da luz. **Nenhum tem base** — existem para
o sistema rodar.

Equilibrar depois dos jogadores não é plano B para eles. **É o único jeito de
acertá-los.**

## Perguntas em aberto

- **Dá para agradar um deus?** Oferenda e santuário dão agência ao jogador sobre
  o equilíbrio — e são sistema novo.
- **Os deuses discordam?** Comércio quer corte; Natureza quer mata. A tensão
  entre eles é conteúdo de graça, e é armadilha se virar aleatoriedade.
- **A faixa-alvo é por ilha ou por região?** Por ilha é simples; por região deixa
  o norte devastado e o sul intacto, o que é mais interessante e mais caro.
