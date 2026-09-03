# OBJETIVO — Cidades do interior

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


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

> **ABERTA PARCIALMENTE em 03/09/2026, pelo usuário.** A regra desta feature é
> que a caixa só abre quando o número de configuração dela existe — então
> abriram as cinco que **não dependem de número nenhum**, e as quatro que
> dependem seguem `[⛔]` com a pergunta nomeada ao lado. Abrir as nove de uma vez
> seria escolher os números por omissão, que é o que este bloqueio impede.

- [x] **CI1** — a carteira existe e aparece na tela
      Começa com **100** (decisão 56). Paga Arena e trabalho; o **achado** é
      task própria, depois que a carteira existir para receber (decisão 57).
- [x] **CI2** — um prédio reage a alguém entrando nele
- [x] **CI3** — Centro de Recuperação cura de verdade, ~~e cobra fora da vila
      inicial~~ **e NÃO cobra**
      Fechada em 03/09 pela decisão 61: a vida persiste em
      `FOwnedPetInstance::HealthPercent` (ausente = cheio, piso de 1 na
      batalha), a derrota acorda curado (60+16), e entrar no Centro cura de
      graça. `CuraNaCidade = 25` segue sem cliente — apagar é decisão do
      usuário.
- [⛔] **CI4** — Escola especializa pelo prédio, mesma função do console
      ⚠️ **PAROU em 03/09/2026 — GEOMETRICAMENTE IMPOSSÍVEL como escrita,
      medido.** A task manda chamar `LearnSpecialtyOfCurrentField()` do gatilho
      da Escola, mas essa função exige estar DENTRO de um campo de treino
      (`AWorldTrainingField::IsInside`), e os campos ficam num anel de 22.000
      unidades do centro (`TrainingFieldRingRadiusUnits`, `DefaultGame.ini:42`)
      enquanto a clareira inteira da vila tem meia-extensão de 1.348. Chamada da
      Escola, ela responde "você não está num campo de treino" — sempre, por
      vinte mil unidades.

      **E o desenho por trás é decisão de conteúdo em aberto na própria spec:**
      "Quais atributos cada cidade ensina? É o que faz viajar valer." Se a
      Escola especializa em QUALQUER atributo sem sair da vila, os cinco campos
      e a viagem até eles perdem o papel. As saídas possíveis — a Escola
      escolhe o atributo num menu; a Escola só MOSTRA as especialidades e o
      gesto continua no campo; ou o campo ganha a confirmação em duas etapas —
      são produtos diferentes, e a escolha é do usuário.
- [x] **CI5** — Mercado vende o pet capturado pela tabela já testada
- [x] **CI6** — ~~Marco de retorno teleporta~~ **o Marco é PONTO DE
      RENASCIMENTO, e quem cai acorda no hospital**
      Reescrita pela decisão 60. Ninguém viaja por vontade própria — o Marco
      diz onde se volta quando se perde, e o lugar é o Centro de Recuperação,
      curado e de graça (decisão 16). Acorda-se no MAIS PERTO de onde se caiu,
      para renascer nunca poupar caminhada.
- [x] **CI7** — Palafita, Passarela e Chinampa têm cor própria
- [x] **CI8** — o ranking existe e aparece ~~e tranca/destranca o Portão~~
      ⚠️ **Metade REESCRITA pela decisão 58:** o Portão **não tranca**, porque o
      jogador tem liberdade de viajar para onde quiser. O ranking continua
      existindo e aparecendo; o que ele deixa de ser é uma cancela.
- [x] **CI9** — um trabalho real, em que o pet facilita e nunca habilita
      O jogador **escolhe onde** (decisão 59): Fazenda, Criadouro ou Pomar. É um
      trabalho com três lugares, e o que muda entre eles é qual atributo do pet
      facilita.
- [x] O grep por `OnActorBeginOverlap`/`TriggerVolume` fora de `/Tests/` acha o
      padrão criado em CI2, reaproveitado por CI3–CI6, CI9
      Medido em 03/09: `Village.cpp` (as portas) e `BattleSquareGameMode.cpp`
      (quem escuta). CI5, CI6 e CI8 consomem o estado da porta; CI9 usa o
      traçado (`UseAt`), porque mancha de solo não é prédio.
- [x] Bateria completa verde, zero falhas, zero crash — **914** em 03/09/2026
- [x] As cinco auditorias limpas
- [x] Um commit por task, cada um com o motivo — não só o quê

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
