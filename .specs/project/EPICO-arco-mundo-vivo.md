# Épico — o arco do MUNDO VIVO e o crime que dele depende

> Rastreio de sequência, NÃO segunda fonte de verdade. Cada caixa aqui é um
> ponteiro para a caixa real no GOAL da feature — as regras, o aceite e o
> contrapeso moram lá, e é lá que se marca PRONTO. Este arquivo só fixa a
> ORDEM e os bloqueios entre features, que nenhum GOAL isolado enxerga.

**Objetivo:** fechar o arco inteiro do tempo que passa — a idade do mundo, a
mata que cresce e cicatriza, os pets que envelhecem — e, na ponta, a prisão do
crime que só tem o que fazer dentro porque o tempo agora corre.

**Fundação já pronta:** `mundo-vivo` MV1 — a idade do mundo é uma data de
nascimento no servidor, e o cliente diz "desconhecida" quando não sabe. Tudo
abaixo mede o tempo por ela; nenhuma caixa reintroduz um segundo relógio.

## A ordem, e por que ela é esta

- [x] **MV1** — a idade do mundo existe *(fundação, pronta)*
- [x] **MV2** — a árvore cresce com a idade do MUNDO (semente + idade), não com
      a dela própria — `mundo-vivo/tasks.md` MV2. *Depende de MV1.*
- [x] **MV3** — cortar deixa marca no SERVIDOR: a derrubada não volta antes do
      prazo — `mundo-vivo/tasks.md` MV3. *Depende de MV1 + posse-no-servidor.*
- [x] **MV4** — o prazo de rebrota é NÚMERO DE CONFIGURAÇÃO, nunca literal —
      `mundo-vivo/tasks.md` MV4. *Depende de MV3.*
- [x] **MV5** — idade do mundo e corte pendente aparecem na TELA —
      `mundo-vivo/tasks.md` MV5. *Depende de MV2 + MV3.*
- [x] **MV6** — pets envelhecem — `mundo-vivo/tasks.md` MV6. As três perguntas
      de conteúdo (a idade tem fim? corre offline? cuidado ativo?) JÁ têm
      resposta nas decisões 32, 33 e 34 — confirmar antes de codar, e o número
      de envelhecimento nasce em config. *Depende de MV1.*
- [ ] **CR10** — o pet capturado envelhece na prisão —
      `crime-e-recompensa/tasks.md` CR10. É a prisão CONSUMINDO o relógio de
      MV6, nunca um segundo envelhecimento. *Depende de MV6.*
- [ ] **MV7** — coleta de recursos, bosque e comerciante —
      `mundo-vivo/tasks.md` MV7. Dono, ferramenta e presença do pet em aberto —
      decisão de conteúdo antes de codar. *Depende de MV2/MV3.*

## As invariantes que valem em TODA caixa deste arco

1. **Uma fonte de verdade por regra.** A idade mora fora do `BattleSim`; a mata
   é base calculada + exceção com prazo; a prisão consome o relógio, não o
   recria. Duplicar tabela ou validação foi L-032/L-033.
2. **Número de balanceamento é config, nunca literal em C++** — prazo de
   rebrota, taxa de envelhecimento, idade máxima. Inventar número sem decisão
   ou medição é o defeito que MV4 e MV6 existem para barrar.
3. **Contrapeso testado em cada caixa** — o negativo prova tanto quanto o
   positivo: terreno intacto não grava exceção, backend fora não vira zero,
   pet novo não morre de velho.
4. **Ator sem malha atribuída no construtor não existe na tela.**
5. **Defeito vira TESTE antes de virar conserto.**
6. **O mundo construído bate com `docs/mundo/carta-ilha-de-mata.html`.**

Fechadas as oito caixas, o arco do tempo está completo e o crime tem o ciclo
inteiro do roubo — do flagrante à cela que envelhece o pet.
