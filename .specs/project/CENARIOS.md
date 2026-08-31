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
| 7 | 👁 **Pântano** | desenhava **exatamente como a mata**: `PresencaDe` não tinha caso para `Swamp` e caía na tabela da floresta. A geografia sabia que ali era brejo, o clima sabia, o mapa sabia — só a tela não | chão de lama escura, poça verde (água parada não reflete o céu como o mar), tronco caído em dobro e **nenhuma copa alta** |
| 8 | 👁 **Deserto** | o chão era um ladrilho **plano**. Areia plana com pedra em cima não é deserto, é um pátio bege | **duna**: meia esfera enterrada até o meio, 14 por pedaço, na cor do próprio chão — enterrar a metade é o que faz encontrar o plano numa curva em vez de num degrau |
| 9 | ✅ **Geleira** | plano branco com 18% de conífera. Sem escala e sem relevo | relevo do deserto com outra mão (10 montes, mais altos e mais estreitos: gelo empurrado, não areia soprada) **e a aurora**: `AAuroraCurtain`, três fitas em arco que serpenteiam, verde embaixo e violeta no topo, a 9000 de altura, com luz de verdade na base. Ela mora SOBRE A GELEIRA — amarrada a quem anda seria lanterna, não céu — e o relógio a acende pela MESMA `WorldNightSky::AuroraStrength` que escreve a linha do painel |
| 10 | 👁 **Vulcão** | **a linha anterior desta lista estava errada**: a lava e os derrames já existiam e já eram pintados de `LavaGlow`. O defeito real era outro — pintar de laranja **não é acender**. `PaintComponent` põe cor num material que não emite luz, e à noite a cratera ficava tão escura quanto o basalto ao lado | luz de verdade na **boca** da cratera (no centro do ator ela acenderia a rocha por dentro), alcance de 2.2 raios da base, mais coluna de fumaça que **alarga ao subir** — largura constante é um cano, e cano não conta que aquilo se dissipa |

---

## Os que ainda estão feios

| # | Cenário | O defeito, dito sem panos quentes | O que ele precisa virar |
|---|---|---|---|
| 11 | ✅ **Cavernas** | três sabores decididos pelo LUGAR: **de lava** (a 160°, dentro do calor do vulcão), **de água** (a 270°, na orla) e **seca**. Estalactite pendurada na verga da boca, estalagmite subindo do chão, poça no corredor, e uma brasa de verdade (`UPointLightComponent`) só na de lava — cor não brilha. A de lava tem a **boca tapada**, e a silhueta conta de longe que ali não se entra | feito em `ACaveSystem` + `IslandFeatureLayout::TemperaAsCavernas`; painel na chave 727 |
| 12 | ✅ **Praia** | faixa de areia molhada acompanhando o ARCO da ilha, espuma logo além da linha d'água, e árvores finas tombadas para o mar | feito. O pacote não traz coqueiro: `tree_thin` inclinada é a silhueta mais próxima, e inclinar é metade do efeito |
| 13 | ✅ **Água doce** | um rio por montanha, descendo do pé dela até a orla: serpentina própria, lago engordando o leito no meio do curso, cachoeira num degrau logo depois, trilha de terra acompanhando a margem, e uma **gruta d’água ao lado de cada queda**. A superfície do rio não tem colisão e fica ACIMA do chão — ninguém mais afunda na água | feito em `FreshWater` + `ARiverCourseView`; a gruta é uma **toca de lado 3**, e o lugar dela é PROCURADO ao redor da queda, do mais perto para o mais longe, porque nenhuma distância fixa serve num rio torto |
| 14 | ✅ **Céu noturno** | mês sinódico de 8,3 dias e mês dracônico de 7,65 — os dois **incomensuráveis**, que é o que faz o eclipse ser raro sem sorteio nenhum. Lua com fase, luar que depende da fase E da altura dela, eclipse lunar (que É a lua vermelha: a cor sai da profundidade do eclipse), eclipse solar escurecendo o meio-dia pela mesma porta que a nuvem já escurecia, estrelas subindo pela rampa do crepúsculo, cometa voltando a cada 41 dias e aurora só no frio | feito em `WorldNightSky` + `ABattleSceneLighting`; painel na chave 761, e `bs.SkyDay 8.15` salta para dentro do eclipse lunar. **Cada eclipse exige no céu o corpo que ele apaga** — deduzir o solar de "a lua nova sobe com o sol" errava por 50 minutos e punha eclipse antes do amanhecer |
| 15 | ✅ **Eventos de terra** | `WorldEvents`: terremoto na falha do vulcão, furacão no mar e na praia, tsunami **depois** do tremor forte — e o mar SOBE de verdade | fechado; a onda é consequência do tremor, não um segundo sorteio |
| 16 | ⛔ **Arena** | ela lê o bioma do `.ini`, não de onde o encontro aconteceu — lutar na geleira dá cenário de floresta | `EncounterLocation` chega até `ABattleArena`, e o cenário da luta é o do chão onde ela começou |

