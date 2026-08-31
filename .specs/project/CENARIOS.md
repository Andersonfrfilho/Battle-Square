# Lista de cenários — o que está feito, o que está feio, o que falta

Pedido do jogador em 30/08/2026: *"o cenário ficou horrível… você pode fazer
muito melhor, igual você tinha feito com a floresta… seria uma boa você fazer
uma lista de cenários e ir ajustando todos"*.

Esta é a lista. Ela vale mais que as correções isoladas porque diz **em que
ordem** os cenários entram e **como cada um vai ser medido** — o padrão que
falhou até aqui foi ajustar um cenário sem saber quais outros herdavam o mesmo
defeito.

**Regra dos três estados:** ✅ corrigido e com teste · 👁 corrigido, falta o
olho humano no PIE · ⛔ ainda feio ou ainda inexistente.

---

## Os que já foram ajustados

| # | Cenário | O que estava errado | O que virou |
|---|---|---|---|
| 1 | 👁 **Montanha caminhável** | um CONE do motor: ponta afiada, parede reta, silhueta que não existe na natureza | pilha de 20 fatias com perfil **curvo** (côncavo embaixo, convexo no topo), 2100 de altura por 2400 de base, revestida com as pedras do Kenney |
| 2 | 👁 **Serra do horizonte** | 66 cones do motor, todos iguais, todos pontudos | instâncias de `rock_tallA` com touca de neve proporcional à altura e ao clima |
| 3 | ✅ **Trilha da montanha** | seguia a parede reta do cone — subida impossível e visualmente horrível | ela lê `RadiusAtHeight`, então acompanhou a curva nova **sozinha**: uma fonte de verdade, não duas |
| 4 | ✅ **A costura do chão** | os ladrilhos de 6400 se encostavam com zero sobreposição, e na diagonal sobrava fresta — era ali que o pet **afundava** | 4% de transbordo (256 unidades por lado), com teste que exige o transbordo |
| 5 | ✅ **Disposição da ilha** | os anéis de relevo foram calculados para uma ilha de 6000 e ficaram; com a ilha em 20000, tudo amontoado num quinto do mapa | anéis a 12000 / 10500 / 15000, validados contra toda folga de peça e todo campo de treino |
| 6 | ✅ **Mata da casa** | *"a floresta desapareceu"* — o miolo de mata tinha 2600 fixos, e um pedaço de mundo tem 6400: **nem um pedaço inteiro cabia**, então o vizinho de quem nasce já era deserto | 35% do raio da ilha (7000 hoje), com teste que mede o pedaço de nascimento **e os quatro vizinhos de lado** |

---

## Os que ainda estão feios

| # | Cenário | O defeito, dito sem panos quentes | O que ele precisa virar |
|---|---|---|---|
| 7 | ⛔ **Pântano** | ele **desenha exatamente como a mata**: `PresencaDe` não tem caso para `Swamp`, então cai na tabela da floresta. A geografia sabe que ali é brejo, o clima sabe, o mapa sabe — só a tela não | chão escuro e molhado, poça, tronco caído, junco no lugar do arbusto, e **nenhuma copa alta**: brejo é mata que não drenou, não mata fechada |
| 8 | ⛔ **Deserto** | o chão é **plano**. Areia plana com pedra em cima não é deserto, é um pátio bege | **duna**: ondulação de baixa frequência no terreno é o que faz o deserto ser deserto. Sem ela, nenhuma quantidade de pedra salva |
| 9 | ⛔ **Geleira** | plano branco com 18% de conífera. Sem escala e sem relevo | bloco de gelo quebrado, fenda, e a **aurora** que o jogador pediu — visível só de noite, só neste setor |
| 10 | 👁/⛔ **Vulcão** | o cone existe e está de pé, mas não brilha, não fumega e não tem lava visível | brasa que **acende de noite** (o papel `LavaGlow` já existe e nunca foi usado), fumaça no topo, campo de cinza em volta |
| 11 | ⛔ **Cavernas** | existem e têm labirinto, mas são caixas de pedra: sem estalactite, sem lava, sem água, e sem diferença entre a que se explora e a que só se olha | três sabores — **seca**, **de lava** (perto do vulcão) e **de água** (perto do mar) — com estalactite pendurada e uma boca que diz de longe se dá para entrar |
| 12 | ⛔ **Praia** | tabela rala de propósito, mas hoje é rala **e sem nada**: areia com pedrinha | faixa molhada mais escura na beira, espuma na linha d'água, e algo em pé — coqueiro ou tronco trazido pelo mar |
| 13 | ⛔ **Água doce** | não existe. Só há o mar da borda | rio que desce da montanha, lago no miolo, cachoeira onde o rio cruza um degrau — com trilha e gruta acompanhando, como foi pedido |
| 14 | ⛔ **Céu noturno** | o dia e a noite viram, mas o céu é vazio | lua com **fase**, eclipse lunar (que É a lua vermelha — um fenômeno, não dois), eclipse solar, estrelas e cometa |
| 15 | ⛔ **Eventos de terra** | furacão, terremoto e tsunami não existem | irmãos do clima, mas com **lugar e hora**: furacão no mar e na praia, terremoto perto do vulcão, tsunami **depois** do terremoto |
| 16 | ⛔ **Arena** | ela lê o bioma do `.ini`, não de onde o encontro aconteceu — lutar na geleira dá cenário de floresta | `EncounterLocation` chega até `ABattleArena`, e o cenário da luta é o do chão onde ela começou |

---

## A ordem, e por quê

1. **Pântano** (7) — é o único da lista que está *pronto por dentro e errado por
   fora*. Custa uma tabela e devolve um bioma inteiro.
2. **Deserto** (8) e **Geleira** (9) — os dois têm a mesma causa (terreno plano)
   e a mesma cura (ondulação no chão). Fazer juntos evita resolver duas vezes.
3. **Vulcão** (10) e **Cavernas** (11) — vizinhos de mapa e de material.
4. **Praia** (12) e **Água doce** (13) — as duas bordas d'água.
5. **Céu** (14) e **Eventos** (15) — nenhum depende de terreno.
6. **Arena** (16) — depende de tudo acima já parecer diferente entre si; antes
   disso, ligar o bioma da luta não mudaria nada visível.
