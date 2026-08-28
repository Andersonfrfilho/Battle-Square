# Roteiro de verificação — Skills por Pet

**Status:** **não verificado ainda.**

Os testes provam que o catálogo carrega, que a fila recusa e que a IA respeita.
O que eles não decidem: **a diferença entre pets se SENTE jogando?**

## Skills de hoje (`Config/PetSkills.json`)

| Tipo | Skill própria |
|---|---|
| Fogo | voar |
| Agua | submergir — **exige casa de água** |
| Planta | camuflar |

Uma por tipo, de propósito: identidade só existe quando há diferença, e duas
por tipo já começam a diluir isso. Os seis tipos de ação universais
(aguardar, mover, atacar, magia, defender, esquivar) são de todo pet.

## SKILL-01 — O pet mostra o que ele é

- [ ] O painel diz, ao começar: `<pet> (tipo X) tem N skill(s) própria(s)`.
- [ ] A barra mostra `SUAS SKILLS — <nomes>`, com botão só para o que o seu
      pet tem. Pet sem skill diz isso por extenso, em vez de ficar em branco:
      vazio e catálogo quebrado seriam idênticos na tela.
- [ ] Na barra do jogador 2, **só aparecem** as skills daquele pet.
- [ ] Um pet de tipo diferente oferece skill diferente.

## SKILL-02 — A recusa é de regra, não de tela

- [ ] Com o catálogo apagado, o painel avisa e todo pet fica com os seis
      universais — nada de crash, nada de "todas as skills".

## SKILL-03 — O oponente também respeita

- [ ] O bot nunca voa com um pet que não voa. (No feed: nenhuma linha de
      "alçou voo" para um pet de tipo que não tem voo.)

## SKILL-04 — Julgamento

- [ ] Enfrentar um pet Água (submerge) é **diferente** de enfrentar um Planta
      (camufla)? Se a sensação for a mesma, as trocas ainda estão indistintas.
- [ ] Uma skill por tipo é pouco, ou é o suficiente para dar identidade?

## SKILL-05 — Submergir exige água (2026-08-27)

Submergir descreve estar DENTRO da água. Sem terreno, ela funcionava em
qualquer casa — "mergulhar no chão seco" contradiz o que a skill é, e tornava o
mapa irrelevante para a decisão.

- [ ] Em casa seca, submergir **falha**, e o feed diz o motivo: *"tentou
      mergulhar, mas não há água nesta casa"*. Não é silêncio nem "não
      funcionou".
- [ ] A grade **mostra** onde é água (azul, com o rótulo ÁGUA) — sem isso a
      regra só se descobre perdendo.
- [ ] Numa casa de água, funciona e a imunidade vale.
- [ ] Dá para planejar: andar até a água num turno, mergulhar no seguinte.
- [ ] As outras posturas (defender, esquivar, camuflar, voar) **não** dependem
      de terreno.
- [ ] Julgamento: procurar a água antes de submergir torna a arena parte da
      decisão, ou vira só um passo a mais?

**Consequência que apareceu sozinha:** uma casa é água OU é de dano, nunca as
duas. Então quem submerge nunca escapa do dano de casa por submergir — só
voando. Foi encontrada quando um teste antigo virou impossível de montar.
