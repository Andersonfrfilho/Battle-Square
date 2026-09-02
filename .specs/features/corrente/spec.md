# O sentido da corrente — a água deixa de ser estado e vira força

**Escrito em 02/09/2026**, a partir da caminhada de G4/T20.

## O pedido

> *"pode ser específico com as propriedades daquele local: água, doce,
> densidade, **velocidade da corrente**, venenoso, condutor, **sentido do
> movimento**"*

Hoje a água é um **estado**: a casa é molhada, e isso muda o passo. Ela não tem
para onde ir. Uma corrente com sentido empurra, carrega, e transforma
atravessar um rio numa decisão de **por onde**, não só de **se**.

## A medição que muda o tamanho disto

**O sentido já existe, e está assado.** O curso do rio é uma polilinha
ORDENADA, da nascente para a foz — `PointAtProgress(curso, t)` com `t` indo de
0 a 1. A direção do fluxo entre dois pontos é a diferença entre eles, e ela já
viaja em `FBakedRiver::PointsUnits`.

E a FORÇA também tem medida pronta: `BedGradientAtProgress` é o declive do
leito, que é o que faz a água correr depressa, e `IsRapidsAtProgress` já marca
onde ela corre demais.

**Isto não é inventar física. É carregar até a casa o que o traçado já sabe** —
a mesma forma do trabalho dos fluidos.

## As duas decisões estruturais

**Como a direção viaja.** `EBattleDirection` já tem as oito direções e já é a
linguagem do movimento no núcleo. Usá-la faz a corrente falar a mesma língua de
quem anda — e limita a oito rumos, que numa grade de casas é o que existe de
qualquer forma. **Medir antes de escolher** se oito bastam.

**A força é inteira.** O núcleo não tem float. O declive é fracionário; ele vira
um inteiro por casa, e a escala tem de ser escolhida com a mesma disciplina das
partes por mil dos fluidos.

## O que a corrente NÃO decide

- **Não decide para onde a eletricidade anda.** A corrente elétrica atravessa o
  meio inteiro; ela não desce o rio. Misturar as duas seria inventar física
  para caber num nome parecido.
- **Não muda o traçado.** O sentido é lido dele, nunca reescrito.
- **Não recalcula a direção pela geometria.** A ordem da polilinha JÁ é a
  direção. Deduzi-la de novo a partir das posições seria uma segunda verdade,
  e ela concordaria com a primeira até a primeira edição.
