> NOTA (04/09, medido): SATISFEITA por a-carta-muda-uma-vez. FreshWaterTest
> afirma a gruta encostada na cachoeira; PlungePoolDepthUnits da o poco por
> fundura. FreshWater 15 verdes. NAO reconstruir (L-032).

# OBJETIVO — Cavernas nas quedas

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**Aberto em 02/09/2026.** Vem de **P9** em
`.specs/features/pendencias/BLOQUEIO-P7-P8-P9-P10.md`, que foi aberta como
"parcial" e reclassificada pela medição: a caverna mais próxima de uma queda
está a **2 710** unidades, e o poço da queda tem **886** de raio.

## O objetivo, numa frase

Existe pelo menos uma cachoeira com uma **boca de gruta atrás da lâmina de
água**, e quem entra por ali fica seco.

## PRONTO é isto, e nada menos

- [x] **CQ1** — a boca tem posição no mundo, e a distância dela até a queda aparece
- [x] **CQ2** — o poço da queda vira exceção declarada: a boca molha, o corpo não
- [x] **CQ3** — a boca olha para a queda
- [x] **CQ4** — a propriedade na forma existencial, e a prova na tela
- [ ] O teste novo reprova o mundo de HOJE antes de aprovar o de depois
- [ ] Carta e `ChartConformance` movidos JUNTOS quando as galerias mudarem
- [ ] Bateria completa verde (hoje **858**; o número só sobe)
- [ ] As cinco auditorias limpas, e `bake_island.sh` com as sete seções
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Esta feature está PARADA, e por quê

As quatro tarefas nascem `[⛔]` porque três decisões de conteúdo são do usuário,
e estão listadas em `tasks.md`: **quantas** quedas ganham gruta na lâmina, se a
gruta da lâmina **substitui** a de lado ou soma a ela, e o que se vê de dentro.

A segunda decide se a carta se move uma vez ou duas. Começar por CQ2 sem ela é
escolher o número de cavernas por omissão.

Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`goal_status.sh` não a elege como próxima.

## Invariantes

As doze de `corrente`, e três que esta feature acrescenta:

15. **Uma fonte de verdade para "aqui é água".** `HalfWidthAtProgress` é a
    largura da água corrente e é lida por vau, balsa e pelo templo de Corrente.
    O poço da queda é `PlungePoolHalfWidthUnits`, função separada. A exceção
    consulta a segunda; **não** alarga a primeira.

16. **O plano da água nunca pergunta ao uso do solo.** O caminho é de mão única
    — a gruta LÊ o plano da ilha, e o uso do solo LÊ a água. A pergunta de volta
    reentra inicializador estático e mata o processo escrevendo dump de pilha por
    minutos (regra de mapas §14).

17. **Campo novo no assado desserializa como o comportamento de HOJE.**
    `UPROPERTY` ausente vira `0`, e `0` na borda da boca é **sul** — que é o que
    toda caverna faz hoje. Assado velho continua válido, e a divergência de
    parâmetro continua NOMEANDO o parâmetro.

## O que este objetivo NÃO faz

- **Não move as três cavernas do `IslandFeatureLayout::Plan()`.** O assunto são
  as treze grutas, que já nascem das quedas.
- **Não afrouxa a folga da gruta.** A exceção é nomeada e vale só no poço; a
  folga menor devolveria as quinas dentro do rio.
- **Não troca a busca por cálculo.** *"Onde não houver lugar, não há gruta"*
  continua valendo.
- **Não autora asset.** A lâmina de água na frente da boca é decisão do usuário,
  e pode ficar só no texto do painel.

## Se o contexto for compactado

Reler este objetivo, `spec.md`, `./Tools/goal_status.sh`, `git log --oneline -15`,
e continuar da primeira caixa aberta. **Não recomeçar, não replanejar.** Se as
caixas ainda estiverem `[⛔]`, a decisão do usuário não veio — não abrir por
conta própria.
