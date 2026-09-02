# Cidades do interior — tarefas

## O que já existe, medido (leia antes de discordar da lista abaixo)

A spec foi escrita em 31/08/2026 e nunca virou `tasks.md`. Nesse intervalo,
outra sessão construiu boa parte da vila **sem passar pelo processo de spec**
— por isso `goal_status.sh` nunca viu este trabalho, e é por isso que ele
precisa nascer bloqueado (ver `GOAL.md`). Medido em `Source/`, não suposto:

- **A vila inteira já nasce sozinha no mundo.**
  `ABattleSquareGameMode::SpawnStartingVillage()` (linha ~812) percorre o
  `RegionLayout::Plan()` **completo** (não só a vila natal — comentário no
  código cita a L-041, "regra completa que ninguém alcançava"), chama
  `SetSettlementKind` antes de `BuildVillage()` para cada assentamento, e
  **respeita vila colocada à mão** via `TActorIterator<AVillage>` (linha 822)
  antes de spawnar. Reporta em `FBattleDebugScreen::Show(..., Key=744)`.
- **`VillageLayout::PlanFor` já desenha os cinco tipos de assentamento**
  (`VilaInicial`, `VilaDaAcademia`, `VilaDoMercado`, `CidadeGrande`,
  `PostoDeFronteira` — `RegionLayout.h`), incluindo o "Mercado do Lago" que a
  spec descreve como ideia: `VillageLayout.cpp` já tem `Palafita`/`Chinampa`
  (linha ~160) e `Passarela` (linha ~178) para `VilaDoMercado`, com
  `FileirasDePalafita = 3` (linha 29). **Isto vai além do que a spec.md pede**
  — a spec não sabia que isso já existia.
- **A economia inteira já está calculada e testada, e não é usada por
  ninguém.** `SettlementEconomy.cpp` implementa exatamente a tabela de preço
  da spec (`PrecoBase=100`, `AgioDaCidade=160`, `CidadePagaPeloPet=70`,
  `CuraNaCidade=25`, `PremioDaVila=30`). Busquei quem chama
  `SettlementEconomy::Offers/PricePercent/PayoutPercent` fora do próprio
  arquivo: só `SettlementEconomyTest.cpp`. **Zero jogador vê este número.** O
  próprio header já avisa (`SettlementEconomy.h`, linha 42): *"A carteira
  ainda não existe. Enquanto não existir, isto é tabela com teste, não preço
  na tela — prédio com porta que não abre é promessa quebrada."* — a casa já
  escreveu a acusação que abre a Fatia 1 abaixo.
- **Nenhum prédio reage a jogador nenhum.** Busca por `OnActorBeginOverlap`,
  `TriggerVolume` e `Interact` em `Source/BattleSquare/*/World/` (fora de
  `Tests/`) não achou nada. A Centro de Recuperação não cura, a Academia não
  cobra, o Mercado não vende, o Marco não teleporta, o Portão não tranca — os
  prédios colidem (`SetCollisionEnabled(QueryAndPhysics)`, `Village.cpp`) e
  ficam parados. **Isto é a fatia que falta inteira: nenhuma tarefa aqui é
  "consertar interação". A primeira tarefa é "criar interação", porque ela
  não existe em nenhum prédio do jogo hoje.**
- **Os cinco campos de treino de atributo já existem e já funcionam** —
  `SpawnTrainingFields()` (linha ~862), gate por `bSpawnTrainingFields`
  (`DefaultGame.ini`), e o painel do mundo já mostra barra de progresso ao
  entrar (ver `CLAUDE.md`, seção "O que fica visível no MUNDO"). **Não é
  tarefa desta feature** — já está pronto e fora de escopo.
- **A especialização (Escola) já tem lógica pronta, só não tem porta.**
  `ABattleSquareGameMode::LearnSpecialtyOfCurrentField()`
  (`BattleSquareGameMode.cpp`, linha 1277) já vira o pet especialista no
  atributo do campo em que ele está parado, e a escolha **não se desfaz**
  (mensagem em ~linha 1230). Hoje só é alcançável por console
  (`bs.Especializar`, linha 2590) — o próprio header documenta por quê
  (`BattleSquareGameMode.h`, linha 404): "punição por acidente" — console
  exige digitar o comando de propósito, botão de UI não.