---

## A ordem, e por quê

1. ~~**Pântano** (7)~~ — feito. Era o único *pronto por dentro e errado por
   fora*: custou uma tabela e devolveu um bioma inteiro.
2. ~~**Deserto** (8) e **Geleira** (9)~~ — feito junto, como previsto: uma causa
   só (chão plano), uma cura só (monte enterrado até o meio). A **aurora** da
   geleira veio depois, junto com o céu (14), porque é lá que ela mora — e o
   atraso valeu: a regra de QUANDO há aurora já estava escrita e testada em
   `WorldNightSky::AuroraStrength`, então a cortina não precisou decidir nada.
   Ela cobrou duas lições. A primeira: pintar dentro do CONSTRUTOR cria a
   instância dinâmica de material no arquétipo da CLASSE, e a cor vaza para toda
   aurora que nascer depois — o vulcão já pintava na montagem, e não no
   construtor, sem que ninguém tivesse escrito por quê. A segunda: a linha do
   painel pergunta "está acima de MIM?" e a cortina pergunta "está acima da
   GELEIRA?" — duas perguntas diferentes que precisam passar pela mesma função,
   senão o texto diz aurora e o céu fica preto (L-032).
3. ~~**Vulcão** (10)~~ — feito, e ele corrigiu esta própria lista: o item
   afirmava que faltava lava, e a lava estava lá desde o começo.
4. ~~**Cavernas** (11)~~ — feito na sequência, como previsto: vizinha do vulcão
   em mapa e em material. Ela cobrou uma mudança de *layout*, não só de malha —
   as três cavernas moravam no mesmo anel, então nenhuma podia ser de mar nem
   de lava, e o sabor teria de ser escrito à mão. Regra derivada do LUGAR não
   admite peça mal colocada.
5. ~~**Praia** (12)~~ ✅ e ~~**Água doce** (13)~~ ✅ — as duas bordas d'água.
   A praia mostrou por que a orla não podia sair da cor do chão: o pedaço tem
   6400 de lado e a praia tem 1600 de largura, então **todo** pedaço de praia
   é meio areia e meio interior. Pintar o ladrilho inteiro molharia mata. A
   faixa nasce do ARCO de raio conhecido, recortado pelo quadrado do pedaço —
   e é a única parte do cenário que depende de ONDE o ator está, não só do
   bioma que lhe mandaram.
   A água doce cobrou a mesma lição por outro lado: o curso inteiro tem 6560 de
   comprimento, menos que o lado de um pedaço, então **nascente, lago, queda e
   foz caem todos no mesmo ladrilho**. Nenhuma pergunta feita por pedaço separa
   uma da outra — quem distingue é o RAIO de cada instância no mundo.
   E a gruta ao lado da queda mostrou que constante fixa não coloca peça em
   terreno torto: entre o lago que alaga rio acima e a praia que começa 1600
   antes da orla sobram pouco mais de 2000 de terra, e a perpendicular à
   corrente aponta para dentro do lago quando o rio vem serpenteado. O lugar
   passou a ser **procurado**, do mais perto da queda para o mais longe, com
   folga em cima de caber — e a caverna encolheu para lado 3, porque uma toca
   ao lado de uma cachoeira nunca deveria ter sido um labirinto.
6. **Céu** (14) — ✅ feito, e não dependeu de terreno nenhum. A lição foi que
   duas voltas de períodos parecidos mas não múltiplos entregam raridade de
   graça: mês cheio em número redondo fazia o eclipse solar ser **impossível**,
   e o teste que só perguntava "aconteceu algum eclipse?" passava calado.
   **Eventos** (15) — ✅ feito, e a lição foi outra: o tsunami **não é um evento
   sorteado**, é o que sobra de um terremoto forte. Sortear os dois em separado
   deixaria a onda vir sem tremor nenhum, e nenhum teste de "houve tsunami?"
   pegaria isso. O preço de derivar é geométrico: a folga em que o tremor pode
   cair encurtou para `período − tremor − espera − onda`, senão a onda de um
   período escorre para dentro do período seguinte, que já tem o tremor dele.
   E o mar levanta o ator de água que já existia — a onda ficou visível **sem
   uma única partícula nova**.
7. **Arena** (16) — depende de tudo acima já parecer diferente entre si; antes
   disso, ligar o bioma da luta não mudaria nada visível.
