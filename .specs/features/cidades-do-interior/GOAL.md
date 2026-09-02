# OBJETIVO — Cidades do interior

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Onze itens seguem em aberto, e estão listados no fim daquele arquivo.


**Spec escrita em 31/08/2026, sem `tasks.md` até agora.** Nesse intervalo a
vila já foi construída no mundo por fora do processo de spec — ver "O que já
existe, medido" no topo de `tasks.md`. Este objetivo nasce **bloqueado**: toda
caixa abaixo começa `[⛔]`, não `[ ]`. Ninguém decidiu ainda os números de
`DefaultGame.ini` que `tasks.md` lista em "Decisões que são do usuário" (valor
inicial da carteira, limiar de ranking, qual trabalho vem primeiro). Até essa
decisão acontecer e alguém trocar `[⛔]` por `[ ]` nas caixas correspondentes,
`./Tools/goal_status.sh` continua apontando para `itens-e-biologia` — que
está aberta e sem bloqueio, e por isso é a prioridade real.

## O objetivo, numa frase

A vila que já existe no mundo passa a fazer alguma coisa quando o jogador
entra nela — curar, cobrar, vender, especializar, viajar e trancar —, em vez
de ser cenário que só colide.

## PRONTO é isto, e nada menos

- [⛔] **CI1** — a carteira existe e aparece na tela
- [⛔] **CI2** — um prédio reage a alguém entrando nele
- [⛔] **CI3** — Centro de Recuperação cura de verdade, e cobra fora da vila inicial
- [⛔] **CI4** — Escola especializa pelo prédio, mesma função do console
- [⛔] **CI5** — Mercado vende o pet capturado pela tabela já testada
- [⛔] **CI6** — Marco de retorno teleporta de verdade, ida e volta
- [⛔] **CI7** — Palafita, Passarela e Chinampa têm cor própria
- [⛔] **CI8** — o ranking existe e tranca/destranca o Portão do Posto de Fronteira
- [⛔] **CI9** — um trabalho real, em que o pet facilita e nunca habilita
- [ ] O grep por `OnActorBeginOverlap`/`TriggerVolume` fora de `/Tests/` acha o
      padrão criado em CI2, reaproveitado por CI3–CI6, CI9
- [ ] Bateria completa verde, zero falhas, zero crash
- [ ] As cinco auditorias limpas
- [ ] Um commit por task, cada um com o motivo — não só o quê

**Trocar `[⛔]` por `[ ]` é decisão do usuário — o número de configuração
correspondente precisa existir antes.** Enquanto qualquer caixa estiver
`[⛔]`, `goal_status.sh` pula esta feature e aponta para a próxima que estiver
de fato aberta.

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Não pare entre tarefas

Depois que as caixas `[⛔]` virarem `[ ]`, `./Tools/goal_status.sh` diz a
próxima. Só se para por três motivos, e todos com a medição junto: decisão de
conteúdo que é do usuário (a lista em `tasks.md` já antecipa as óbvias),
encostar no traçado ou no gabarito de `VillageLayout`/`RegionLayout`/
`SettlementEconomy` além do necessário para ler, e bateria vermelha que não é
do teste novo.

## Invariantes

As doze de `corrente`, mais as duas de `itens-e-biologia`, mais duas que esta
feature acrescenta:

15. **Preço e pagamento vêm sempre de `SettlementEconomy`, nunca de número
    escrito à mão numa tarefa de interação.** O próprio comentário do módulo
    (`SettlementEconomy.h`) já cita o defeito histórico que essa duplicação
    causou noutro sistema (o raio da ilha) — reescrever a tabela dentro de
    CI3/CI5/CI8 recriaria exatamente esse defeito, com outro nome.

16. **Nenhum prédio novo entra sem o `case` de cor correspondente em
    `Village.cpp`.** CI7 existe porque essa regra já foi quebrada uma vez
    silenciosamente (Palafita/Passarela/Chinampa caindo no `default`) — a
    regra vale para qualquer prédio que ainda vier a existir depois desta
    feature.

## O que este objetivo NÃO faz

- **Não decide números de config** (carteira inicial, limiar de ranking,
  preço do trabalho) — ver "Decisões que são do usuário" em `tasks.md`.
- **Não implementa fadiga/saúde do treinador** — pergunta em aberto na
  própria spec, sem mecânica hoje para se apoiar.
- **Não toca `crime-e-recompensa` nem `posse-no-servidor`** — a única
  interseção (venda de pet no Mercado) opera só sobre a coleção local do
  próprio jogador.
- **Não redesenha `VillageLayout`, `RegionLayout` nem a tabela de
  `SettlementEconomy`** — todos os três já implementam o traçado e o preço
  corretos; o trabalho é ligar interação a quem já existe.

## Se o contexto for compactado

Reler este objetivo, `./Tools/goal_status.sh`, `git log --oneline -15`, e
`tasks.md` inteiro (a seção "O que já existe, medido" evita remedir o que já
foi medido). Continuar da primeira caixa `[⛔]` que o usuário já tiver aberto
para `[ ]`. **Não recomeçar, não replanejar, não remedir o que já está
citado com arquivo e linha em `tasks.md`.**