- **O elemento Terra já tem a skill que a spec.md perguntava.** A spec (linha
  ~142) faz a pergunta "que skill o elemento TERRA ganha?" como questão em
  aberto. Já está respondida em código:
  `FPetSkillCatalog::SkillFromName` mapeia `escavar→Escavar`, e
  `PetTypeCatalogTest.cpp` (linha 276) confirma
  `GetSkillsForType("Fisica/Terra").Contains(EActionType::Escavar)`. **Não é
  tarefa — é achado a reportar.**
- **O ranking é "o clímax e a chave da fronteira", por comentário do próprio
  código, e não existe.** `RegionLayout.h` (linha 29) documenta: *"A arena e o
  ranking da região. O clímax, e a chave da fronteira"*, e
  `VillageLayout.h` (linha 62): *"O portão do posto de fronteira. Só abre
  para quem venceu o ranking."* Busquei por qualquer coisa que LEIA uma
  pontuação de ranking para abrir o portão (`VenceuRanking`, `GateOpen`,
  `PortaoAberto` e variantes) — nada. O Portão (`EVillageBuilding::Portao`)
  spawna, colide, e nunca tranca nem destranca. É parede decorativa hoje.
- **Trabalho (`trabalhos`) é cenário, não mecânica.** `GroundUseWorkTest.cpp`
  testa Fazenda/Criadouro/Pomar/PomarSelvagem/Loja/Acampamento/Deck/Poço só
  por posicionamento e material (ex.: poço seco vs. cheio muda a textura).
  Nenhum arquivo liga essas construções a uma tarefa, um pet ou uma
  recompensa.
- **As três construções novas do lago não têm cor própria.**
  `Village.cpp`, função que decide a cor por tipo de prédio (linhas 32–43),
  tem `case` para `CentroDeRecuperacao`, `Escola`, `Arena`, `Marco`, `Praca`,
  `Casa`, `Academia`, `Mercado`, `Portao` — **não tem** `case` para
  `Palafita`, `Passarela` nem `Chinampa`. Elas caem no `default` cinza
  genérico. Não é "invisível" (o projeto já sofreu esse defeito três vezes,
  ver `CLAUDE.md`) — é pior que decorativo e melhor que invisível: existe,
  colide, mas não se distingue de nada por cor, violando a própria regra do
  projeto de "cor com significado".
- **Venda no Mercado é do PET DO JOGADOR, não de posse alheia.**
  `SettlementEconomy.h` (linha 21): *"Vender o pet capturado. Troco, nunca
  salário."* Isto opera sobre `FPetCollectionSaveGame` do jogador que já
  existe hoje — **não depende de `posse-no-servidor`**. A dependência de
  posse-servidor é só do roubo (feature `crime-e-recompensa`), não da venda
  no mercado.

---

## CI1 — A carteira existe, e um número na tela prova
> 🤖 Modelo: `sonnet`

Cria o campo de moeda do jogador (persistido — mesma frase do B-005 aplicada
aqui: campo novo em save existente não pode apagar o que já havia, ver
Invariante 14 de `itens-e-biologia`). Some ao HUD/painel do mundo.

**Por que esta é a primeira tarefa e não uma tarefa qualquer:** o próprio
código já apontou o bloqueio (`SettlementEconomy.h`, linha 42, citado acima).
Toda tarefa CI2 em diante que cobra ou paga precisa debitar/creditar algo que
hoje não existe.

**Não invente um valor inicial "bonito"** — o número de partida vai para
`DefaultGame.ini` como config, do mesmo jeito que o tamanho da grade e
`bSpawnTrainingFields` já vivem lá. Se não houver decisão do usuário sobre o
valor, a tarefa fica em "existe, lê de ini, aparece na tela" — o valor
numérico é decisão do usuário (ver seção final).

