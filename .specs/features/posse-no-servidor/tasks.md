# Posse no servidor — tarefas

**Regra de fatiamento desta feature:** nenhuma tarefa exige trocar tudo de uma
vez, e **todas rodam com o `USaveGame` local ainda presente**. Migrar posse de
quem já joga (PS9) é o passo mais arriscado da lista, e por isso ele vem depois
de a posse do servidor já estar funcionando e observável — nunca antes.

O ciclo de cada tarefa é o da casa:

```
ler a task  →  procurar o cano que já existe  →  escrever o teste E o
contrapeso  →  implementar  →  build  →  bateria  →  auditorias  →
grep fora de /Tests/  →  pôr na tela  →  commit  →  próxima
```

---

## PS1 — A tabela da posse EXISTE no banco
> 🤖 Modelo: `sonnet`

**Cano que já existe:** `apps/api-battle-pets/src/ownership/ownership.schema.ts`
está escrito, com dono, `catalog_id`, aquisição, `stolen_from_account_id`, índice
por dono e unicidade por `(dono, catálogo)`. O gerador de migration é o mesmo que
produziu `0000` a `0008`.

**O que a medição achou:** `drizzle.config.ts` lista `pet.schema.ts`,
`account.schema.ts` e `moderation.schema.ts` — **e não lista
`ownership.schema.ts`**. Não é uma migration esquecida: é uma tabela que o
gerador não tem como gerar. `rg owned_pets apps/api-battle-pets/drizzle/` não
acha nada.

**Dependências:** nenhuma.

**Verificação:**
```bash
cd apps/api-battle-pets && bunx drizzle-kit generate && bun test
rg -n owned_pets apps/api-battle-pets/drizzle/
```

*Aceite:* existe migration **aditiva** criando `owned_pets`, e a unicidade
`(dono, catálogo)` está nela. Sem a constraint no banco, a idempotência da
captura dependeria do CLIENTE — que é exatamente o que uma posse de servidor
existe para não fazer.

*Contrapeso obrigatório:* a migration não altera nem remove nada de `0000`–`0008`.
Rodar a bateria antes e depois dá o mesmo resultado nas outras tabelas — uma
migration de posse que mexesse em pet ou conta trocaria um problema de posse por
um problema de dado assinado.

---

## PS2 — O token do JOGADOR autentica, e `admin` não é dono de nada
> 🤖 Modelo: `opus` 🧠 — fronteira de autenticação

**Cano que já existe:** `account/account.token.ts` emite e verifica token de
acesso (`verifyAccessToken`, HMAC, segredo e instante INJETADOS — DP-conta-02).
`auth/auth.middleware.ts` já faz comparação `timingSafeEqual`.

**O que a medição achou:** o middleware conhece **dois tokens estáticos**
(`ADMIN_API_TOKEN`, `SYNC_API_TOKEN`) e dois escopos (`admin`, `sync`).
`verifyAccessToken` não é chamado por nenhuma rota. **Nenhuma rota do serviço
sabe quem é o jogador.**

**Dependências:** nenhuma (é backend puro, independe de PS1).

**Verificação:**
```bash
cd apps/api-battle-pets && bun test
```

*Aceite:* um token de acesso de conta válido autentica e **traz o `accountId`**;
token expirado, adulterado ou emitido com outro segredo não autentica. A decisão
é pura e testada sem banco, como o resto do módulo de conta.

*Contrapeso obrigatório:* **o `ADMIN_API_TOKEN` NÃO passa a valer como jogador.**
Se ele valesse, um segredo estático e sem dono seria o dono de todas as
coleções — e a posse no servidor não teria ficado mais segura que o arquivo que
ela substitui, só mais difícil de editar.

---

## PS3 — A autorização é por OBJETO, e há teste negativo de isolamento
> 🤖 Modelo: `opus` 🧠 — é a regra que impede o roubo por HTTP

**Cano que já existe:** o `accountId` que PS2 passou a extrair, e a coluna
`owner_account_id` de `owned_pets`.

**Por quê:** `security.md` §2 e o BOLA/API1 do OWASP. Checar só a ROTA parece
suficiente e é o defeito mais comum em API REST — a rota está certa e o objeto é
de outra pessoa. `database.md` exige, junto, **teste negativo de isolamento**:
a query filtra o dono por construção, e há prova de que filtrar falta acusa.

**Dependências:** PS1, PS2.

**Verificação:**
```bash
cd apps/api-battle-pets && bun test
```

*Aceite:* a conta A **não lê e não escreve** um pet cujo `owner_account_id` é da
conta B. O código de recusa é estável e o mesmo para os dois verbos.

