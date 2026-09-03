# A carta muda uma vez — o que existe hoje, medido

## Os dezesseis números que a carta afirma

Medidos em 02/09/2026, direto de `ChartConformanceTest.cpp`:

| o que | quantos |
|---|---|
| bosques | 9 |
| clareiras | 6 |
| fazendas | 8 |
| criadouros | 4 |
| lojas | 4 |
| acampamentos | 6 |
| pomares | 3 |
| decks | 7 |
| templos | 5 |
| ruínas | 4 |
| cemitérios | 7 |
| poços que dão água | 2 |
| **vaus** | **30** |
| **balsas** | **25** |
| **pontes** | **0** |
| travessias ao todo | 56 |

Os quatro em negrito são os que esta feature mexe. Os outros doze só se movem se
o degrau na rocha os mover — e é por isso que a M2 vem antes de tudo.

## A ilha, medida

| | |
|---|---|
| raio | 140 000 |
| assentamentos | 7 |
| cavernas | 16 |
| manchas de uso do solo | 79 |
| quedas com poço | 13 |
| meia-largura do poço | 886 |
| fundura do poço | **30 a 51** |

**O poço é um prato:** 886 de largura contra 30–51 de fundura. É isto que o
degrau na rocha conserta.

## As três âncoras da cintura, e por que elas brigavam

| âncora | valor |
|---|---|
| `TrailLayout::WadableDepthUnits()` | 100 (um metro) |
| meia-altura da cápsula do jogador | 88 |
| fundura das 30 travessias de vau | todas abaixo de 94 |

**A resposta do usuário dissolve a briga:** a cintura passa a ser **40% da
altura de quem pisa**. Deixa de ser constante e vira fórmula — um pet miúdo se
molha antes de um corpulento na mesma água, e nenhum dos três números precisa
ganhar.

## A estimativa que precisa morrer

`TrailLayout.cpp` estima fundura pela **largura da calha**:

```cpp
constexpr float FunduraSobreLargura = 0.065f;
```

O comentário dela defende a escolha:

> *"Estimar não é inventar: a alternativa seria um campo de profundidade
> separado, que é uma segunda fonte da mesma verdade (L-032)."*

**O comentário estava certo, e deixa de estar** no dia em que a fundura for
assada de verdade. Aí a estimativa vira a segunda fonte que ele temia — e as
duas concordam até o primeiro rio que alguém alargar.

Por isso a M4 a **mata**, e não a mantém "como fallback": fallback de fonte de
verdade é fonte de verdade.

## O que as pontes trazem, e não é só uma travessia nova

O usuário pediu **três materiais**: bloco, madeira, **destruída**.

A destruída é a que muda o mapa de verdade: é uma travessia que **existe e não
serve**. Ela não é obstáculo (o obstáculo é a água), não é passagem, e não é
ruína decorativa — é a promessa de um caminho que alguém já teve.

E, pela decisão F3, a ponte pode **ligar ilha a ilha** — o que ela ainda não faz
por só haver uma ilha, mas a forma precisa comportar.

## O mercado-negro, e o encontro de duas features

**Lugar** (K3) entra na carta. **Escondido** (J4) não pode ser apontado. As duas
juntas exigem a `segredos-e-a-carta`: a carta **conta** o que não mostra.

Sem isso, o mercado-negro precisaria de exceção no gabarito — e exceção no
gabarito é o começo do gabarito não valer.

## O que fica de fora, e por quê

- **Segunda ilha** — B-001, e a primeira ainda não usa o que tem.
- **Área de criminoso que MUDA de lugar** — todo lugar deste mundo é assado uma
  vez e fica. Lugar com estado que muda em jogo é camada nova.
- **Estações** — conversam com a idade do mundo, que já existe, e são feature
  própria.
