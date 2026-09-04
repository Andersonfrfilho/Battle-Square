# OBJETIVO — A malha vem de fora

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.

**Aberto em 02/09/2026**, depois da pesquisa de `docs/assets/PRONTOS.md`. Vem de
uma medição, não de um desejo: existem **6 assets autorados** no projeto (o resto
de `Content/` é `MI_StreamingCell_*` gerado por código), e a malha de ~17 atores
está **escrita dentro do construtor C++**.

## O objetivo, numa frase

A malha e o material de cada ator param de morar no construtor e passam a vir de
**dado** — para que adotar um pacote pronto seja trocar dado, e trocar de pacote
também.

## Por que agora, e por que ANTES de baixar qualquer coisa

Importar modelo hoje põe `.uasset` em `Content/` que **nenhum ator olha**. É a
versão invertida do defeito que este projeto pagou **três vezes** (componente
visual sem asset atribuído): lá o componente existia sem asset; aqui o asset
existiria sem componente. Nos dois casos, bateria verde e nada na tela.

## PRONTO é isto, e nada menos

- [x] **MV1** — a linha de base MEDIDA: cada `FObjectFinder` de malha/material
      do projeto, com arquivo e linha, congelada antes de mexer
- [x] **MV2** — nasce o dado: malha + material por PAPEL, num lugar só
- [x] **MV3** — `ScenaryPalette` VIRA o ponto por onde o material é escolhido —
      não ganha uma irmã (L-032 pela quarta vez seria aqui)
- [x] **MV4** — o primeiro ator migra (`APetView`), e o teste prova que a malha
      veio do DADO, não do construtor
- [x] **MV5** — os atores de batalha migram (`PetOwnerView`, `BattleArena`)
- [x] **MV6** — os atores de mundo migram (`Village`, `GroundUseActor`,
      `FerryActor`, `WorldEncounterActor`, `WorldExplorerCharacter`)
- [x] **MV7** — os atores de cenário migram (`ForestBackdrop`, `MountainRange`,
      `WalkableMountain`, `CaveSystem`, `Volcano`, `AuroraCurtain`,
      `WorldBoundaryWater`)
- [⛔] **MV8** — os cinco de malha procedural pegam MATERIAL pelo dado
      (`TrailMesh`, `AqueductMesh`, `RiverMesh`, `TerrainMesh`, `CrossingMesh`)
- [⛔] **MV9** — o padrão hardcoded não VOLTA: auditoria que reprova
      `FObjectFinder` de malha novo fora do dado
- [⛔] **MV10** — na tela: o painel diz de onde veio a malha de cada ator, para
      que "não apareceu" e "veio do lugar errado" sejam perguntas diferentes
- [⛔] Bateria completa verde (hoje **873**; o número só sobe)
- [⛔] As sete auditorias limpas
- [⛔] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

**PARADA.** As caixas nascem `⛔` de propósito: `goal_status.sh` é de valor
único, e a sessão vizinha está em `a-carta-muda-uma-vez: M3`. Caixa aberta aqui
roubaria o ponteiro dela no meio do trabalho. **Trocar `⛔` por espaço é seu ato**,
e a hora certa é quando aquela feature fechar.

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos
com a medição junto: decisão de conteúdo que é do usuário, encostar no traçado
ou no gabarito, e bateria vermelha que não é do teste novo.

## Invariantes

As doze de `corrente`, e três que esta feature acrescenta:

18. **Uma fonte de verdade para o VISUAL, e ela é a mesma da cor.**
    `ScenaryPalette` já é o lugar único da cor, com 28 papéis em `EScenaryRole`.
    Material e malha entram POR ELA. Uma tabela nova ao lado seria a segunda
    fonte de verdade — o defeito L-032, que este projeto já pagou três vezes, e
    as cópias concordam até a primeira edição.

19. **Componente criado não é componente visível.** Todo ator migrado mantém o
    teste que verifica a ATRIBUIÇÃO da malha e da cor. Migrar sem esse teste
    troca um caminho provado por um caminho que só falha na tela — e o de agora
    ao menos falha igual sempre.

20. **O dado não decide ESTILO.** Ele decide de ONDE vem a malha. Qual pacote
    entra é decisão do usuário (`docs/assets/PRONTOS.md` §5), e esta feature
    tem de fechar verde ainda apontando para `/Engine/BasicShapes/*` — se ela
    exigir asset novo para passar, ela virou outra feature.

## O que este objetivo NÃO faz

- **Não escolhe estilo de arte.** É do usuário, e está registrado como aberto.
- **Não baixa nem importa pacote nenhum.** Nem Quaternius, nem Fab, nem Synty.
- **Não abre áudio nem VFX.** Os dois são zero medido e nenhum GOAL os pediu.
- **Não autora asset.** Tudo continua em primitiva da engine até alguém decidir.
- **Não mexe no traçado nem no gabarito.**

## Se o contexto for compactado

Reler este objetivo, `docs/assets/INVENTARIO.md`, `docs/assets/PRONTOS.md`,
`./Tools/goal_status.sh`, `git log --oneline -15`, e continuar da primeira caixa
aberta. **Não recomeçar, não replanejar.**
