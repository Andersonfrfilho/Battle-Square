# Inventário dos biomas — o que entra na tela, e por quê

Este arquivo existe porque o mundo estava **aleatório**: as seis regiões
sorteavam do MESMO saco de 27 espécies, e só a proporção mudava. Geleira, mata
e pântano recebiam o mesmo carvalho e o mesmo pinheiro; deserto e vulcão
recebiam a mesma pedra. Proporção diferente sobre o mesmo elenco não faz
lugar diferente — faz o mesmo lugar mais cheio ou mais vazio.

A regra deste arquivo: **cada bioma tem elenco próprio, escrito aqui antes de
existir em código.** Nada entra na tela sem linha nesta tabela.

Fonte da verdade em código:
- elenco por bioma → `Public/Environment/BiomeFlora.h`
- proporção por papel → `PresencaDe` em `Private/Environment/ForestBackdrop.cpp`
- cor por papel → `Public/Environment/ScenaryPalette.h`

Estado: `✅ na tela e com teste` · `👁 no código, falta o olho humano` · `⛔ ainda não existe`

---

## O elenco disponível

O pacote traz 38 assets em `Content/Environment/Nature/`. Os que valem como
prop são 27 — o resto é material e textura. Não há coqueiro, não há cacto, não
há árvore morta. Onde falta a malha, a silhueta se constrói **inclinando,
esticando e recolorindo** o que existe; inventar asset que não veio é o que
antes produzia "bloco quadrado".

| Faixa | Espécies |
|---|---|
| Rasteira | `grass_large` `grass_leafs` `flower_redA` `flower_yellowA` `mushroom_red` |
| Borda | `plant_bushSmall` `plant_bush` `plant_bushLarge` `stump_round` `log` `rock_smallA` `rock_smallD` |
| Mata | `rock_largeA` `rock_largeC` `rock_tallD` `rock_tallA` `tree_pineSmallA` `tree_pineSmallC` `tree_oak` `tree_blocks` |
| Dossel | `tree_pineRoundC` `tree_pineRoundA` `tree_default` `tree_thin` `tree_tall` `tree_pineTallA_detailed` `tree_pineTallB_detailed` `tree_pineTallC_detailed` |

---

## Bioma 1 — FLORESTA (o de casa) 👁

O bioma em que o jogador nasce, e o único que o jogo mostrava bem. É a
referência: os outros cinco se afastam DELE, não de um ideal abstrato.

**Silhueta pretendida:** copa cheia e arredondada, chão coberto, três verdes
distintos empilhados (capim, arbusto, árvore).

| Papel | Espécies DESTE bioma | Proporção | O que precisa dizer |
|---|---|---|---|
| Chão | disco pintado `GroundCover` | — | verde de grama, mais escuro que a folha |
| Rasteira | `grass_large` `grass_leafs` | 100% | textura logo atrás da grade |
| Enfeite | `flower_redA` `flower_yellowA` `mushroom_red` | 100% | os únicos pontos de cor quente |
| Arbusto | `plant_bushSmall` `plant_bush` `plant_bushLarge` | 100% | o volume que separa tabuleiro de mata |
| Tronco | `stump_round` `log` | 100% | o que se derruba e o que se sobe |
| Pedra | `rock_smallA` `rock_smallD` `rock_largeA` `rock_largeC` | 100% | pedra ARREDONDADA — floresta não tem lâmina |
| Árvore (mata) | `tree_oak` `tree_blocks` `tree_pineSmallA` `tree_pineSmallC` | 100% | folhosa em primeiro plano |
| Árvore (dossel) | `tree_pineRoundA` `tree_pineRoundC` `tree_default` `tree_tall` | 100% | copa REDONDA fechando o fundo |
| Relevo | — | — | floresta é plana; o relevo vem da montanha atrás |

**Fora do elenco, de propósito:** `rock_tallA` `rock_tallD` (agulha de pedra é
vulcão), `tree_thin` (é praia e pântano), as três `tree_pineTall*_detailed`
(é geleira). Eram justamente elas que faziam a floresta parecer as outras.

---

## Bioma 2 — PRAIA ⛔

**Silhueta pretendida:** rampa clara que desce até a espuma, poucos troncos
tortos, nada alto tapando o mar.

| Papel | Espécies DESTE bioma | Proporção | O que precisa dizer |
|---|---|---|---|
| Chão | `BeachSand` / `WetSand` / `WaterFoam` | — | três faixas, e a do meio é a que molha |
| Rasteira | `grass_leafs` | 12% | capim de duna, ralo |
| Arbusto | `plant_bushSmall` | 8% | moita baixa, longe da água |
| Tronco | `log` | 20% | madeira trazida pela maré |
| Pedra | `rock_smallA` | 35% | seixo, nunca paredão |
| Árvore | `tree_thin` inclinada ao mar | via `ShoreTrees` | silhueta de coqueiro por inclinação |

