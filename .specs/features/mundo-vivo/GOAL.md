# OBJETIVO — Mundo vivo

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**Escrito em 02/09/2026, e NÃO ABERTO.** Vem de
`.specs/features/mundo-vivo/spec.md` (31/08/2026) — que tinha spec e nenhum
`tasks.md`, e por isso era invisível para `./Tools/goal_status.sh`.

Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`goal_status.sh` não a elege como próxima.

## O objetivo, numa frase

**A mata cresce e o corte fica — pela idade real do mundo, não pela sessão de
quem está jogando agora.**

## PRONTO é isto, e nada menos

- [x] **MV1** — a idade do mundo existe, e ela não para quando o jogo fecha
- [⛔] **MV2** — a árvore cresce com a idade do mundo, não com a dela própria
- [⛔] **MV3** — cortar deixa marca: a árvore derrubada não volta sozinha amanhã
- [⛔] **MV4** — o prazo de rebrota é número de configuração, nunca literal em C++
- [⛔] **MV5** — idade do mundo e corte pendente aparecem na tela
- [⛔] **MV6** — pets envelhecem — três perguntas de conteúdo sem resposta
- [⛔] **MV7** — coleta de recursos, bosque e comerciante — dono, ferramenta e presença do pet em aberto
- [ ] O grep fora de `/Tests/` acha quem LÊ a idade do mundo em produção
- [ ] Bateria completa verde — o total do dia em que a feature abrir (hoje **858**); o número só sobe
- [ ] As cinco auditorias limpas
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

Caixa **⛔** não está aberta nem fechada: ela **não começou**, de propósito. Uma
sessão está executando `itens-e-biologia`, e o ponteiro do projeto é único.

**MV6 e MV7 não entram nesta lista.** Elas têm pergunta de conteúdo sem
resposta (ver "Decisões que são do usuário") — uma caixa PRONTO sem aceite
possível seria a mesma promessa quebrada que `SettlementEconomy.h` já nomeia
para a carteira: "prédio com porta que não abre."

## Decisões que são do usuário

- **A idade do pet tem fim?** Ele pode morrer de velho, ou envelhece sem
  limite?
- **O tempo passa com o jogo fechado, para o PET?** (Para a mata, MV1 já
  decide que sim — subtração de data, sempre atual. Para o pet pode ser
  diferente: "cuidar" pode significar justamente proteger de um tempo que só
  corre jogando.)
- **O cuidado do dono é ativo (uma ação que ele faz) ou passivo (só o tempo,
  e o dono não precisa fazer nada)?**
- **Quem é o dono de uma plantação** — o jogador que a plantou, o assentamento
  mais perto, ou ninguém (comum)?
- **A ferramenta usada para coletar importa** (machado colhe mais madeira que
  a mão), ou todo corte rende o mesmo?
- **Coletar exige o pet por perto**, ou o jogador coleta sozinho?

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos
com a medição junto: decisão de conteúdo que é do usuário (as seis acima),
encostar em `BattleSim` ou na fronteira do núcleo determinístico, e bateria
vermelha que não é do teste novo.

## Invariantes

As doze de `corrente`, as duas de `itens-e-biologia`, as três de
`posse-no-servidor` (13–17), e três que esta feature acrescenta:

18. **A idade do mundo mora FORA do `BattleSim`.** É estado do mundo, não do
    combate — atravessar essa fronteira com relógio ou com estado de servidor
    é o mesmo erro que já se proíbe para `float`/`FMath::Rand` dentro do
    núcleo determinístico.

19. **Rebrota é sempre BASE CALCULADA + EXCEÇÃO COM PRAZO, nunca uma segunda
    malha de "estado da árvore" paralela ao gerador por semente.** Duas
    fontes concordam até a primeira edição — é como L-032/L-033 nasceram.

20. **Prazo de rebrota é sempre número de CONFIGURAÇÃO.** É a garantia que a
    torneira de `mae-natureza` precisa para existir; sem ela, "ajustar o
    equilíbrio" significaria recompilar o jogo.

## O que este objetivo NÃO faz

- **Não decide envelhecimento de pet nem coleta de recursos.** MV6/MV7 ficam
  fora da lista PRONTO — ver "Decisões que são do usuário".
- **Não cria guarda florestal, comerciante nem plantação.** Zero ocorrência
  hoje (`rg -c "GuardaFlorestal|Comerciante" Source/` = 0); nascem só depois
  de MV7 destravar.
- **Não toca `BattleSim`.** O padrão de `CellCountdown`/`CellRevertsTo` é
  imitado, nunca reusado — o timebase dos dois é diferente por natureza (turno
  de batalha vs. tempo real).
- **Não decide onde o backend mora.** Assume estender `apps/api-battle-pets`
  pelo padrão de `ownership/`; se for serviço novo, é decisão de arquitetura
  pendente de confirmação.
- **Não implementa a leitura do lado de `mae-natureza`.** Essa feature lê o
  que `mundo-vivo` grava; escrever essa leitura é dela, não desta.

## Se o contexto for compactado

Reler este objetivo, `.specs/features/mundo-vivo/spec.md` e
`.specs/features/mundo-vivo/tasks.md` (a seção "O que a medição achou" de cada
task reordena o trabalho: nada de idade de mundo existe hoje, e o único
relógio existente reseta a cada boot), `./Tools/goal_status.sh`,
`git log --oneline -15`, e continuar da primeira caixa aberta. **Não
recomeçar, não replanejar.**
