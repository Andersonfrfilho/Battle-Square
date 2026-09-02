# Posse no servidor — o pet tem DONO, e o dono é a conta

**Escrita em 02/09/2026.** Vem do último parágrafo de **B-005**, que fechou o
defeito de mistura e deixou escrito o que sobrou:

> *"A coleção continua num `USaveGame` local, agora separado por dono. Para o
> roubo de pet e para o cenário compartilhado, ela precisa ir para o SERVIDOR,
> ligada à conta que já existe no backend. Este conserto tirou o defeito de
> mistura; a posse ainda não é do servidor."*

## A frase

**De quem é o pet deixa de ser uma pergunta sobre o ARQUIVO da máquina e passa a
ser uma pergunta sobre a CONTA.**

---

## O que a medição achou antes de qualquer linha

Esta feature nasceu presumindo que a posse não existia no servidor. **Ela existe
pela metade, e a metade que falta é justamente a que a torna alcançável.** O
commit `cb54335` (31/08/2026) escreveu as regras de transferência e a tabela; o
resto do caminho não foi ligado.

| O que já existe, medido | Onde |
|---|---|
| As regras de transferência, PURAS e testadas | `apps/api-battle-pets/src/ownership/ownership.pure.ts` — `transferOwnership`, `isStolen`, `revealsInBattle` |
| A tabela `owned_pets`, com dono, aquisição, `stolen_from_account_id` e unicidade por `(dono, catálogo)` | `apps/api-battle-pets/src/ownership/ownership.schema.ts` |
| A conta, com token de acesso curto e renovação rotativa | `apps/api-battle-pets/src/account/` (`conta-de-jogador`, concluída) |
| A coleção POR LADO, com "vazio = sem dono" | `ABattleArena::ResolveCollectionSlotForSide` (B-005) |
| O caminho backend → jogo, assinado e cifrado | `/v1/pets/export` → `apps/worker-pet-sync` → espelho SQLite → `FPetDataLoader::LoadVerifiedPets` |

| O que NÃO existe, medido | Como se prova |
|---|---|
| **Migration da tabela** | `rg owned_pets apps/api-battle-pets/drizzle/` não acha nada. As migrations vão de `0000` a `0008` e nenhuma cria a tabela |
| **O gerador nem VÊ o arquivo** | `drizzle.config.ts` lista `pet.schema.ts`, `account.schema.ts` e `moderation.schema.ts` — e não `ownership.schema.ts`. Não é migration esquecida: é uma tabela que o gerador não tem como gerar |
| **Rota nenhuma** | `src/index.ts` não tem um caminho de posse. As regras puras não são chamadas por ninguém fora do próprio teste |
| **Autenticação de JOGADOR em qualquer rota** | `auth/auth.middleware.ts` conhece dois tokens ESTÁTICOS (`ADMIN_API_TOKEN`, `SYNC_API_TOKEN`) e dois escopos (`admin`, `sync`). `verifyAccessToken` existe e é usado só dentro do módulo de conta |
| **A palavra `AccountId` na Unreal** | `grep -rn "AccountId" Source/` devolve zero linhas. O jogo não sabe o que é uma conta |
| **Qualquer cliente HTTP na Unreal** | `BattleSquare.Build.cs` não declara `"HTTP"`, e `HttpModule`/`IHttpRequest` não aparecem em `Source/` |

**A conclusão que reordena o trabalho:** as primeiras tarefas desta feature não
inventam nada — elas **ligam o que já foi escrito e testado**. Escrever regra
nova de posse antes disso produziria a segunda fonte de verdade que já custou
L-032, L-033 e um defeito de direção neste projeto.

---

## Por que isto não é arrumação

**Sem posse no servidor, `crime-e-recompensa` não é implementável.** A spec dela
diz por escrito que B-005 deixa de ser limitação conhecida e vira bloqueio, e o
motivo é curto: num `USaveGame` local, **todo roubo é uma edição de arquivo**.
Quem quer o pet do outro não precisa vencer batalha nenhuma — precisa de um
editor de texto.