*Contrapeso obrigatório:* o teste que prova o isolamento **falha se o filtro de
dono for removido**. Um teste que passa com e sem o `where` não está medindo
isolamento — está medindo que a rota responde.

---

## PS4 — As rotas da posse existem, e a captura é idempotente
> 🤖 Modelo: `sonnet`

**Cano que já existe:** `ownership.pure.ts` — `transferOwnership` com os quatro
tipos (`capture`, `trade`, `theft`, `return`), `isStolen` e `revealsInBattle`,
tudo testado sem banco desde `cb54335`. A regra **já está escrita**; falta um
chamador.

**O que a medição achou:** `src/index.ts` não tem caminho de posse nenhum. As
regras puras são chamadas só pelo próprio teste.

**Dependências:** PS1, PS2, PS3.

**Verificação:**
```bash
cd apps/api-battle-pets && bun test
# roteiro de ponta a ponta (precisa de Postgres): docs/verification/posse-no-servidor.md
```

*Aceite:* a coleção da conta do token é listável, e registrar a MESMA captura
duas vezes deixa **uma** linha — a segunda bate na unicidade de PS1, não numa
checagem do cliente. `CaptureIfNewInMemory` já é idempotente no jogo; aqui a
garantia deixa de depender de quem chama.

*Contrapeso obrigatório:* a lista da conta A **nunca** contém pet da conta B,
mesmo com paginação e mesmo com a conta B tendo o mesmo `catalog_id`. Duas contas
com o mesmo pet do catálogo é o caso normal, e é exatamente o que uma unicidade
mal escrita transforma em erro.

---

## PS5 — O lado da batalha passa a ter CONTA, sem tocar em rede
> 🤖 Modelo: `opus` 🧠 — mexe na costura de B-005

**Cano que já existe:** `ABattleArena::ResolveCollectionSlotForSide` e
`PetCollectionSlotForSide[2]`. **Vazio já significa "lado sem dono"** — o
selvagem, a IA — e lado sem dono não captura nem progride. A costura de "de quem
é este lado" existe e está isolada num ponto só.

**Por que não criar um `OwnerId` novo ao lado:** daria duas respostas para "de
quem é este lado", e as duas concordariam até a primeira partida em rede — que é
literalmente como B-005 nasceu.

**O que a medição achou:** `grep -rn "AccountId" Source/` devolve **zero**
linhas. O jogo não sabe o que é uma conta.

**Dependências:** nenhuma no jogo (não depende do backend estar pronto).

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
./Tools/sync_module_manifest.sh   # DEPOIS do build
grep -rn "AccountId" Source/ | grep -v "/Tests/"
```

*Aceite:* cada lado carrega a conta dona, **vazio = selvagem**, e o teste prova
que lado sem conta continua não capturando. Como a conta chega ao processo é
decisão do usuário (decisão 3 da spec) — enquanto ela não vem, o valor entra por
configuração, no mesmo lugar em que o tamanho da grade já entra.

*Contrapeso obrigatório:* **Standalone e Editor não mudam.** Sem conta
configurada, o caminho cai no slot local de hoje e a captura acontece igual. Foi
esse cuidado que fez a correção de B-005 não mudar o jogo de então, e ele vale
duas vezes aqui — porque agora há um servidor do outro lado para culpar.

---

## PS6 — O jogo pode falar HTTP, e o núcleo continua sem alcançá-lo
> 🤖 Modelo: `sonnet`

**Cano que já existe:** `BattleSquare.Build.cs`, que já consome o que a engine
tem em vez de vendorizar (SQLiteCore, OpenSSL — AD-019). As sondas de isolação
`Tools/probe_isolation.sh` e `Tools/probe_isolation_pet_backend.sh` já provam,
por build reprovado de propósito, que `BattleSim` não alcança SQLite nem OpenSSL.

**O que a medição achou:** `"HTTP"` não está em `BattleSquare.Build.cs`, e
`HttpModule`/`IHttpRequest` não aparecem em `Source/`. Não existe cliente HTTP
nenhum no jogo hoje.

**Dependências:** nenhuma.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
./Tools/probe_isolation.sh && ./Tools/probe_isolation_pet_backend.sh
./Tools/build_editor.sh   # L-020: a sonda deixa o .dylib do BattleSim quebrado
```

*Aceite:* `BattleSquare` compila com HTTP; a sonda que tenta usar HTTP **de
dentro de `BattleSim` não compila**, e a reprovação é a prova. Afirmação de
fronteira sem teste negativo que falhe de propósito é narrativa (L-004).