*Aceite:* a carteira sobrevive a salvar e carregar, save antigo sem carteira
carrega com o valor de `DefaultGame.ini` (não zera nem trava), e o número
aparece no painel do mundo.

*Contrapeso obrigatório:* gastar mais do que se tem **falha e não desconta**
— carteira negativa é o mesmo tipo de defeito que mochila com quantidade
negativa seria em `itens-e-biologia`.

---

## CI2 — Um prédio reage a alguém entrando nele
> 🤖 Modelo: `opus` 🧠 — é o PRIMEIRO padrão de interação de prédio do jogo

Depende de: nenhuma (não depende de CI1 — a interação pode existir antes de
custar algo).

Hoje nenhum prédio dispara nada quando o jogador chega perto ou entra
(medido acima — grep vazio). Esta tarefa cria o padrão genérico — volume de
gatilho no lote do prédio (`VillageLayout::PlotHalfExtentUnitsFor`,
`FitsInPlotFor` já existem como extensão do lote, é reaproveitar geometria
que já existe, não inventar nova) que dispara um evento identificando QUAL
`EVillageBuilding` foi tocado.

**Por que `opus` e não `sonnet`:** toda tarefa CI3 em diante DEPENDE deste
padrão. Escolher errado aqui (por exemplo, prender a lógica ao prédio
específico da Centro de Recuperação em vez de a um padrão genérico por
`EVillageBuilding`) obriga a refazer o mesmo gatilho seis vezes.

*Aceite:* entrar e sair do lote de QUALQUER prédio aparece no painel de
debug (`FBattleDebugScreen::Show`) nomeando o prédio — provado em pelo menos
dois tipos de prédio diferentes, não só um.

*Contrapeso obrigatório:* sair do lote sem ter disparado a ação (ex.: entrar
e sair da Academia sem confirmar treino) não cobra nem aplica nada — entrar
não é comprometer-se.

---

## CI3 — Centro de Recuperação cura de verdade
> 🤖 Modelo: `sonnet`

Depende de: CI1, CI2.

Liga `SettlementEconomy::Offers/PricePercent(Kind, ESettlementService::Cura)`
ao pet do jogador. A vila inicial cura de graça (`PricePercent` retorna 0
para ela — já testado em `SettlementEconomyTest.cpp`), as outras cobram.

*Aceite:* o MESMO pet ferido cura de graça na vila inicial e cobra da
carteira em outro assentamento que ofereça cura — o preço que aparece na
tela é o que `SettlementEconomy::PricePercent` devolve, não um número
digitado à mão na tarefa.

*Contrapeso obrigatório:* lugar que não oferece cura (`Offers` retorna
falso) não pode curar de graça por engano — o próprio comentário do código
avisa que essa é a ordem que confunde (`SettlementEconomy.h`, linha 51).

---

## CI4 — Escola especializa pelo prédio, não só pelo console
> 🤖 Modelo: `sonnet`

Depende de: CI2.

