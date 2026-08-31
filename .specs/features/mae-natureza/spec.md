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

## Perguntas em aberto

- **Dá para agradar um deus?** Oferenda e santuário dão agência ao jogador sobre
  o equilíbrio — e são sistema novo.
- **Os deuses discordam?** Comércio quer corte; Natureza quer mata. A tensão
  entre eles é conteúdo de graça, e é armadilha se virar aleatoriedade.
- **A faixa-alvo é por ilha ou por região?** Por ilha é simples; por região deixa
  o norte devastado e o sul intacto, o que é mais interessante e mais caro.