*Contrapeso obrigatório:* a sonda nova é **removida mesmo em interrupção**
(`trap`), como as que já existem. Sonda esquecida dentro do núcleo é uma furada
de fronteira que passa a bateria inteira em silêncio.

---

## PS7 — LER a coleção da conta, e a queda do backend não impedir jogar
> 🤖 Modelo: `opus` 🧠 — encosta em AD-014

**Cano que já existe:** o espelho local — `/v1/pets/export` →
`apps/worker-pet-sync` (`runSyncCycle`, cifra AES-256-GCM e verifica assinatura
Ed25519) → `FPetDataLoader::LoadVerifiedPets`. AD-014 é explícito: **a montagem
lê do espelho, nunca da rede diretamente**, e `ResolveTurn` não toca rede nem
disco sob nenhuma circunstância.

**A diferença que esta tarefa tem de respeitar:** o espelho de hoje é de **uma
via só** e carrega CATÁLOGO, que quase não muda. A posse muda durante o jogo e é
por conta. Ler a posse é caminho de **entrada no mundo**, fora da batalha.

**Dependências:** PS4 (rota de leitura), PS5 (o lado sabe de que conta é),
PS6 (HTTP).

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
./Tools/audit_no_recalculation.sh && ./Tools/audit_determinism.sh
# roteiro com o backend DERRUBADO: docs/verification/posse-no-servidor.md
```

*Aceite:* **com o backend fora do ar, a batalha monta e roda.** A coleção lida é
a última conhecida, e o jogo diz que ela é a última conhecida (PS10). Se a
montagem travasse esperando resposta, o jogo teria adquirido de graça o ponto
único de falha que AD-014 existe para não ter.

*Contrapeso obrigatório:* o teste prova que **nada no caminho de resolução de
turno** faz chamada de rede — nem indireta. É a mesma exigência que
`audit_determinism.sh` já cobra do núcleo, e a razão é a mesma: latência no meio
da regra é regra que muda de resultado por causa do Wi-Fi.

---

## PS8 — ESCREVER a captura, por fila, e uma vez só
> 🤖 Modelo: `opus` 🧠 — idempotência com efeito externo

**Cano que já existe:** a unicidade `(dono, catálogo)` de PS1 e a rota
idempotente de PS4. `FPetCollectionService::CaptureIfNewInMemory` já separa
DECISÃO de EFEITO — decidir se captura não toca disco.

**Por quê:** `security.md` §6 e as regras de fila do ecossistema pedem chave de
idempotência e retentativa com limite em todo job com efeito externo. Aqui o
efeito externo é a posse de outro jogador.

**Dependências:** PS4, PS7.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
# roteiro: derrubar o backend, capturar, subir o backend
# docs/verification/posse-no-servidor.md
```

*Aceite:* capturar com o backend fora do ar **não perde a captura** e **não
trava o jogo**; ao reconectar, ela sobe **uma vez só**. Reenviar a mesma captura
duas vezes deixa uma linha — e a garantia é da constraint, não da educação do
cliente.

*Contrapeso obrigatório:* a fila tem **teto de tentativas** e o que estourou o
teto **aparece** (PS10), em vez de acumular calado. Fila que retenta para sempre
em silêncio é progresso perdido que ninguém sabe que perdeu — e o dono do pet é
a última pessoa a descobrir.

---

## PS9 — Quem JÁ joga migra, sem perder nada, e a migração é repetível
> 🤖 Modelo: `opus` 🧠 — é o passo que apaga progresso se estiver errado

**Cano que já existe:** `FPetCollectionService::LoadCollection` /
`SaveCollection`, e o cuidado que as duas gravações já praticam — **preservar a
outra metade do save**, porque montar o save do zero e gravar por cima foi o que
faria salvar experiência apagar as especialidades do treinador.

**Por quê:** invariante 14 — save antigo carrega, e campo novo não apaga os pets
de quem já jogava. `UPROPERTY` ausente desserializa como vazio, e vazio é "não
informado". A migração da posse é o único passo desta feature que, feito errado,
apaga progresso que ninguém consegue devolver.

