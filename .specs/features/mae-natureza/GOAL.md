# OBJETIVO — Mãe Natureza

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**Escrito em 02/09/2026, e NÃO ABERTO.** Vem de
`.specs/features/mae-natureza/spec.md` (31/08/2026) — que tinha spec e nenhum
`tasks.md`, e por isso era invisível para `./Tools/goal_status.sh`.

Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`goal_status.sh` não a elege como próxima.

## O objetivo, numa frase

**Um corretor PURO — censo entra, correção sai — que gira a torneira do mundo
sem nunca tocar o balde do jogador, e que nunca age em silêncio.**

## PRONTO é isto, e nada menos

- [x] **MN1** — medir o panteão: "rebrota" e "censo" já estão construídos? (não)
- [x] **MN2** — o corretor nasce puro: censo entra, correção sai, nada mais
- [x] **MN3** — toda correção é delatada — correção sem registro é bug
- [x] **MN4** — torneira, nunca balde: teste negativo prova o limite
- [x] **MN5** — o preço do assentamento vira número de configuração
- [x] **MN6** — espécie rara migra quando o censo cai — depende de censo agregado, que não existe
- [x] **MN7** — Mãe Natureza age devagar e fora de vista — mas o jogador vê
- [ ] O grep fora de `/Tests/` acha quem CHAMA `NatureBalance::Correct`
- [ ] Bateria completa verde — o total do dia em que a feature abrir (hoje **858**); o número só sobe
- [ ] As cinco auditorias limpas, e a sonda de isolação sem `NatureBalance` importando mundo
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

Caixa **⛔** não está aberta nem fechada: ela **não começou**, de propósito. Uma
sessão está executando `itens-e-biologia`, e o ponteiro do projeto é único.

**MN6 não entra nesta lista.** Ela depende de censo agregado de espécie, que
por sua vez depende de `posse-no-servidor` PS1 estar no ar — sem isso, o
"censo" que ela mediria seria inventado, e a regra da casa proíbe número
inventado.

## Decisões que são do usuário

- **Faixa-alvo por ILHA ou por REGIÃO?** Muda o tamanho do censo que MN6
  precisa calcular.
- **Dá para agradar um deus** (oferenda, santuário), ou a correção é sempre
  autônoma?
- **Deuses podem discordar entre si?** (Ex.: Mãe Natureza querendo repovoar
  onde outro deus quer escassez.)

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos
com a medição junto: decisão de conteúdo que é do usuário (as três acima),
o corretor deixar de ser puro (importar símbolo de mundo — MN2 tem contrapeso
para isso), e bateria vermelha que não é do teste novo.

## Invariantes

As doze de `corrente`, as duas de `itens-e-biologia`, as três de
`posse-no-servidor` (13–17), as três de `mundo-vivo` (18–20), e três que esta
feature acrescenta:

21. **O corretor de Mãe Natureza é PURO.** Nunca importa símbolo de um módulo
    que leia o mundo — a camada de fora busca o censo e aplica a correção.
    Sem essa separação, `NatureBalance` e `ForestBackdrop` viram dois planos
    se consultando, e o resultado disso já tem nome documentado: `abort()`
    depois de minutos de log sem dizer nada útil.

22. **Toda correção é DELATADA.** Correção sem registro é bug, nunca efeito
    colateral silencioso — é a diferença entre "Mãe Natureza agiu" e "alguém
    arredondou errado."

23. **Mãe Natureza ajusta a TORNEIRA, nunca o BALDE.** Prazo, demanda, preço,
    prêmio — nunca coleção, atributo ou especialidade do jogador. Provado por
    teste negativo (MN4), nunca só por comentário em prosa.

## O que este objetivo NÃO faz

- **Não implementa aviso de guarda florestal nem poste da vila.** `mundo-vivo`
  MV7 ainda não existe (bloqueada); MN7 entrega o equivalente em painel de
  depuração.
- **Não decide se dá para agradar um deus, nem se deuses discordam.** Ver
  "Decisões que são do usuário".
- **Não escreve `mundo-vivo`.** Só lê o que ele grava.
- **Não corrige `spec.md`.** A correção do panteão (MN1) mora em `tasks.md`.
- **Não toca `BattleSim`.** `NatureBalance` é `BattleSquare`, puro, mas fora
  do núcleo determinístico de combate.

## Se o contexto for compactado

Reler este objetivo, `.specs/features/mae-natureza/spec.md` e
`.specs/features/mae-natureza/tasks.md` (MN1 é o achado que reordena tudo: o
panteão dizia "rebrota, censo" já construídos, e a medição achou zero dos
dois), `./Tools/goal_status.sh`, `git log --oneline -15`, e continuar da
primeira caixa aberta. **Não recomeçar, não replanejar.**
