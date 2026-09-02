---
description: Retoma o objetivo aberto e trabalha até todas as caixas de PRONTO fecharem
---

Leia o GOAL da feature que `$ARGUMENTS` nomeia:
`.specs/features/$ARGUMENTS/GOAL.md` — ele é o rumo, e vale mais que qualquer
coisa que você lembre desta conversa.

**Sem argumento:** rode `ls .specs/features/*/GOAL.md`, leia cada um, e continue
o que tiver caixa de PRONTO aberta. Se houver mais de um, PERGUNTE qual —
escolher sozinho gastaria a rodada no objetivo errado.

Depois:

1. `git log --oneline -15` e `git status --short --branch` — os commits dizem
   até onde foi, e o status diz em que branch você está.
2. Ache a **primeira caixa aberta** de PRONTO no GOAL.
3. Continue dali. **Não recomece, não replaneje, não peça confirmação para
   seguir** — o objetivo já foi aprovado quando foi aberto.

Execute `.specs/features/construcao-do-mundo/tasks.md` em ordem, um commit por
task, com o motivo no corpo — não só o quê.

As seis invariantes do GOAL reprovam a task independentemente do resto. As duas
que mais custaram rodada neste projeto:

- **ator sem malha atribuída no construtor não existe na tela** (três
  ocorrências: pets, inimigos do mundo, o próprio jogador);
- **defeito vira TESTE antes de virar conserto** (consertar por hipótese custou
  três rodadas em agosto e mais três nas galerias).

Gabarito de aceite: `docs/mundo/carta-ilha-de-mata.html`. O mundo construído
tem de bater com a carta — mesma quantidade, mesmos lugares.

Se bloquear: faça tudo o que não depende do bloqueio, e então diga o que travou
com a **medição** que sustenta isso. Reduzir escopo é decisão do usuário.

$ARGUMENTS
