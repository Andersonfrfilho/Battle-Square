# GOAL — FINALIZAR TUDO (sem pausas)

> Instrução do usuário (04/09): criar um objetivo que fecha TODAS as caixas
> abertas do projeto e **executar em fluxo, sem parar a cada caixa para pedir
> "pode seguir"**. Continuar até uma caixa que dependa GENUINAMENTE de fato ou
> decisão que só o usuário resolve — essa se REGISTRA e se pula, não trava o
> resto.

## Decisões do usuário (04/09)

1. **Profundidade: FEATURE-COMPLETA, uma a uma.** Cada caixa fecha de ponta a
   ponta — regra pura + teste (contrapeso) + tela + wiring físico em mundo —
   antes da próxima. Sem "regra-pura-primeiro em largura".
2. **O cluster de geometria (P7–P10, cavernas CQ1–4, fundura F1) SE RESOLVE na
   raiz: REGENERAR O ASSADO do mundo** com fundura por ponto. É empreitada
   estrutural, e destrava nado, poço e cavernas de uma vez. Deixa de ser
   "registrar e pular".

## A regra de não-pausa

- Cada caixa acionável: regra pura → teste (com contrapeso) → fiação → bateria +
  auditorias → commit. Segue para a próxima sem perguntar.
- Caixa travada por FATO medido (geometria do mundo) ou por NÚMERO de conteúdo
  que só o usuário decide: registra o porquê com a medição, pula, segue.
- Número de balanceamento sempre em `DefaultGame.ini` com default são e
  documentado — o usuário afina depois; ausência de decisão não trava o código.
- Uma fonte de verdade por regra. Nada de segundo relógio, segunda tabela,
  segunda validação.

## Ordem (do que fecha mais barato ao que é sistema novo)

1. [x] **crime-e-recompensa/CR5** — recompensa paga pelo criminoso. Pequena,
       fecha 100% do crime. Compõe com a carteira (CI1) e ecoa a decisão 28.
2. [ ] **mundo-por-biomas** (MB1–MB4) — posto de fronteira, pet muda de elemento
       por bioma, vila veste a cor, desastre muda a vila. Reusa fontes já
       existentes (bioma, WorldEvents).
3. [ ] **montaria-e-trilhas** (MT1–MT5) — montado é mais rápido, subir cansa
       mais, peso cansa, nem todo pet monta, o montado aparece na tela.
4. [ ] **mae-natureza** (MN1–MN7) — o corretor de censo (torneira, não balde),
       delatado, com preço de assentamento em config; migração de espécie rara.
5. [ ] **a-malha-vem-de-fora** (MV1–MV10) — malha/material por dado, não
       hardcoded; migração ator a ator + auditoria que impede a volta.
6. [ ] **segredos-e-a-carta** (SC1–SC4) — o gabarito aprende "escondido"; o
       mapa revela por andar; o achado na tela.
7. [ ] **cavernas-nas-quedas** (CQ1–CQ4) / **fundura-no-tracado** (F1) —
       dependem de fundura por ponto no assado. Ver bloqueio abaixo.
8. [ ] **mundo-vivo/MV7** — coleta, bosque, comerciante. SISTEMA NOVO inteiro
       (GuardaFlorestal, Comerciante, compra em sociedade). O maior; por último.

## O cluster de geometria — via REGENERAR O ASSADO (decisão do usuário)

- **fundura-no-tracado F1** é a raiz: gerar **fundura por ponto** no assado do
  mundo (hoje não existe). Feito isso, destravam em cascata: **pendencias P7**
  (nadar quando a água passa da cintura), **P8/cavernas CQ2** (o poço se lê pela
  fundura), **P9/cavernas CQ1/CQ3** (caverna por dentro da cachoeira — hoje a
  mais perto está a 2710u de um poço de 886u, o assado precisa aproximá-las),
  **P10/SC/ChartConformance** (carta e mundo idênticos). Ordem: F1 → assado →
  poço/nado → cavernas → conformidade da carta.

## Invariantes (valem em toda caixa)

1. Ator sem malha atribuída no construtor não existe na tela.
2. Defeito vira TESTE antes de virar conserto.
3. Uma fonte de verdade por regra.
4. Número de balanceamento é config, nunca literal.
5. Contrapeso testado em cada caixa.
6. O mundo bate com `docs/mundo/carta-ilha-de-mata.html`.