**Fora do elenco:** toda copa fechada. Praia com dossel vira floresta com areia.

---

## Bioma 3 — DESERTO ⛔

**Silhueta pretendida:** duna curva, sombra longa, pedra chapada. Nada verde.

| Papel | Espécies DESTE bioma | Proporção | O que precisa dizer |
|---|---|---|---|
| Chão | `DesertSand` | — | areia, e o relevo é o assunto |
| Relevo | 14 montes, 26 casas de largura, 3 de altura | — | duna LARGA e baixa — a curva é o bioma |
| Arbusto | `plant_bushSmall` | 6% | mato seco, quase nenhum |
| Tronco | `stump_round` | 25% | toco ressecado |
| Pedra | `rock_largeA` `rock_smallD` | 70% | mesa e seixo — pedra CHAPADA |
| Árvore | nenhuma | 0% | deserto sem árvore é o ponto |

**Fora do elenco:** `rock_tall*` (agulha), capim, flor.

---

## Bioma 4 — GELEIRA ⛔

**Silhueta pretendida:** agulhas escuras contra o branco. Vertical, esparso.

| Papel | Espécies DESTE bioma | Proporção | O que precisa dizer |
|---|---|---|---|
| Chão | `GlacierIce` | — | branco azulado |
| Relevo | 10 montes, 15 casas, 3.6 de altura | — | gelo QUEBRADO: monte alto e estreito |
| Tronco | `log` | 10% | tronco caído, meio enterrado |
| Pedra | `rock_largeC` `rock_tallD` | 55% | pedra saindo do gelo |
| Árvore (dossel) | `tree_pineTallA_detailed` `tree_pineTallB_detailed` `tree_pineTallC_detailed` `tree_thin` | 18% | conífera ALTA e estreita, só ela |

**Fora do elenco:** folhosa, flor, capim, arbusto. Carvalho na neve foi o
defeito mais visível do elenco único.

---

## Bioma 5 — VULCÃO ⛔

**Silhueta pretendida:** pedra sobre pedra, agulha, brilho de lava. Nada vivo.

| Papel | Espécies DESTE bioma | Proporção | O que precisa dizer |
|---|---|---|---|
| Chão | `VolcanicRock` | — | basalto quase preto |
| Relevo | 8 montes, 12 casas, 2.4 de altura | — | monte curto e íngreme |
| Tronco | `stump_round` `log` | 15% | madeira QUEIMADA (cor `DeadWood`) |
| Pedra | `rock_tallA` `rock_tallD` `rock_largeC` | 85% | agulha — é aqui que a lâmina cabe |
| Árvore | nenhuma | 0% | — |

---

## Bioma 6 — PÂNTANO ⛔

**Silhueta pretendida:** água parada entre troncos finos, cogumelo, nada de flor.

| Papel | Espécies DESTE bioma | Proporção | O que precisa dizer |
|---|---|---|---|
| Chão | `SwampMud` + poças `SwampWater` | — | lama, e a poça reflete |
| Rasteira | `grass_large` | 90% | capim alto na água |
| Enfeite | `mushroom_red` | 60% | cogumelo SIM, flor não |
| Arbusto | `plant_bush` `plant_bushLarge` | 70% | moita densa |
| Tronco | `stump_round` `log` | 100% | tronco apodrecendo — o mais denso de todos |
| Pedra | `rock_smallD` | 18% | pouca pedra: é chão mole |
| Árvore | `tree_thin` `tree_tall` `tree_blocks` | 25% | tronco FINO e alto, copa rala |

**Fora do elenco:** `flower_redA` `flower_yellowA` (jardim no pântano),
conífera, carvalho.

---

## O que sai da tela

| Item | Por que sai |
|---|---|
| Disco marrom de treino (`AWorldTrainingField`) ⛔ | elipse chapada pintada no chão, sem volume e sem borda: lê como falha de render, não como clareira. A REGRA de treino fica; o disco vira clareira com props que dizem o que ela treina |

---

## Ordem de execução

Um bioma por vez, e o olho humano entre eles. Fazer os seis de uma vez foi o
que produziu "montanhas pontudas extremamente esquisitas": seis lugares
errados ao mesmo tempo, e nenhum jeito de saber qual ajuste consertou o quê.

1. **Floresta** — elenco próprio + tirar dela o que é dos outros
2. Praia · 3. Deserto · 4. Geleira · 5. Vulcão · 6. Pântano