E `captura-roubo-e-treinador` está parada há dias esperando exatamente uma
coisa: **o conceito de DONO**. Sem ele, "sem dono", "selvagem" e "roubado" não
têm como ser distinguidos, e a captura de hoje usa o critério errado — ela
pergunta *"quem venceu?"* quando a pergunta é *"este pet tem dono?"*.

---

## O desenho

### A costura já existe, e é uma STRING

`PetCollectionSlotForSide[2]` **já é** "de quem é este lado": preenchido é lado
com dono, vazio é o selvagem e a IA, e lado sem dono não captura nem progride.
O que ele guarda é o **nome de um slot de save**.

A feature troca o que essa costura guarda, não a costura: onde hoje há um nome
de arquivo, passa a haver a **conta**. É por isso que o caminho aguenta rodar
com o save local ainda presente — o ponto de decisão é um só, e ele já está
isolado atrás de `ResolveCollectionSlotForSide`.

**Não se cria um segundo conceito de dono.** Um `OwnerId` novo ao lado do slot
por lado daria duas respostas para "de quem é este lado", e as duas concordariam
até a primeira partida em rede.

### A batalha nunca lê a rede — AD-014 vale aqui igual

O espelho local existe e é **de uma via só**: o backend exporta o catálogo, o
worker cifra e assina, o jogo lê. A posse é o contrário — ela **muda durante o
jogo**, e a mudança nasce no processo da arena.

Isso não autoriza a arena a falar com a rede. AD-014 é explícito: a montagem lê
do espelho, nunca da rede diretamente, e `FBattleResolver::ResolveTurn` não toca
rede nem disco sob nenhuma circunstância. Então a posse tem **dois movimentos
separados**:

| movimento | quando acontece | o que não pode fazer |
|---|---|---|
| **ler** a coleção da conta | ao ENTRAR no mundo, fora da batalha | bloquear a montagem se o backend estiver fora |
| **escrever** a captura | depois do fim da batalha, por fila | esperar resposta da rede para o jogo continuar |

**Se a escrita esperasse resposta, a batalha passaria a depender de latência** —
e o jogo teria adquirido, de graça, o ponto único de falha que AD-014 existe para
não ter.

### Autorização por OBJETO, ou "roubar" é uma requisição

Hoje o único token que escreve pet é o `ADMIN_API_TOKEN`: um segredo estático,
sem dono, que pode escrever qualquer registro. Se a coleção subir para o servidor
atrás desse token, **a posse não fica mais segura do que estava** — troca-se
editar um arquivo por mandar um `PUT`.

`security.md` §2 pede o oposto, e pede por objeto: **o servidor confere que o pet
pertence à conta do token**, não só que a rota é permitida. É o BOLA/API1 do OWASP
API Top 10, e é o campeão de vulnerabilidade em API REST justamente porque a
checagem por rota parece suficiente.

Consequência que precisa estar escrita: **`admin` não vira dono de tudo.** O
escopo administrativo continua existindo para o catálogo; a coleção de um jogador
responde ao token DELE.

### O dado do pet é ASSINADO, e a posse não o reassina

O catálogo do pet vem assinado do backend, e nomes antigos continuam valendo
porque recusá-los invalidaria pets que existem (invariante 13). A posse é
**registro de outra tabela**: ela aponta para `catalog_id`, e nada nela entra no
payload canônico do catálogo.

Isso é decisão, não acidente: se a posse fosse um campo do pet assinado, dar um
pet a alguém exigiria **reassinar o registro** — e cada captura passaria pela
chave privada do backend.

### Save antigo carrega, e é o passo mais arriscado

Invariante 14: campo novo na coleção não pode apagar os pets de quem já jogava —
`UPROPERTY` ausente desserializa como vazio, e vazio é "não informado".

Aqui isso vira uma exigência a mais: **a coleção local existente tem de virar
posse da conta sem perder nada, e a subida tem de ser repetível.** Migrar posse
de quem já joga é o único passo desta feature que, feito errado, apaga
progresso que ninguém consegue devolver.

