# Achado — travessias duplicadas no traçado

**Medido em 03/09/2026**, durante a M6, e **não consertado nela**.

## O que há

**Cinco coordenadas com mais de uma travessia**, em 37:

| onde | quantas | tipos |
|---|---|---|
| (−89283, −57373) | 3 | balsa, balsa, balsa |
| (13642, −38790) | **4** | vau ×4 |
| (14031, −39053) | **4** | vau ×4 |
| (54104, −39194) | 2 | vau ×2 |
| (54489, −39579) | 2 | ponte ×2 |

## Por que não é desta feature

**Vau e balsa duplicam também**, e os dois existem desde muito antes da
`a-carta-muda-uma-vez`. A causa é o traçado marcar uma travessia por TRILHA que
cruza a água — e várias trilhas cruzam o mesmo rio no mesmo lugar, que é
justamente o que faz aquele lugar ser um bom ponto de travessia.

A M6 é sobre **material e estado de ponte**. Consertar duplicação aqui seria
alargar a tarefa por conta própria.

## Por que também não é urgente

A duplicata não quebra o jogo: são travessias no mesmo ponto, com o mesmo tipo e
a mesma fundura. O que ela contamina é a CONTAGEM — e a contagem é exatamente o
que a **M10** vai reescrever de uma vez, com a carta.

## O que decidir quando for tratado

1. **Deduplicar por coordenada** é o óbvio, e muda os números da carta.
2. **Ou aceitar** e dizer que travessia é por trilha, não por lugar — e aí a
   carta conta "passagens", não "travessias", e o nome no gabarito muda.

A segunda é mais barata e talvez mais correta: quatro trilhas cruzando no mesmo
vau são quatro passagens de verdade. **É decisão do dono**, e por isso está
aqui em vez de consertada.

⚠️ Se a M10 for escrita sem decidir isto, o gabarito novo vai gravar contagens
que incluem duplicatas — e ninguém saberá disso ao lê-lo.
