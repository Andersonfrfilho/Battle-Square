# Mundo por biomas — objetivo

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


## O objetivo, numa frase

Cada ilha é um bioma inteiro e coerente — relevo, pet selvagem, vila e
assentamento abandonado concordam entre si — e o Posto de Fronteira só abre
para quem venceu o ranking regional.

## PRONTO é isto, e nada menos

- [x] **MB1** — o Posto de Fronteira barra a passagem quando a região não
  venceu o ranking, e libera a MESMA passagem quando vence
- [x] **MB2** — o pet selvagem que aparece muda de elemento conforme o bioma
  do encontro, e o encontro sem lugar (`EncounterLocation` vazio) continua
  sem filtro
- [⛔] **MB3** — a vila veste a cor do bioma onde está, reusando a mesma fonte
  de paleta que a arena já usa (não uma segunda tabela)
- [⛔] **MB4** — uma vila atingida por um desastre de `WorldEvents` muda de
  estado observável (prédio danificado, serviço que some), e sem desastre
  continua oferecendo tudo de sempre
- [ ] bateria de testes completa verde (meça o total ao pegar a tarefa — não
  fixe um número aqui; o total só cresce)
- [ ] auditorias de determinismo, recálculo e texto localizável limpas
  (`Tools/audit_determinism.sh`, `Tools/audit_no_recalculation.sh`,
  `Tools/audit_localizable_text.sh`)
- [ ] um commit por tarefa (MB1, MB2, MB3, MB4 — quatro commits, não um só)

**Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`Tools/goal_status.sh` segue apontando para o que estiver aberto em outra
feature.**

## Invariantes

- **Uma ilha, um bioma** (`IslandGeography::IslandBiome()`) não muda — MB1–MB4
  decoram e regram em cima disso, nunca reabrem a escolha de "vários biomas
  por ilha."
- **Fonte única por regra.** MB3 não cria uma segunda tabela cor×bioma se a
  duplicação já registrada em `ROADMAP.md` ("a arena veste o bioma") resolver
  o problema em algum lugar reusável — a tarefa PRIMEIRO confere, depois
  decide se reusa ou se essa verificação vira ela mesma o achado.
- **Todo comportamento observável aparece na tela** via
  `FBattleDebugScreen::Show` — o Posto barrando, o pet mudando de elemento, a
  vila mudando de cor, a vila danificada.
- **Texto do jogador é `LOCTEXT`**, nunca `FString::Printf` — vale para
  qualquer mensagem nova de "Posto fechado" ou de vila abandonada.
- **Constante de espaço nasce fração do raio**, nunca unidade absoluta — como
  já é `RegionSizeUnits()`, `FracaoDasVilas`, `HomeRadiusUnits` (L-049:
  constante absoluta envelhece quando a escala do mundo cresce).

## O que este objetivo NÃO faz

- Não cria sistema de ranking/pontuação — MB1 lê um booleano por região; o que
  alimenta esse booleano é outra frente.
- Não decide quantas vilas o jogador vê ao nascer, nem onde ele nasce.
- Não decide se pet de bioma é exclusivo ou só mais comum — MB2 assume
  exclusivo; mudar isso é decisão do usuário e reabre a tarefa.
- Não adiciona ilha nova, elemento novo nem espécie de pet nova — os oito
  elementos e o catálogo de pets já existem.
- Não mexe em traçado de trilha nem em `montaria-e-trilhas` — são objetivos
  irmãos, não um dependendo do outro.

## Se o contexto for compactado

Leia primeiro `.specs/features/mundo-por-biomas/tasks.md`, seção "O que JÁ
existe, medido" — ali está o inventário completo do que já está pronto
(`IslandGeography`, `RegionLayout`, `VillageLayout`, `SettlementEconomy`,
`TrailLayout`, `WorldEvents`, `ScenaryPalette`, `Config/PetTypes.json`,
`EncounterMatchAssembler`). Não meça de novo o que já está medido ali. Antes
de tocar em MB3, leia `.specs/project/ROADMAP.md` em volta de "a arena veste o
bioma" — é uma duplicação já conhecida e ainda não verificada.

Enquanto MB1–MB4 estiverem `[⛔]`, esta feature não abre sozinha —
`Tools/goal_status.sh` só aponta para ela depois que alguém decidir destravar
cada caixa.