Chama `ABattleSquareGameMode::LearnSpecialtyOfCurrentField()` (já existe,
`BattleSquareGameMode.cpp:1277`) a partir do gatilho da Escola, com uma
confirmação explícita na tela — a mensagem já existente ("a escolha NÃO se
desfaz") precisa aparecer ANTES de confirmar, não depois.

*Aceite:* entrar na Escola e confirmar produz o MESMO resultado que
`bs.Especializar` produzia pelo console — mesma função, chamada de outro
lugar. `bs.Especializar` continua existindo (não se remove um caminho que
funciona).

*Contrapeso obrigatório:* entrar na Escola sem confirmar não muda a
especialidade — a "punição por acidente" que o comentário do header teme
(`BattleSquareGameMode.h:404`) tem que continuar impossível pelo prédio
também.

---

## CI5 — Mercado vende o pet capturado, pelo preço da tabela
> 🤖 Modelo: `sonnet`

Depende de: CI1, CI2.

Liga `SettlementEconomy::Offers/PayoutPercent(Kind, ESettlementService::Venda)`
à coleção do jogador (`FPetCollectionSaveGame`/`FOwnedPetInstance` — já
existem). **Isto é sobre o SEU pet capturado, não sobre posse de outro
jogador — não depende de `posse-no-servidor`** (ver achado acima).

*Aceite:* vender o mesmo pet no Mercado paga mais do que vender numa vila
comum, na proporção exata que `SettlementEconomy::PayoutPercent` já
calcula e já testa.

*Contrapeso obrigatório:* vender remove o pet da coleção — vender sem
remover duplicaria pet e dinheiro ao mesmo tempo, o mesmo formato de defeito
que I3 (`itens-e-biologia`) evitou para item equipado.

---

## CI6 — Marco de retorno teleporta de verdade
> 🤖 Modelo: `sonnet`

Depende de: CI2.

Hoje o Marco existe, tem cor própria (`Village.cpp:35`), colide, e não faz
nada — a spec (linha 44) descreve exatamente o problema que isso deixa:
"sem ele, uma ilha de exploração vira ilha de andar igual toda vez". Liga o
gatilho da Marco a viagem rápida real (ida) e registra o ponto para volta.

*Aceite:* usar o Marco muda a posição do jogador de verdade (não é um texto
dizendo "você viajou" — é o pawn em outro lugar do mundo), e voltar ao Marco
volta para onde ele estava antes.

*Contrapeso obrigatório:* usar o Marco durante uma batalha não pode ser
possível — viagem rápida no meio de um turno é o tipo de atalho que quebra
estado (mesma classe de cuidado que `BattleSim` já tem contra relógio
externo).

---

## CI7 — Cor própria para Palafita, Passarela e Chinampa
> 🤖 Modelo: `haiku`

Depende de: nenhuma.

Acrescenta os três `case` que faltam em `Village.cpp` (linhas 32–43) para
`EVillageBuilding::Palafita`, `Passarela` e `Chinampa`. É o menor
consertável desta lista, e o único que não depende de nada.

*Aceite:* as três construções deixam de cair no `return
FLinearColor(0.5f, 0.5f, 0.5f)` do `default` — teste novo lê a cor de cada
uma e confirma que não é mais a cor padrão, e que as três são diferentes
entre si (senão "cor própria" é só trocar um cinza por outro).

*Contrapeso obrigatório:* o `default` continua existindo para qualquer
`EVillageBuilding` futuro que ainda não tenha cor — não se remove a rede de
segurança, só se para de precisar dela para estas três.

---

## CI8 — O ranking existe, e ele tranca/destranca o Portão
> 🤖 Modelo: `opus` 🧠

Depende de: CI2.

`RegionLayout.h` e `VillageLayout.h` já chamam o ranking de "o clímax e a
chave da fronteira" e "só abre para quem venceu o ranking" (citado acima) —
isso é intenção documentada no próprio código, não invenção minha. Hoje não
existe pontuação nenhuma nem checagem nenhuma; o Portão nunca tranca.

Cria uma pontuação de ranking por jogador (incrementada por vitória na
Arena da vila — `SettlementEconomy::Offers(Kind,
ESettlementService::PremioDeRanking)` já sabe quais assentamentos pagam
prêmio), e faz o Portão do Posto de Fronteira consultar essa pontuação
contra um limiar. **O limiar vai para `DefaultGame.ini`** — não é número
que esta tarefa escolhe, é número que esta tarefa expõe.

*Aceite:* o MESMO jogador, abaixo do limiar, não atravessa o Portão do
Posto de Fronteira; acima do limiar, atravessa. A prova é o mesmo jogador
nos dois estados, não dois jogadores diferentes — é a mesma forma de aceite
que a bota de lava usa em `itens-e-biologia` ("o MESMO pet... e sem ela,
não").

*Contrapeso obrigatório:* perder na Arena não pode ANDAR a pontuação para
trás abaixo de um vitória já contada — ranking que soma e depois desconta
por perda é decisão de conteúdo (ver seção final), não suposição desta
tarefa.

---

## CI9 — Um trabalho, e o pet FACILITA, nunca HABILITA
> 🤖 Modelo: `opus` 🧠 — decisão de design que a spec já resolveu, falta implementar

Depende de: CI1, CI2.

A spec (linhas 138–163) já decide a forma: começar com **um** tipo de
trabalho, e a regra inquebrável é que a skill do pet acelera ou paga melhor,
nunca tranca quem não tem. Cita o próprio risco: "um trabalho que EXIGE
submergir tranca o jogador sem" pet daquele tipo, e é o oposto do que se
quer.

Escolhe UM `EGroundUse` já existente como cenário (`Fazenda`, `Criadouro`,
`Pomar` ou similar — já plantado no mundo, `GroundUseWorkTest.cpp` confirma)
e liga produção real: entrar, esperar, receber pagamento da carteira (CI1).
Se o jogador tiver o pet com a skill relevante (ex.: `Escavar` para minério,
usando o mapeamento que já existe em `PetSkillCatalog`), o resultado é
melhor — nunca condição para participar.

*Aceite:* um jogador SEM o pet de skill relacionada completa o trabalho e
recebe pagamento (mais baixo); o MESMO jogador COM o pet completa mais
rápido ou recebe mais — nunca "não pode fazer".

*Contrapeso obrigatório:* o teste que prova isso precisa rodar o caminho
SEM o pet primeiro — se só se testa com o pet presente, a regra "facilita,
nunca habilita" nunca é realmente verificada (o caminho sem pet é o que a
spec teme quebrar).

---

## O que estas tarefas NÃO fazem

- **Não decidem o valor inicial da carteira, o limiar de ranking, nem o
  preço/pagamento do trabalho novo** — esses números vão para
  `DefaultGame.ini` como configuração, e a decisão de qual valor colocar lá
  é do usuário (ver "Decisões que são do usuário" abaixo). O aceite de CI1,
  CI8 e CI9 é "o número existe e é lido de configuração", não um valor
  específico.
- **Não tocam `spec.md`, `VillageLayout.cpp`/`.h`, `RegionLayout.cpp`/`.h`
  nem `SettlementEconomy.cpp`/`.h`** além do necessário para LER o que já
  existe — esses arquivos já implementam corretamente o traçado da vila e a
  tabela de preço; a mudança é ligar quem já existe a interação, não
  redesenhar a tabela.
- **Não criam trabalho além do primeiro** — a spec pede começar com um, e
  níveis são bloco explícito de trabalho futuro.
- **Não mexem em fadiga/saúde do treinador** — a spec deixa essa pergunta
  aberta de propósito ("O que machuca o treinador?"), e não há mecânica
  nenhuma hoje para se apoiar; é decisão do usuário, não tarefa.
- **Não implementam o roubo, a posse por servidor nem qualquer coisa da
  feature `crime-e-recompensa`** — CI5 (venda no Mercado) opera só sobre a
  coleção local do próprio jogador.
- **Não autoram asset novo.** Cor e texto seguem a mesma convenção do resto
  do projeto — mesh primitivo + `FLinearColor`, texto por `LOCTEXT`.

## Decisões que são do usuário

- **Valor inicial da carteira** (CI1) e como o jogador ganha mais dinheiro
  além de vender pet e ganhar ranking (batalha comum paga? só a Arena?).
- **Limiar de ranking que abre o Posto de Fronteira** (CI8), e se perder
  batalha desconta pontos já ganhos ou só trava novos ganhos.
- **Qual `EGroundUse` vira o primeiro trabalho** (CI9) — Fazenda, Criadouro
  e Pomar já existem plantados; a spec não escolhe qual.
- **Se o trabalho tem turno/tempo de espera real ou é instantâneo ao
  entrar** — não decidido em spec.md nem em código.
- **O que machuca/cansa o treinador**, e se essa mecânica entra nesta
  feature ou fica para depois — spec.md deixa a pergunta em aberto, sem
  recomendação própria.
- **Se o Marco (CI6) tem custo em dinheiro ou é sempre de graça** — a spec
  não fala em preço para ele, só em existir.