---

## Aceite

**O aceite da feature, na forma do gabarito da casa:**

> **O MESMO pet aparece na coleção depois de trocar de máquina**; e o pet que
> tem dono não entra na coleção de quem venceu a batalha.

E o negativo, que é o que prova que a posse é do servidor e não do arquivo:

> **A conta A pede o pet da conta B e não o recebe** — nem para ler, nem para
> escrever. Sem isso, "roubar" é uma requisição HTTP bem formada.

E o que prova que a fronteira de AD-014 foi respeitada:

> **Com o backend derrubado, a batalha monta e roda.** A captura fica pendente e
> sobe depois, uma vez só.

---

## Decisões que são do usuário

Nada abaixo é implementável antes da resposta, e nenhuma tem resposta óbvia.

1. **A coleção local vira CACHE, ou desaparece?**
   *Cache* mantém o jogo jogável sem rede e obriga a decidir conflito entre o
   local e o remoto. *Desaparecer* elimina o conflito e transforma queda de
   backend em "não dá para jogar" — o oposto do princípio que
   `backend-dados-pet` estabeleceu ("o jogo não pode ficar refém do backend no
   caminho crítico"). **Recomendação: cache**, com o servidor mandando no
   empate; a recomendação não é a decisão.

2. **Quem joga OFFLINE, o que acontece?**
   Três respostas, três jogos diferentes: (a) joga e nada é gravado; (b) joga,
   grava local e sobe quando reconectar — e aí capturas offline podem colidir
   com o que o servidor já tem; (c) não joga sem conta. A conta é **opcional**
   por decisão da spec de `conta-de-jogador`, o que empurra para (a) ou (b), mas
   qual das duas é escolha de produto.

3. **Como a conta chega ao processo do jogo?**
   Não existe cliente C++ de conta — está fora de escopo por decisão explícita
   da spec de `conta-de-jogador`. Tela de login, token em configuração de
   desenvolvimento, ou o servidor de combate recebendo a identidade do
   pareamento (`sala-e-pareamento`, pronta)? A terceira é a que não inventa
   tela; também é a que não serve para jogar sozinho.

4. **Conta A pedindo o pet de B recebe `403` ou `404`?**
   `403` é honesto e confirma que o pet existe; `404` não confirma nada e é o
   que `conta-de-jogador` já escolheu para o login (DP-conta-04, não contar qual
   metade falhou). Duas respostas defensáveis, e a diferença é o quanto se
   aceita virar oráculo de "este pet existe?".

5. **Um pet ABANDONADO volta a ser selvagem?**
   Pergunta 1 de `captura-roubo-e-treinador`, ainda sem resposta. A posse no
   servidor a torna respondível — não a responde.

6. **A coleção tem teto?**
   Com save local, o teto era o disco de quem jogava. Com servidor, o custo é do
   dono do serviço, e "sem limite" é uma decisão financeira disfarçada de
   ausência de decisão.

---

## O que esta feature NÃO faz

- **Não implementa roubo.** Ela entrega o DONO, que é o pré-requisito da Fatia 1
  de `captura-roubo-e-treinador`. As regras de transferência por roubo já estão
  escritas e testadas; ligá-las a uma ação de turno é outra feature.
- **Não cria a lista de procurados, nem a polícia, nem a recompensa.** Tudo em
  `crime-e-recompensa`, que espera esta.
- **Não cria o treinador como alvo.** Fatia 3, e ela exige entidade nova.
- **Não sincroniza progressão de mundo, mapa, mochila nem descoberta.** Só a
  POSSE do pet. A mochila está sendo escrita agora em `itens-e-biologia`, e
  atravessar as duas features no mesmo arquivo produziria conflito sem entregar
  nada a mais.
- **Não toca `BattleSim`.** O núcleo é determinístico e não conhece conta
  nenhuma; a posse é camada de fora, e a sonda de isolação continua sendo a
  prova disso.
- **Não decide balanceamento nem número nenhum.** Onde falta número, a tarefa é
  medir ou perguntar.
