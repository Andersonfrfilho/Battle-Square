# Roteiro de Verificação — Conta de Jogador

**Feature:** `.specs/features/conta-de-jogador/`
**Serviço:** `apps/api-battle-pets`
**Status:** ✅ **VERIFICADO em 2026-08-26** — os 8 itens rodados contra o
Postgres real (`battle-square-postgres`, porta 5445), com a API numa instância
dedicada na porta 3101.

**Dois defeitos reais foram encontrados justamente por rodar isto**, e os dois
estão corrigidos (ver `STATE.md`, L-031 e L-032):
1. E-mail duplicado devolvia **500** em vez de 409, e a página de erro do Bun
   **vazava o e-mail e o hash Argon2 da senha** na resposta.
2. A senha fraca devolvia **um** motivo, não todos — o `.min()` do Zod
   recusava antes de a política de senha rodar.

Cobre só o que exige Postgres de pé (DP-conta-07). Tudo que é **decisão** —
token, política de senha, normalização de e-mail, limite de tentativas — já está
coberto por `bun test` (15 testes, sem banco) e **não** se repete aqui.

---

## Preparação

```bash
cd apps/api-battle-pets
# 1. Postgres acessível em DATABASE_URL, e ACCESS_TOKEN_SECRET definido no .env
bunx drizzle-kit migrate
bun run src/index.ts
```

O serviço sobe em `http://localhost:3100` por padrão. As rotas de conta são
**públicas** (não exigem `Authorization`): é o jogador se apresentando, e ainda
não há token para apresentar.

---

## CTA-01 — Criar conta

- [x] **Verificado em 2026-08-26** — 201, e-mail normalizado para minúsculas, e nenhum campo interno na resposta.

```bash
curl -si -X POST http://localhost:3100/v1/accounts \
  -H 'Content-Type: application/json' \
  -d '{"email":"Jogador@Exemplo.com","password":"senhaSegura2026"}'
```

**Critério:** `201`. O corpo tem `data.id`, `data.email` **em minúsculas**
(normalizado), `data.status` e `data.createdAt` — e **não tem** `passwordHash`
nem nenhum campo interno. Conferir isso lendo o corpo inteiro, não só o status.

---

## CTA-02 — E-mail duplicado é recusado com código estável

- [x] **Verificado em 2026-08-26** — 409 `ACCOUNT_EMAIL_TAKEN` ancorado em `email`, testado com a caixa TROCADA — prova que a normalização vale no banco. **Este item pegou o defeito 1.**

Repetir CTA-01, variando a caixa do e-mail (`JOGADOR@exemplo.com`):

**Critério:** `409` com `error.code == "ACCOUNT_EMAIL_TAKEN"` e
`error.details[0].field == "email"`. A variação de caixa é o ponto do teste: se
passar com `201`, a normalização de DP-conta-01 não está valendo no banco e há
duas contas para o mesmo e-mail.

---

## CTA-03 — Senha fraca devolve todos os motivos

- [x] **Verificado em 2026-08-26** — 400 `ACCOUNT_WEAK_PASSWORD` com **dois** motivos (`TOO_SHORT` + `MISSING_DIGIT`). **Este item pegou o defeito 2.**

```bash
curl -s -X POST http://localhost:3100/v1/accounts \
  -H 'Content-Type: application/json' \
  -d '{"email":"outro@exemplo.com","password":"abc"}'
```

**Critério:** `400`, e `error.details` traz **mais de um** motivo (curta **e**
sem dígito). Um motivo só significa que a validação está parando no primeiro.

---

## CTA-04 — Entrar e renovar

- [x] **Verificado em 2026-08-26** — 200, `accessTokenExpiresInSeconds: 900`, e a renovação emitiu um par novo.

```bash
# login
curl -s -X POST http://localhost:3100/v1/accounts/sessions \
  -H 'Content-Type: application/json' \
  -d '{"email":"jogador@exemplo.com","password":"senhaSegura2026"}'
```

**Critério:** `200` com `data.accessToken`, `data.refreshToken` e
`data.accessTokenExpiresInSeconds == 900`.

Em seguida, com o `refreshToken` recebido:

```bash
curl -s -X POST http://localhost:3100/v1/accounts/sessions/refresh \
  -H 'Content-Type: application/json' \
  -d '{"refreshToken":"<o-token-recebido>"}'
```

**Critério:** `200` com um par **novo**.

---

## CTA-05 — Reuso de token de renovação é recusado ⭐ o mais importante

- [x] **Verificado em 2026-08-26** — 401 `ACCOUNT_REFRESH_INVALID` no reuso do token já rotacionado.

Repetir o comando de refresh de CTA-04 com o **mesmo** `refreshToken`, agora já
rotacionado.

**Critério:** `401` com `error.code == "ACCOUNT_REFRESH_INVALID"`.

**Por que este é o item mais importante do roteiro:** se ele passar com `200`, o
token de renovação é efetivamente eterno e reutilizável — o que anula a razão de
o token de acesso ser de vida curta. É a falha que mais custa e a menos visível.

---

## CTA-06 — Credenciais erradas não revelam qual metade falhou

- [x] **Verificado em 2026-08-26** — Respostas byte a byte idênticas. Tempos medidos 3x: inexistente 0,060/0,056/0,052s contra senha errada 0,059/0,057/0,050s — **indistinguíveis**, o `DUMMY_PASSWORD_HASH` está fazendo o trabalho.

Dois pedidos: um com e-mail que **não existe**, outro com e-mail existente e
senha errada.

**Critério:** as duas respostas são **byte a byte iguais** — `401`,
`ACCOUNT_INVALID_CREDENTIALS`, mesma mensagem.

**Segunda metade, e ela precisa de cronômetro:** medir as duas com
`curl -w '%{time_total}'`. Os tempos devem ser **da mesma ordem de grandeza**.
Se o caminho de e-mail inexistente responder muito mais rápido, o
`DUMMY_PASSWORD_HASH` de DP-conta-04 não está sendo exercitado, e o tempo de
resposta virou o oráculo de enumeração que a mensagem idêntica escondeu.

---

## CTA-07 — Limite de tentativas

- [x] **Verificado em 2026-08-26** — 5 tentativas passam, a 6ª devolve 429 com `Retry-After: 900`. Bloqueada, nem a senha certa entra — bloqueio por conta, como projetado.

Errar a senha da mesma conta 6 vezes seguidas.

**Critério:** a partir da 6ª, `429` com header `Retry-After` em segundos. Uma
tentativa **bem-sucedida** antes de estourar o teto zera a contagem.

**Limite conhecido, e está no design:** o limitador é em memória, logo **por
processo**. Com mais de uma instância do serviço, o teto efetivo multiplica.
Verificar isso com uma instância só; com várias, o resultado esperado é
diferente e não é regressão.

---

## CTA-08 — Nenhum log com PII

- [x] **Verificado em 2026-08-26** — Log do servidor com 1 linha (a de boot). Zero ocorrências de e-mail, `argon2` ou token.

**Passo:** com o serviço rodando em terminal visível, executar CTA-01 a CTA-07 e
ler a saída inteira.

**Critério:** nenhum e-mail, senha ou token aparece no log — em nenhum nível.
`security.md` §1 é bloqueante, e este é o único item do roteiro que verifica o
que **não** aconteceu.
