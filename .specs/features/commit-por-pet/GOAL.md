# OBJETIVO — Commit por pet

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**Aberto em 02/09/2026.** Vem de **B-003**, registrado em 25/08/2026 e adiado
"para M3, junto da expansão do contrato de rede para commit por pet". **M3
fechou sem ela** — é por isso que este objetivo existe hoje, órfão, e não como
item de um marco em andamento.

## O objetivo, numa frase

O turno passa a endereçar **PET**, e não LADO — e é isso que faz dois aliados
poderem convergir para a mesma casa, que é o requisito **BTL-05** que hoje
nenhum teste alcança.

## PRONTO é isto, e nada menos

- [x] **CP1** — o teto de pets por lado que o sistema já aguenta está MEDIDO
- [x] **CP2** — aliados nascem em casas DIFERENTES, e o duelo nasce onde nascia
- [x] **CP3** — o commit do núcleo endereça pet por `PetId`, não lado
- [x] **CP4** — postura, movimento e combate agem sobre o pet endereçado
- [x] **CP5** — o `AddWarning` de BTL-05 virou PROVA, e não sobrou aviso nenhum
- [x] **CP6** — o desempate entre aliados está medido e decidido por `PetId`
- [x] **CP7** — o fio carrega N pets, e a auditoria do commit cobre o tipo novo
- [x] **CP8** — quem joga escolhe ação POR PET, e a tela diz de quem é qual
- [x] O grep por `AddWarning` em `Source/` não acha mais BTL-05
- [ ] Bateria completa verde (hoje **866** depois da CP5; o número só sobe)
- [x] As auditorias limpas — SETE agora, com `audit_anonymous_namespace_names.sh`
      que nasceu nesta feature (L-045)
- [x] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

**ABERTA e em andamento.** CP1 a CP5 commitadas (`461e8d7`, `6c5e18a`, `88df909`,
`71c04ee`, `c6410f7`), bateria 866. As caixas nasceram `⛔` e continuaram assim
depois de a feature começar: o ponteiro dizia "nada aberto" enquanto cinco tasks
fechavam. Marcar a caixa é parte de fechar a task, não do commit final.

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos
com a medição junto: decisão de conteúdo que é do usuário, encostar no traçado
ou no gabarito, e bateria vermelha que não é do teste novo.

## Invariantes

As doze de `corrente`, e três que esta feature acrescenta:

15. **Uma ação tem UM dono, e o dono é o `PetId`.** Endereçar por lado foi o
    que tornou BTL-05 inalcançável: com uma direção só por lado, dois aliados
    partindo de casas diferentes nunca convergem — matematicamente, não por
    teste esquecido. Endereçar por índice de array não serve: `BTL-17` já diz
    que a ordem de `State.Pets` não é determinismo.

16. **O commit NÃO entra no hash — e isso é medido, não presumido.**
    `FBattleState::ComputeHash` não combina nenhum campo de commit; ele ordena
    `Pets` por `PetId` antes de somar, então N pets já hasheiam de forma
    estável. Mudar a forma do commit não invalida snapshot de determinismo por
    si só; **acrescentar pet ao estado, sim.**

17. **Teste que afirma a regra velha VIRA o teste da regra nova.** O
    `AddWarning` de BTL-05 é o caso central: ele existe para não fingir
    cobertura, e a feature tem de **trocá-lo por prova** — não deixar os dois.
    Aviso que sobrevive ao conserto é aviso que ninguém mais lê.

## O que este objetivo NÃO faz

- **Não decide quantos pets por lado.** O número é do usuário, e CP1 só mede o
  TETO que o resto do sistema aguenta hoje.
- **Não reabre DP-02.** A não-coabitação está implementada e provada — a
  medição está na spec, com arquivo e função.
- **Não autora asset.** Pet a mais aparece pela malha e cor que `APetView` já
  atribui no construtor.
- **Não mexe no traçado nem no gabarito.**

## Se o contexto for compactado

Reler este objetivo, `./Tools/goal_status.sh`, `git log --oneline -15`, e
continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**
