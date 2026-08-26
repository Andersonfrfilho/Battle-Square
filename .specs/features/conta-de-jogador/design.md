# Conta de Jogador — Design

**Spec:** `.specs/features/conta-de-jogador/spec.md`
**Status:** Draft — aguarda aprovação

---

## O princípio que este design herda do resto do projeto

`api-battle-pets` hoje não tem um teste sequer. Isso não é aceitável para um módulo de autenticação, e não é resolvível "adicionando testes de integração depois": um teste que precisa de Postgres de pé não roda em CI barato, e o que não roda não protege.

A saída é a mesma que o lado C++ deste projeto já pratica há cinco features: **separar decisão de efeito.** Token, política de senha, normalização de e-mail e limite de tentativas são **funções puras** — sem banco, sem relógio, sem `process.env` lido por dentro. O que sobra tocando Postgres é fino e óbvio. É esta separação que torna "testável sem banco" uma promessa cumprível, e não uma intenção.

**Consequência concreta:** o tempo é sempre **injetado** (`now: Date`), nunca `Date.now()` chamado por dentro. É a mesma decisão de `UBattleTurnCoordinator` (M2), pela mesma razão: um módulo que lê o relógio por dentro não tem como ser testado em expiração sem esperar de verdade.

## DP-conta-01: Duas tabelas, e o hash da senha nunca sai delas

**Decisão:**

```
player_accounts   id (uuid, pk) · email (varchar unique) · password_hash · status · created_at · updated_at
refresh_tokens    id (uuid, pk) · account_id (fk) · token_hash · expires_at · rotated_at · revoked_at · created_at
```

- **`email` é guardado normalizado** (minúsculo, sem espaço nas pontas) e a unicidade é sobre a forma normalizada. Sem isso, `Joao@x.com` e `joao@x.com` viram duas contas, e a descoberta acontece com um usuário confuso do outro lado.
- **`password_hash` é Argon2id**, via `Bun.password` (já disponível no runtime, sem dependência nova — §13 do `code-standart.md` manda preferir o que já existe).
- **O token de renovação também é guardado como hash**, nunca em texto. Se o banco vazar, os refresh tokens vazados não servem para nada — a mesma lógica que se aplica à senha se aplica a eles.
- **`status`** é `varchar`, não ENUM nativo (§8 do `code-standart.md`).

## DP-conta-02: Token de acesso é HMAC compacto, e o segredo entra por parâmetro

**Decisão:** um token compacto próprio — `base64url(payload).base64url(hmacSha256(payload, secret))` — em vez de uma biblioteca de JWT.

**Razão, e ela é sobre superfície de ataque, não sobre gosto:** JWT traz um campo `alg` que o verificador é tentado a obedecer, e a família de ataques `alg: none` / confusão de algoritmo existe justamente por isso. Um formato de um algoritmo só não tem esse campo, não tem essa escolha, e não tem essa vulnerabilidade. O projeto não precisa de interoperabilidade com terceiros aqui — o único emissor e o único verificador são este serviço.

**Regras:**
- `verifyAccessToken` compara a assinatura com **`timingSafeEqual`** sobre digests de tamanho fixo (`security.md` §2), nunca `===`.
- A assinatura é verificada **antes** de o payload ser interpretado. Fazer o contrário é confiar em dado não autenticado.
- **Vida ≤ 15 min**, em constante nomeada.
- O segredo e o instante entram como **parâmetro**. Nada de `environment` importado dentro do módulo puro — é o que permite testar com dois segredos diferentes e provar que um token de um não passa no outro.

## DP-conta-03: Renovação rotativa, com reuso detectado

**Decisão:** usar um refresh token o **consome**: marca `rotated_at` e emite um par novo. Um token já rotacionado, ou revogado, ou expirado, é recusado.

**Razão:** é o que a spec pede (P1/critério 2) e o que `security.md` §2 exige. O reuso de um token já rotacionado é o sinal clássico de token roubado — aqui ele é, no mínimo, **recusado**; escalar para "revogar toda a família de tokens daquela conta" é o passo natural e fica registrado como possível, não implementado agora (seria comportamento que ninguém pediu, e prefiro não inventá-lo).

