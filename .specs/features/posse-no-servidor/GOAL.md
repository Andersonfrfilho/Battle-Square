# OBJETIVO — Posse no servidor

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**ABERTO em 03/09/2026, pelo usuário** ("pode seguir para o multiplayer,
posse-no-servidor" — e a emenda que reordena tudo: "o jogo deve rodar
normalmente offline"). As seis decisões da spec estão respondidas na 38-b de
`DECISOES.md`; a 5 (pet abandonado) segue fora de escopo, como a spec já
mandava. **Escrito em 02/09/2026.** Vem do último parágrafo de **B-005**,
que fechou o defeito de mistura e deixou escrito o que sobrou: *"este conserto
tirou o defeito de mistura; a posse ainda não é do servidor."*

Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`goal_status.sh` não a elege como próxima.

## O objetivo, numa frase

**De quem é o pet deixa de ser uma pergunta sobre o ARQUIVO da máquina e passa a
ser uma pergunta sobre a CONTA.**

## PRONTO é isto, e nada menos

- [x] **PS1** — a tabela da posse existe no banco, com unicidade `(dono, catálogo)`
- [x] **PS2** — o token do JOGADOR autentica, e `admin` não vira dono de nada
- [x] **PS3** — a autorização é por OBJETO, com teste negativo de isolamento
- [x] **PS4** — as rotas da posse existem, e a captura é idempotente
- [x] **PS5** — o lado da batalha passa a ter CONTA, sem tocar em rede
- [x] **PS6** — o jogo pode falar HTTP, e o núcleo continua sem alcançá-lo
- [x] **PS7** — ler a coleção fora da batalha, e backend fora do ar não impede jogar
- [x] **PS8** — a captura viaja por fila, e sobe uma vez só
- [x] **PS9** — quem já joga migra sem perder nada, e a migração é repetível
- [x] **PS10** — a posse na TELA, dizendo o que está pendente
- [x] **PS11** — capturar passa a exigir SEM DONO
- [x] O grep fora de `/Tests/` acha quem PERGUNTA de quem é o pet
- [x] Bateria completa verde — **953** em 03/09/2026
- [x] As seis auditorias limpas, e as sondas de isolação reprovando de propósito
- [x] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

Caixa **⛔** não está aberta nem fechada: ela **não começou**, de propósito. Uma
sessão está executando `itens-e-biologia`, e o ponteiro do projeto é único.

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos com
a medição junto: decisão de conteúdo que é do usuário (esta feature tem **seis**,
listadas na spec), encostar em AD-014 ou na fronteira de `BattleSim`, e bateria
vermelha que não é do teste novo.

## Invariantes

As doze de `corrente`, as duas de `itens-e-biologia`, e três que esta feature
acrescenta:

13. **O dado do pet é ASSINADO, e campo novo não invalida assinatura velha.**
    A posse é registro de OUTRA tabela, apontando para `catalog_id`. Se fosse
    campo do payload canônico, cada captura passaria pela chave privada do
    backend.

14. **Save antigo carrega.** Campo novo na coleção não pode apagar os pets de
    quem já jogava. `UPROPERTY` ausente desserializa como vazio, e vazio é
    exatamente "não informado". Migração **não apaga** o save local.

15. **A batalha nunca lê a rede.** AD-014: a montagem lê do espelho local, e
    `ResolveTurn` não toca rede nem disco. Ler posse é caminho de ENTRADA no
    mundo; escrever posse é fila DEPOIS da batalha. Se a escrita esperasse
    resposta, o combate passaria a depender de latência.

16. **Autorização é por OBJETO, não por rota.** O servidor confere que o pet
    pertence à conta do token. Sem isso, "roubar" é uma requisição HTTP bem
    formada — e a posse no servidor não seria mais segura que o arquivo que ela
    substitui, só mais difícil de editar.

17. **Uma fonte de verdade para "de quem é este lado".** A costura já existe e
    é `ResolveCollectionSlotForSide`. Troca-se o que ela GUARDA, nunca se
    acrescenta um segundo dono ao lado — duas respostas concordariam até a
    primeira partida em rede, que é como B-005 nasceu.

## O que este objetivo NÃO faz

- **Não implementa roubo.** Entrega o DONO, que é o pré-requisito da Fatia 1 de
  `captura-roubo-e-treinador`. As regras de transferência já estão escritas e
  testadas em `ownership.pure.ts`.
- **Não cria lista de procurados, polícia, recompensa nem o treinador como
  alvo.** Tudo em `crime-e-recompensa`, que espera esta.
- **Não sincroniza mochila, mapa, descoberta nem perfil de treinador.** Só a
  POSSE do pet.
- **Não toca `BattleSim`.** Nenhuma linha; a sonda de isolação é a prova, e PS6
  acrescenta uma.
- **Não decide número nenhum.** Teto de coleção, política de offline e como a
  conta chega ao processo são decisões do usuário.

## Se o contexto for compactado

Reler este objetivo, `.specs/features/posse-no-servidor/spec.md` (a seção *"O que
a medição achou antes de qualquer linha"* é a que reordena o trabalho: a posse
existe pela metade no servidor), `./Tools/goal_status.sh`,
`git log --oneline -15`, e continuar da primeira caixa aberta. **Não recomeçar,
não replanejar.**
