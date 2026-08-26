# Roteiro de Verificação — Interface de Batalha

**Feature:** `.specs/features/interface-de-batalha/`
**Nível:** `Content/Maps/BattleScreen` (`/Game/Maps/BattleScreen`)
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado por um humano.

Cobre só o que DP-ui-05 marca como não automatizável: o layout aparecer, os
botões serem clicáveis, o texto caber, e a sensação de jogar um turno inteiro.
O contrato entre a tela e o `UBattleActionQueueComponent` já está coberto por
`Automation RunTests BattleSquare.UI` (9 testes) e **não** se repete aqui.

---

## Preparação

**Abra o editor pelo Finder ou pelo Epic Games Launcher, não por terminal.**
Editor lançado por linha de comando em segundo plano não recebe teclado na
viewport de PIE — foi o que travou a verificação de M5 por várias rodadas.

Depois, abra `/Game/Maps/BattleScreen` e aperte Play.

---

## UI-01 — A batalha abre sozinha, sem mundo

- [ ] **Não verificado**

**Critério:** ao dar Play, a arena aparece com os **dois pets** e a interface
por cima. Nenhum mundo aberto é carregado, nenhuma caminhada é necessária.

Se aparecer céu e chão vazios em vez da arena, a câmera não foi apontada para
ela — é regressão do `SetViewTarget` em `ABattleScreenGameMode`.

---

## UI-02 — Escolher os dois passos

- [ ] **Não verificado**

**Passo:** clique em **Atacar**.

**Critério:** os 6 botões de tipo somem e aparece a grade 3x3 de direções, com
o texto mudando para "Escolha a direção". Clique numa seta: a grade some, os
tipos voltam, e o contador vira "Ações: 1 de 3".

**Depois:** clique em **Defender**.

**Critério:** confirma **direto**, sem pedir direção, e o contador vai para 2.
É a regra do componente aparecendo na tela — se pedir direção aqui, a tela está
decidindo por conta própria, o que DP-ui-01 proíbe.

---

## UI-03 — Desfazer e cancelar

- [ ] **Não verificado**

**Critério, em três partes:**
1. Com 1+ ação confirmada, **Desfazer** está habilitado e reduz o contador.
2. Com 0 ações, **Desfazer** está desabilitado (cinza).
3. No passo de direção, **Cancelar** (o centro da grade) volta aos tipos sem
   confirmar nada.

---

## UI-04 — Fechar o turno

- [ ] **Não verificado**

**Passo:** confirme 3 ações e clique em **Confirmar turno**.

**Critério:** o texto vira "Turno fechado — aguardando o oponente", e nenhum
botão de ação aceita mais clique. A resolução roda e a vida dos pets muda.

---

## UI-05 — O layout é usável (julgamento humano)

- [ ] **Não verificado**

Três perguntas, cada uma com resposta escrita:

1. Os botões são grandes o bastante para clicar sem mirar? ____________
2. Dá para ler quantas ações faltam sem procurar? ____________
3. A grade de direção é compreensível como "para onde"? ____________

**Contexto honesto:** DP-ui-03 escolheu **layout funcional, sem identidade
visual** — botões com rótulo de texto, posição simples, nenhuma arte. Feio é
esperado; **confuso** não é. Só o segundo caso é defeito.

**E DP-ui-02** escolheu a grade 3x3 em vez da roseta radial que DP-08 preferia,
com o custo nomeado: pior para o dedo. Se a resposta 3 for "não", a roseta volta
à mesa — e trocar não muda nenhuma chamada, por construção.
