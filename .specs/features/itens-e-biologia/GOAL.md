# OBJETIVO — Itens e biologia

**Aberto em 02/09/2026.** Vem de `tudo-que-falta`, que o abriu ao chegar nos
dois sistemas que várias tarefas pediram e que nunca existiram.

## O objetivo, numa frase

Dois pets do mesmo elemento resistem diferente **pela biologia**, e um pet
resiste diferente de si mesmo **pelo item que veste**.

## PRONTO é isto, e nada menos

- [x] **B1** — a biologia existe, tem quatro eixos e chega à batalha
- [ ] **I1** — o item existe como cadastro
- [ ] **I2** — a mochila guarda, com quantidade de pilha
- [ ] **I3** — equipar tira da mochila, desequipar devolve
- [ ] **I4** — o item soma na resistência
- [ ] **I5** — o consumível gasta, e a falha aparece
- [ ] **I6** — o item na tela
- [ ] O grep fora de `/Tests/` acha quem EQUIPA um item
- [ ] Bateria completa verde (hoje **843**; o número só sobe)
- [ ] As cinco auditorias limpas
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos
com a medição junto: decisão de conteúdo que é do usuário, encostar no traçado
ou no gabarito, e bateria vermelha que não é do teste novo.

## Invariantes

As doze de `corrente`, e duas que esta feature acrescenta:

13. **O dado do pet é ASSINADO, e campo novo não invalida assinatura velha.**
    Ausente é ausente, não vazio: acrescentar `"biology":{}` a um pet que não o
    tinha muda o payload dele e o descarta como violação — um pet correto,
    apagado por um campo que não existia quando ele nasceu.

14. **Save antigo carrega.** Campo novo na coleção não pode apagar os pets de
    quem já jogava. `UPROPERTY` ausente desserializa como vazio, e vazio é
    exatamente "não informado".

## O que este objetivo NÃO faz

- **Não decide que itens existem.** O catálogo nasce com a bota de lava, que é
  o aceite escrito, e pergunta o resto.
- **Não autora asset.** Item aparece por texto no painel.
- **Não mexe no traçado nem no gabarito.**

## Se o contexto for compactado

Reler este objetivo, `./Tools/goal_status.sh`, `git log --oneline -15`, e
continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**