**Dependências:** PS4, PS7, PS8.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
# roteiro com save GRAVADO ANTES desta feature: docs/verification/posse-no-servidor.md
```

*Aceite:* uma coleção local gravada antes desta feature sobe inteira para a
conta, **e o save continua legível depois**. Rodar a migração duas vezes não
duplica pet nenhum — a segunda passada bate na unicidade de PS1.

*Contrapeso obrigatório:* migração **não apaga o save local**. Se a coleção
local vira cache ou desaparece é decisão do usuário (decisão 1 da spec), e
apagar o arquivo antes da resposta escolheria por ele — no sentido que não tem
volta.

---

## PS10 — A posse na TELA, com o que está pendente
> 🤖 Modelo: `sonnet`

**Cano que já existe:** o painel do mundo (`FBattleDebugScreen::Show`, chaves
740+, `WorldStatusReadout`), que já **amarela quando o adversário mais perto se
aproxima** — quem anda não acompanha número, percebe cor.

**Por quê:** a regra da casa. Sete de oito defeitos de 26–27/08 só apareceram
porque um humano olhou a tela; e uma fila de escrita é invisível por natureza —
ela funciona igual quando está funcionando e quando está parada.

**Dependências:** PS5, PS7, PS8.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
./Tools/audit_localizable_text.sh && ./Tools/gather_text.sh
# roteiro: docs/verification/posse-no-servidor.md
```

*Aceite:* o painel diz de quem é o pet e se a posse está **no servidor** ou
**pendente**, e a cor muda quando o backend está fora. Posse que nunca subiu
**não pode parecer sincronizada** — um pet que o jogador acha que é dele e que o
servidor não conhece é o pior estado possível desta feature.

*Contrapeso obrigatório:* texto do jogador é `LOCTEXT`/`NSLOCTEXT` com argumentos
**nomeados**, nunca `FString::Printf`, e **nada de PII** — nem e-mail, nem nome
de conta; o identificador que vai para a tela e para o log é opaco
(`security.md` §1, DP-conta-06). Depois de mexer em texto, rodar
`./Tools/gather_text.sh`.

---

## PS11 — Capturar passa a exigir SEM DONO
> 🤖 Modelo: `opus` 🧠 — muda regra que já tem teste afirmando o contrário

**Cano que já existe:** `ABattleArena::CheckForCapture` +
`FPetCollectionService::CaptureIfNew`, e o comentário que já está lá: *"QUEM
VENCEU captura, e a coleção é a DELE"*. O critério de hoje é a **derrota**.

**Por quê:** `captura-roubo-e-treinador` escreve por extenso qual é o defeito —
*"já existe `CaptureIfNew`, mas hoje o critério é a derrota, não a posse"*. Esta
é a Fatia 1 daquela spec, e a única que não exige entidade nova.

**Dependências:** PS4, PS7.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
./Tools/audit_test_helper_names.sh
grep -rn "CaptureIfNew" Source/ | grep -v "/Tests/"
```

*Aceite:* vencer uma batalha contra um pet **COM dono** não o põe na sua coleção
— e vencer contra um **selvagem** continua pondo. Hoje os dois casos põem, e é
por isso que este é o aceite.

*Contrapeso obrigatório:* **todo teste que afirma a regra VELHA vira o teste da
regra nova.** Mudar o código deixando de pé um teste que afirma "venceu, logo
capturou" deixaria a bateria verde sobre a regra antiga — que é pior que
vermelho, porque parece prova. Foi exatamente o que aconteceu na batalha de um
turno só: 189 testes verdes, e um deles *afirmava* o defeito.

---

## O que estas tarefas NÃO fazem

- **Não implementam roubo.** As regras de transferência por roubo, comércio e
  devolução **já estão escritas e testadas** (`ownership.pure.ts`, `cb54335`);
  ligá-las a uma ação de turno é `crime-e-recompensa`, e ela espera esta feature.
- **Não criam lista de procurados, polícia, recompensa nem o treinador como
  alvo.** Fatias 2 e 3 de `captura-roubo-e-treinador`.
- **Não decidem se um pet abandonado volta a ser selvagem.** É a pergunta 1
  daquela spec, e continua sem resposta — a posse a torna respondível.
- **Não sincronizam mochila, mapa, descoberta nem perfil de treinador.** Só a
  POSSE do pet. A mochila está sendo escrita agora em `itens-e-biologia`, e
  atravessar as duas no mesmo arquivo produziria conflito sem entregar nada.
- **Não tocam `BattleSim`.** Nenhuma linha. As sondas de isolação continuam
  sendo a prova, e PS6 acrescenta uma.
- **Não mexem no dado ASSINADO do pet.** A posse é outra tabela e aponta para
  `catalog_id`; se ela fosse um campo do payload canônico, cada captura passaria
  pela chave privada do backend.
- **Não inventam número nem rota de balanceamento.** Teto de coleção, custo e
  política de offline são decisões do usuário, listadas na spec.
- **Não apagam o save local.** Nem em PS9. Cache ou fim é decisão do usuário, e
  ela não tem volta.