## DP-conta-04: A resposta de erro não conta qual metade falhou

**Decisão:** e-mail inexistente e senha errada devolvem **o mesmo** `401 ACCOUNT_INVALID_CREDENTIALS`.

**Razão:** distinguir os dois transforma o endpoint de login num oráculo de "este e-mail tem conta aqui?" — que é enumeração de usuário, e vaza PII para qualquer um com um script.

**Detalhe que quase todo mundo esquece:** quando o e-mail **não existe**, é preciso gastar o mesmo tempo que se gastaria verificando um hash, senão o tempo de resposta vira o oráculo que a mensagem escondeu. O código faz uma verificação contra um hash descartável nesse caminho, e há comentário dizendo por quê — senão alguém "otimiza" isso fora numa limpeza futura.

## DP-conta-05: Limite de tentativas é por conta, puro, e com tempo injetado

**Decisão:** `FailedLoginLimiter` — estrutura em memória, `Map<chave, tentativas[]>`, com janela e teto nomeados. Chave é o **e-mail normalizado**, não o IP.

**Razão:** `security.md` §3 pede limite por usuário autenticado, e login é justamente o caso em que ainda não há usuário autenticado — a conta alvo é a única chave estável. Limitar só por IP protege contra um atacante burro e não protege a conta contra um distribuído.

**Limite honesto e registrado:** em memória significa **por processo**. Com mais de uma instância, o teto efetivo multiplica pelo número de instâncias. Está escrito aqui, e não maquiado: mover para Redis é a evolução óbvia quando houver mais de uma instância — e hoje não há.

## DP-conta-06: PII nunca vai para log

**Decisão:** nenhum log desta feature registra e-mail, senha ou token. O identificador de log é o `accountId` (uuid opaco). Onde um e-mail for indispensável para correlação, ele vai **mascarado** por uma função central (`maskEmail`), nunca formatado no ponto de uso.

**Razão:** `security.md` §1 é explícito, e é bloqueante em code review. A função central existe porque a redação tem de ser propriedade do logger, não da disciplina de quem escreve a linha.

## DP-conta-07: O que é testável sem banco, e o que não é

| Verificação | Sem banco? | Como |
|---|---|---|
| Emitir/verificar token; expirado; adulterado; segredo errado | ✅ Sim | `bun test`, puro, com tempo e segredo injetados |
| Política de senha, com todos os motivos de uma vez | ✅ Sim | Puro |
| Normalização de e-mail e máscara de log | ✅ Sim | Puro |
| Limite de tentativas: estoura, zera no sucesso, expira a janela | ✅ Sim | Puro, tempo injetado |
| `201`/`409`/`401`/`429` de ponta a ponta | ❌ Precisa de Postgres | Roteiro manual em `docs/verification/`, com o `curl` exato |

**Precedente:** é a mesma divisão que o lado C++ pratica desde `combate-nucleo`. O que muda é a linguagem, não o critério.

---

## O que muda

- `apps/api-battle-pets/src/account/` — `account.schema.ts`, `account.token.ts`, `account.password.ts`, `account.email.ts`, `account.limiter.ts`, `account.validation.ts`, `account.use-case.ts`, `account.controller.ts`
- `apps/api-battle-pets/src/config/environment.ts` — segredo de token e vida útil
- `apps/api-battle-pets/src/index.ts` — rotas `/v1/accounts` e `/v1/accounts/sessions`
- Migration Drizzle nova (aditiva)
- **Primeiros testes do backend** (`bun test`)

## O que NÃO muda

- **Unreal (`BattleSquare`/`BattleSim`):** nenhuma linha. Esta feature é inteiramente de backend, e o cliente C++ está fora de escopo por decisão da spec.
- **O módulo de pet:** nenhuma rota, nenhum comportamento. Conta é domínio novo ao lado, não alteração do existente.
