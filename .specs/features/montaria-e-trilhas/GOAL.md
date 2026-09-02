# Montaria e trilhas — objetivo

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


## O objetivo, numa frase

Montar um pet muda a velocidade e o cansaço de andar pelo relevo real da
ilha — sem travar passagem por peso, sem virar veículo, e sempre visível na
tela.

## PRONTO é isto, e nada menos

- [⛔] **MT1** — andar montado é mais rápido que a pé no MESMO trecho de
  ladeira, respeitando a proporção de custo subida/descida que já existe
- [⛔] **MT2** — subir acumula cansaço mais rápido que descer, num valor novo
  e isolado, visível na tela
- [⛔] **MT3** — pet mais pesado cansa mais no mesmo trajeto, mas nenhum
  trajeto fica impossível por peso
- [⛔] **MT4** — nem todo pet pode ser montado; dado de save antigo sem o
  campo novo carrega com default seguro
- [⛔] **MT5** — o pet montado é visível na tela, com malha e cor atribuídas e
  testadas no construtor
- [ ] bateria de testes completa verde (meça o total ao pegar a tarefa — não
  fixe um número aqui; o total só cresce)
- [ ] auditorias de determinismo, recálculo e texto localizável limpas
  (`Tools/audit_determinism.sh`, `Tools/audit_no_recalculation.sh`,
  `Tools/audit_localizable_text.sh`)
- [ ] um commit por tarefa (MT1, MT2, MT3, MT4, MT5 — cinco commits, não um só)

**Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`Tools/goal_status.sh` segue apontando para o que estiver aberto em outra
feature.**

## Invariantes

- **Uma fonte de custo de trajeto.** `TravelCostBetween` e
  `UphillCostWeight`/`DownhillCostWeight` continuam sendo a ÚNICA conta de
  ladeira — MT1 e MT2 leem dela, nenhuma tarefa cria uma segunda conta
  paralela (é a mesma disciplina de `TrailLayout`, e o motivo já está
  documentado no próprio código: "quem traça a trilha e quem cobra o cansaço
  precisam concordar").
- **Peso nunca bloqueia, só multiplica cansaço** — invariante nomeada
  explicitamente na spec e repetida como contrapeso obrigatório em MT3.
- **Todo comportamento observável aparece na tela** via
  `FBattleDebugScreen::Show` — velocidade montada, cansaço acumulado, recusa
  de montaria, o próprio pet montado.
- **Ator novo nasce com malha e cor atribuídas no construtor, e o teste
  verifica a ATRIBUIÇÃO** — este projeto já teve três atores invisíveis
  passarem em toda bateria de lógica (pet, inimigo do mundo, o jogador). MT5
  existe para não ser o quarto.
- **Save antigo carrega.** MT4 adiciona campo novo a um registro de pet já
  existente — retrocompatibilidade é teste obrigatório, não boa intenção.

## O que este objetivo NÃO faz

- Não cria veículo mecânico, trilha por pisoteio, estrada pavimentada nem
  viagem rápida pela trilha — a spec rejeita as quatro.
- Não traz voar nem submergir para fora da batalha — esses verbos continuam
  vivendo só dentro de `BattleSim` até outra decisão.
- Não fixa os números finais de cansaço, peso ou velocidade — a FORMA da
  conta é entregue, os números são decisão do usuário.
- Não mexe em `IslandGeography`, `TrailLayout` nem em `mundo-por-biomas` — são
  objetivos irmãos, sem dependência de um sobre o outro.

## Se o contexto for compactado

Leia primeiro `.specs/features/montaria-e-trilhas/tasks.md`, seção "O que JÁ
existe, medido" — o bloqueio que a spec original citava (`GroundHeightAt` não
existia) **já foi resolvido antes desta feature começar**; não reabra essa
investigação. `TravelCostBetween` e as duas ponderações de custo são a base
de MT1 e MT2; não invente uma conta nova onde essa já serve.

Enquanto MT1–MT5 estiverem `[⛔]`, esta feature não abre sozinha —
`Tools/goal_status.sh` só aponta para ela depois que alguém decidir destravar
cada caixa.
